#include "CompressorEngine.h"
#include <cmath>

CompressorEngine::CompressorEngine()
    // 2 channels, log2(factor) = 1 → 2x and 2 → 4x. IIR halfband polyphase
    // is the lowest-latency / lowest-CPU choice while still rejecting >70 dB
    // of the new mirror image — plenty for a tube saturation stage. Max-
    // quality flag = true for cleaner stopband; integerLatency = true so
    // host compensation lines up to whole samples.
    : os2x_ (2, 1,
             juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
             true, true)
    , os4x_ (2, 2,
             juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
             true, true)
{}

void CompressorEngine::prepare (double sampleRate, int samplesPerBlock)
{
    hostSampleRate_ = sampleRate;
    sampleRate_     = sampleRate;
    maxBlockSize_   = samplesPerBlock;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1;

    for (auto& c : ch_)
    {
        c.scHpf.prepare (spec);
        c.scHpf.reset();
    }

    // Oversamplers need to know the largest block we'll ever feed them so
    // they can pre-allocate their internal upsampled buffer. The block we
    // pass in process() may be smaller, that's fine.
    os2x_.initProcessing ((size_t) samplesPerBlock);
    os4x_.initProcessing ((size_t) samplesPerBlock);

    // Pick up any factor that was requested before prepare (e.g. set by the
    // processor in prepareToPlay so initial latency report lines up). If
    // the audio thread later changes requestedFactor_ between blocks, the
    // top of process() will catch it.
    if (requestedFactor_ != activeFactor_)
        activeFactor_ = juce::jlimit (1, 4, requestedFactor_);

    // Force HPF recompute for the (possibly oversampled) effective rate.
    scHpfHzCached_ = -1.0f;
    onEffectiveRateChanged();

    reset();
}

void CompressorEngine::reset()
{
    for (auto& c : ch_)
    {
        c.envFix = c.envMan = 0.0f;
        c.optoMemory = 0.0f;
        c.overThreshAge = 0.0f;
        c.scHpf.reset();
    }
    os2x_.reset();
    os4x_.reset();
    inputLevel_.store (0.0f);
    outputLevel_.store (0.0f);
    gainReductionDb_.store (0.0f);
    clipping_.store (false);
}

void CompressorEngine::setOversampling (int factor)
{
    requestedFactor_ = (factor == 4) ? 4 : (factor == 2) ? 2 : 1;
}

int CompressorEngine::getLatencySamples() const
{
    return getLatencySamplesForFactor (activeFactor_);
}

int CompressorEngine::getLatencySamplesForFactor (int factor) const
{
    if (factor == 2) return (int) os2x_.getLatencyInSamples();
    if (factor == 4) return (int) os4x_.getLatencyInSamples();
    return 0;
}

// Called whenever activeFactor_ changes — sync sampleRate_, invalidate the
// SC HPF cache so it gets rebuilt for the new rate, and reset oversampler
// state to avoid carrying stale samples across the rate transition.
void CompressorEngine::onEffectiveRateChanged()
{
    sampleRate_      = hostSampleRate_ * (double) activeFactor_;
    scHpfHzCached_   = -1.0f;
    updateScHpfCoefficients();
    // Reset the OS instance we're about to start using (or just left) so
    // there's no half-filtered tail in its delay line.
    if (activeFactor_ == 2) os2x_.reset();
    if (activeFactor_ == 4) os4x_.reset();
}

float CompressorEngine::msToCoef (float ms, double sampleRate)
{
    if (ms <= 0.0f) return 0.0f;
    return std::exp (-1.0f / (float (sampleRate) * ms * 0.001f));
}

void CompressorEngine::updateScHpfCoefficients()
{
    if (std::abs (scHpfHz_ - scHpfHzCached_) < 0.5f) return;
    scHpfHzCached_ = scHpfHz_;

    auto coefs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate_, scHpfHz_);
    for (auto& c : ch_)
        *c.scHpf.coefficients = *coefs;
}

// Tube push-pull saturation — asymmetric soft clip.
// drive 0..10. At low drive: gentle, even-harmonic (asymmetric).
// As drive pushes: harder soft clip, odd harmonics emerge.
inline float CompressorEngine::tubeSat (float x, float drive)
{
    if (drive <= 0.0001f) return x;

    const float d  = drive * 0.15f;          // map 0..10 → 0..1.5
    const float gd = 1.0f + d * 4.0f;        // input gain into stage
    float y = x * gd;

    // Asymmetry — bias the curve so positive/negative halves saturate differently.
    // This produces 2nd-harmonic (even) content like a single-ended tube.
    const float bias = 0.08f * d;            // small DC bias
    y += bias;

    // Soft-clip via tanh — adds odd harmonics smoothly.
    y = std::tanh (y);

    // Remove DC again
    y -= std::tanh (bias);

    // Normalize back so unity drive ≈ unity gain
    y *= 1.0f / (1.0f + d * 1.5f);
    return y;
}

void CompressorEngine::process (juce::AudioBuffer<float>& buffer)
{
    // Bypass meters are computed at host rate regardless of OS — there's
    // nothing the engine processes, so OS would just add latency for free.
    if (bypass_)
    {
        const int numCh = buffer.getNumChannels();
        const int numSm = buffer.getNumSamples();
        const float peakL = numCh > 0 ? buffer.getMagnitude (0, 0, numSm) : 0.0f;
        const float peakR = numCh > 1 ? buffer.getMagnitude (1, 0, numSm) : peakL;
        const float peak  = juce::jmax (peakL, peakR);
        inputLevel_.store (peak);
        outputLevel_.store (peak);
        inputLevelL_.store (peakL);
        inputLevelR_.store (peakR);
        outputLevelL_.store (peakL);
        outputLevelR_.store (peakR);
        gainReductionDb_.store (0.0f);
        clipping_.store (peak > 0.99f);
        return;
    }

    // Pick up requested OS factor changes between blocks (audio-safe — no
    // allocation, just buffer routing).
    if (requestedFactor_ != activeFactor_)
    {
        activeFactor_ = requestedFactor_;
        onEffectiveRateChanged();
    }

    if (activeFactor_ == 1 || buffer.getNumChannels() < 2)
    {
        // 1x — run the engine directly at host rate. (Mono input also takes
        // this path because the oversampler was sized for stereo; rather
        // than re-init mid-stream we just skip OS for the rare mono case.)
        processInternal (buffer);
        return;
    }

    auto& os = (activeFactor_ == 2) ? os2x_ : os4x_;

    juce::dsp::AudioBlock<float> block (buffer);
    auto upBlock = os.processSamplesUp (block);

    // Wrap the upsampled samples in an AudioBuffer so processInternal can
    // operate on it the same way it operates on the host buffer. The block
    // owns the storage; we just borrow the channel pointers.
    const int upChans = (int) upBlock.getNumChannels();
    const int upSamps = (int) upBlock.getNumSamples();
    float* chPtrs[2] = {
        upBlock.getChannelPointer (0),
        upChans > 1 ? upBlock.getChannelPointer (1) : upBlock.getChannelPointer (0)
    };
    juce::AudioBuffer<float> upBuffer (chPtrs, juce::jmin (2, upChans), upSamps);

    processInternal (upBuffer);

    os.processSamplesDown (block);
}

void CompressorEngine::processInternal (juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (2, buffer.getNumChannels());
    if (numChannels == 0 || numSamples == 0) return;

    updateScHpfCoefficients();

    const float attackCoef  = msToCoef (attackMs_,  sampleRate_);
    const float releaseCoef = msToCoef (releaseMs_, sampleRate_);

    // FIX-mode envelope timing — much slower attack, opto-style release that
    // depends on how long the signal has been over threshold.
    const float fixAttackCoef  = msToCoef (40.0f,  sampleRate_);   // forgiving slow attack
    const float fixReleaseFast = msToCoef (60.0f,  sampleRate_);   // dense passages
    const float fixReleaseSlow = msToCoef (1500.0f, sampleRate_);  // sparse passages

    const float threshLin = juce::Decibels::decibelsToGain (thresholdDb_);
    const float makeupLin = juce::Decibels::decibelsToGain (makeupDb_);
    const float invRatio  = 1.0f / ratio_;
    const bool  isMS      = (stereoMode_ == 2);
    const bool  isDual    = (stereoMode_ == 1);

    // Stereo / M-S input encoding
    auto* L = buffer.getWritePointer (0);
    auto* R = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    if (isMS && R != nullptr)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            const float m = (L[n] + R[n]) * 0.5f;
            const float s = (L[n] - R[n]) * 0.5f;
            L[n] = m;  // mid in ch0
            R[n] = s;  // side in ch1
        }
    }

    float inPeak = 0.0f, outPeak = 0.0f, grPeak = 0.0f;
    float inPeakL = 0.0f, inPeakR = 0.0f, outPeakL = 0.0f, outPeakR = 0.0f;
    bool clipFlag = false;

    for (int n = 0; n < numSamples; ++n)
    {
        // For Stereo (linked) mode we use a single shared GR computed from the
        // max of both channels. For Dual-Mono / M-S we run independent paths.
        const bool linked = !isDual && !isMS;

        // Detector signal — pre-HPF stereo magnitude
        float detL = std::abs (L[n]);
        float detR = (R != nullptr) ? std::abs (R[n]) : detL;

        // Sidechain HPF on the detector path (per channel)
        detL = std::abs (ch_[0].scHpf.processSample (detL));
        if (R != nullptr) detR = std::abs (ch_[1].scHpf.processSample (detR));

        const float detMax = juce::jmax (detL, detR);

        auto computeGr = [&] (ChannelState& cs, float det) -> float
        {
            // Update over-threshold "memory age"
            if (det > threshLin)
                cs.overThreshAge = juce::jmin (1.0f, cs.overThreshAge + 1.0f / (float (sampleRate_) * 0.1f)); // 100ms saturation
            else
                cs.overThreshAge = juce::jmax (0.0f, cs.overThreshAge - 1.0f / (float (sampleRate_) * 1.5f)); // 1.5s decay

            // ── MAN envelope: classic peak follower with user attack/release ──
            const float manCoef = (det > cs.envMan) ? attackCoef : releaseCoef;
            cs.envMan = manCoef * (cs.envMan - det) + det;

            // ── FIX envelope: slow attack, program-dependent release ────────
            // Release time blends fast (dense audio) → slow (sparse audio)
            // based on overThreshAge.
            const float fixRel = fixReleaseFast + (fixReleaseSlow - fixReleaseFast) * (1.0f - cs.overThreshAge);
            const float fixCoef = (det > cs.envFix) ? fixAttackCoef : fixRel;
            cs.envFix = fixCoef * (cs.envFix - det) + det;

            // Pick envelope per timeMode
            float env;
            switch (timeMode_)
            {
                case 0: env = cs.envFix; break;
                case 1: env = cs.envMan; break;
                default: // F+M = whichever envelope is currently more aggressive
                    env = juce::jmax (cs.envFix, cs.envMan);
                    break;
            }

            // Opto-cell thermal memory — release lags slightly behind the envelope
            // when the cell has been heavily illuminated. Subtle but audible;
            // 0.70 floor gives the famous opto "tail" without the squashed feel
            // a higher value produces.
            cs.optoMemory = 0.999f * cs.optoMemory + 0.001f * env;
            const float effectiveEnv = juce::jmax (env, cs.optoMemory * 0.70f);

            // Gain reduction with 6dB soft knee around threshold — quadratic
            // transition from 0..full ratio across [thresh-3 .. thresh+3] dB.
            // This is the CL-1B's natural compression curve and keeps the
            // top-end of vocals smooth instead of crunchy at the threshold.
            const float overDb = juce::Decibels::gainToDecibels (effectiveEnv) - thresholdDb_;
            const float kneeDb     = 6.0f;
            const float halfKnee   = kneeDb * 0.5f;
            float reductionDb = 0.0f;
            if (overDb >= halfKnee)
            {
                // Hard region — full ratio
                reductionDb = overDb * (1.0f - invRatio);
            }
            else if (overDb > -halfKnee)
            {
                // Soft-knee region — quadratic blend
                const float x = overDb + halfKnee;        // 0 .. kneeDb
                reductionDb = (x * x) / (2.0f * kneeDb) * (1.0f - invRatio);
            }
            return reductionDb;
        };

        float grLDb, grRDb;
        if (linked)
        {
            // Single detector for both channels — use channel 0's state
            const float reductionDb = computeGr (ch_[0], detMax);
            // Mirror state to keep ch_[1] coherent for mode-switch transitions
            ch_[1].envFix = ch_[0].envFix;
            ch_[1].envMan = ch_[0].envMan;
            ch_[1].optoMemory = ch_[0].optoMemory;
            ch_[1].overThreshAge = ch_[0].overThreshAge;
            grLDb = grRDb = reductionDb;
        }
        else
        {
            grLDb = computeGr (ch_[0], detL);
            grRDb = (R != nullptr) ? computeGr (ch_[1], detR) : grLDb;
        }

        const float grL = juce::Decibels::decibelsToGain (-grLDb);
        const float grR = juce::Decibels::decibelsToGain (-grRDb);

        // Apply GR to main signal
        float wetL = L[n] * grL * makeupLin;
        float wetR = (R != nullptr) ? R[n] * grR * makeupLin : 0.0f;

        // Tube push-pull saturation (drive)
        if (drive_ > 0.001f)
        {
            wetL = tubeSat (wetL, drive_);
            if (R != nullptr) wetR = tubeSat (wetR, drive_);
        }

        // Dry/wet mix (input pre-GR is the "dry" side)
        const float yL = L[n] * (1.0f - mix_) + wetL * mix_;
        const float yR = (R != nullptr) ? R[n] * (1.0f - mix_) + wetR * mix_ : 0.0f;

        L[n] = yL;
        if (R != nullptr) R[n] = yR;

        // Track meters — keep mono peaks for legacy consumers + per-channel
        // peaks for the stereo meter UI.
        const float aL  = std::abs (L[n]);
        const float aR  = (R != nullptr) ? std::abs (R[n]) : 0.0f;
        const float ayL = std::abs (yL);
        const float ayR = std::abs (yR);
        inPeak    = juce::jmax (inPeak,  aL, aR);
        outPeak   = juce::jmax (outPeak, ayL, ayR);
        inPeakL   = juce::jmax (inPeakL,  aL);
        inPeakR   = juce::jmax (inPeakR,  aR);
        outPeakL  = juce::jmax (outPeakL, ayL);
        outPeakR  = juce::jmax (outPeakR, ayR);
        grPeak    = juce::jmax (grPeak,  grLDb, grRDb);
        if (ayL > 0.999f || (R != nullptr && ayR > 0.999f)) clipFlag = true;
    }

    // Decode M/S back to L/R if needed
    if (isMS && R != nullptr)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            const float m = L[n];
            const float s = R[n];
            L[n] = m + s;
            R[n] = m - s;
        }
    }

    // True-peak safety brickwall — sample-peak ceiling at -0.1 dBFS.
    // Catches any overs from the saturation/makeup stage. Hard but transparent
    // for stray peaks; the proper TP detection lands when oversampling is enabled.
    {
        const float ceiling = juce::Decibels::decibelsToGain (-0.1f);
        for (int n = 0; n < numSamples; ++n)
        {
            if (std::abs (L[n]) > ceiling)
                L[n] = (L[n] > 0 ? 1.0f : -1.0f) * ceiling;
            if (R != nullptr && std::abs (R[n]) > ceiling)
                R[n] = (R[n] > 0 ? 1.0f : -1.0f) * ceiling;
        }
    }

    inputLevel_.store (inPeak);
    outputLevel_.store (outPeak);
    gainReductionDb_.store (grPeak);
    clipping_.store (clipFlag);
    inputLevelL_.store (inPeakL);
    inputLevelR_.store (inPeakR);
    outputLevelL_.store (outPeakL);
    outputLevelR_.store (outPeakR);
}
