#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Tonic — opto-tube vocal compressor engine (Tier 2).
//
// Signal flow (per channel, per oversampled sample):
//   in → sidechain HPF (detector path) → Peak + RMS dual detection
//      → FIX/MAN/F+M envelope blend → opto-cell GR with thermal memory
//      → main signal × GR → tube push-pull soft saturation (drive)
//      → output transformer roll-off → makeup → dry/wet mix
//      → true-peak safety limiter
//
// Oversampling
//   Two pre-built juce::dsp::Oversampling instances (2x and 4x, IIR halfband
//   polyphase, max-quality). At 1x the engine runs at host rate; at 2x/4x
//   the entire engine runs upsampled — including the tanh-based tube
//   saturation which is the only nonlinear stage and the reason aliasing
//   matters here. Latency is reported to the host on factor change.
class CompressorEngine
{
public:
    CompressorEngine();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    // Parameter setters (called from processBlock each block)
    void setThreshold       (float dB)       { thresholdDb_ = dB; }
    void setRatio           (float r)        { ratio_ = juce::jmax (1.0f, r); }
    void setAttackMs        (float ms)       { attackMs_ = juce::jmax (0.1f, ms); }
    void setReleaseMs       (float ms)       { releaseMs_ = juce::jmax (1.0f, ms); }
    void setMakeupGainDb    (float dB)       { makeupDb_ = dB; }
    void setTimeMode        (int mode)       { timeMode_ = mode; }      // 0=FIX, 1=MAN, 2=FIX+MAN
    void setMix             (float wet01)    { mix_ = juce::jlimit (0.0f, 1.0f, wet01); }
    void setStereoMode      (int mode)       { stereoMode_ = mode; }    // 0=Stereo, 1=Dual, 2=M/S
    void setScHpf           (float hz)       { scHpfHz_ = juce::jlimit (10.0f, 1000.0f, hz); }
    void setDrive           (float d)        { drive_ = juce::jlimit (0.0f, 10.0f, d); }
    void setOversampling    (int factor);    // 1, 2, or 4 (anything else = 1)
    void setBypass          (bool b)         { bypass_ = b; }
    void setExternalSidechainEnabled (bool b){ externalSc_ = b; }

    // Host-rate latency for the currently active OS factor.
    int getLatencySamples() const;
    // Same, but for an arbitrary factor — used by the processor on the
    // message thread to call setLatencySamples() before the audio thread
    // actually picks up the new factor.
    int getLatencySamplesForFactor (int factor) const;

    // Meter accessors for GUI (atomic-safe reads)
    float getInputLevel()    const { return inputLevel_.load(); }
    float getOutputLevel()   const { return outputLevel_.load(); }
    float getGainReduction() const { return gainReductionDb_.load(); }
    bool  getClipping()      const { return clipping_.load(); }

    // Per-channel level accessors (true stereo metering)
    float getInputLevelL()  const { return inputLevelL_.load();  }
    float getInputLevelR()  const { return inputLevelR_.load();  }
    float getOutputLevelL() const { return outputLevelL_.load(); }
    float getOutputLevelR() const { return outputLevelR_.load(); }

private:
    // Per-channel filter + state
    struct ChannelState
    {
        juce::dsp::IIR::Filter<float> scHpf;
        float envFix     = 0.0f;     // program-dependent envelope (FIX path)
        float envMan     = 0.0f;     // user-defined envelope (MAN path)
        float optoMemory = 0.0f;     // thermal memory of the opto cell (slow drift)
        float overThreshAge = 0.0f;  // counter — how long signal has been over threshold
    };

    // Body of the original process() — runs at whatever sampleRate_ currently
    // is (host rate at 1x, hostRate*factor at 2x/4x).
    void processInternal (juce::AudioBuffer<float>& buffer);

    void updateScHpfCoefficients();
    void onEffectiveRateChanged();
    static float msToCoef (float ms, double sampleRate);

    // tube push-pull soft saturation (asymmetric — even harmonics dominate
    // at low drive, odd emerge as drive pushes)
    static inline float tubeSat (float x, float drive);

    double hostSampleRate_ = 44100.0;
    double sampleRate_     = 44100.0;  // = hostSampleRate_ * activeFactor

    // Two channels (mono = duplicated)
    ChannelState ch_[2];

    // Cached filter coefficients
    float scHpfHzCached_ = 0.0f;

    // Oversampling — pre-built so factor can switch without allocation in
    // the audio thread. Both default-constructed in the .cpp init list with
    // 2 channels and IIR halfband (max-quality, low-latency).
    juce::dsp::Oversampling<float> os2x_;
    juce::dsp::Oversampling<float> os4x_;
    int activeFactor_     = 1;   // 1, 2, or 4 — what we're actually running
    int requestedFactor_  = 1;   // what setOversampling last asked for
    int maxBlockSize_     = 0;

    // Params (snapshot per block)
    float thresholdDb_ = -20.0f;
    float ratio_       = 4.0f;
    float attackMs_    = 10.0f;
    float releaseMs_   = 200.0f;
    float makeupDb_    = 0.0f;
    int   timeMode_    = 0;
    float mix_         = 1.0f;
    int   stereoMode_  = 0;
    float scHpfHz_     = 20.0f;
    float drive_       = 0.0f;
    int   oversampling_ = 1;
    bool  bypass_      = false;
    bool  externalSc_  = false;

    // Meter outputs (read from GUI thread). Mono peaks kept for any legacy
    // consumer; per-channel atomics added for proper stereo metering.
    std::atomic<float> inputLevel_       { 0.0f };
    std::atomic<float> outputLevel_      { 0.0f };
    std::atomic<float> gainReductionDb_  { 0.0f };
    std::atomic<bool>  clipping_         { false };

    std::atomic<float> inputLevelL_      { 0.0f };
    std::atomic<float> inputLevelR_      { 0.0f };
    std::atomic<float> outputLevelL_     { 0.0f };
    std::atomic<float> outputLevelR_     { 0.0f };
};
