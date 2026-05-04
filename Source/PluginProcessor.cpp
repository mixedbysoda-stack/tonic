#include "PluginProcessor.h"
#include "PluginEditor.h"

#if TONIC_USE_WEBVIEW
 #include "WebViewEditor.h"
#endif

TonicProcessor::TonicProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Listen to every parameter so we can mark the loaded preset dirty when
    // the user touches anything.
    for (auto* p : getParameters())
    {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.addParameterListener (rp->paramID, this);
    }
}

void TonicProcessor::parameterChanged (const juce::String& id, float newValue)
{
    presetManager.markDirty();

    // Oversampling adds latency (a few host samples for IIR halfband). Report
    // it to the host as soon as the param changes — we're on the message
    // thread here so setLatencySamples is safe to call. The audio thread
    // will pick up the new factor on the next processBlock call.
    if (id == "oversampling")
    {
        const int idx = (int) newValue;
        const int factor = (idx == 0) ? 1 : (idx == 1) ? 2 : 4;
        setLatencySamples (engine.getLatencySamplesForFactor (factor));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout TonicProcessor::createParameterLayout()
{
    using P = std::unique_ptr<juce::RangedAudioParameter>;
    std::vector<P> params;

    // --- Main face controls (CL-1B 5-knob layout) ---
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Threshold",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -20.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ratio", 1 }, "Ratio",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f), 4.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 }, "Attack",
        juce::NormalisableRange<float> (0.5f, 300.0f, 0.1f, 0.3f), 10.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 }, "Release",
        juce::NormalisableRange<float> (50.0f, 10000.0f, 1.0f, 0.3f), 200.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 }, "Gain",
        juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 0.0f));

    // Time mode: 0=FIX, 1=MAN, 2=FIX+MAN
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "timeMode", 1 }, "Time Mode",
        juce::StringArray { "FIX", "MAN", "FIX+MAN" }, 0));

    // Meter source: 0=INPUT, 1=GR, 2=OUTPUT
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "meterSource", 1 }, "Meter",
        juce::StringArray { "INPUT", "GR", "OUTPUT" }, 1));

    // --- QoL strip ---
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "stereoMode", 1 }, "Stereo",
        juce::StringArray { "STEREO", "DUAL_MONO", "M_S" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "scHpf", 1 }, "SC HPF",
        juce::NormalisableRange<float> (20.0f, 500.0f, 1.0f, 0.3f), 20.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 }, "Drive",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "oversampling", 1 }, "Oversampling",
        juce::StringArray { "1x", "2x", "4x" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "externalSc", 1 }, "External SC", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return { params.begin(), params.end() };
}

bool TonicProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    auto scLayout = layouts.getChannelSet (true, 1);
    if (! scLayout.isDisabled()
        && scLayout != juce::AudioChannelSet::mono()
        && scLayout != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void TonicProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Stage the requested OS factor BEFORE prepare so the engine prepares
    // for the correct effective rate on the first block — otherwise we'd
    // get one block at host rate followed by a switch-over.
    const int osIdx    = (int) *apvts.getRawParameterValue ("oversampling");
    const int osFactor = (osIdx == 0) ? 1 : (osIdx == 1) ? 2 : 4;
    engine.setOversampling (osFactor);

    engine.prepare (sampleRate, samplesPerBlock);
    demoSampleCounter_ = 0;

    setLatencySamples (engine.getLatencySamplesForFactor (osFactor));
}

void TonicProcessor::releaseResources()
{
    engine.reset();
}

void TonicProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Push parameter state into the engine
    engine.setThreshold       (*apvts.getRawParameterValue ("threshold"));
    engine.setRatio           (*apvts.getRawParameterValue ("ratio"));
    engine.setAttackMs        (*apvts.getRawParameterValue ("attack"));
    engine.setReleaseMs       (*apvts.getRawParameterValue ("release"));
    engine.setMakeupGainDb    (*apvts.getRawParameterValue ("gain"));
    engine.setTimeMode        ((int) *apvts.getRawParameterValue ("timeMode"));
    engine.setMix             (*apvts.getRawParameterValue ("mix") * 0.01f);
    engine.setStereoMode      ((int) *apvts.getRawParameterValue ("stereoMode"));
    engine.setScHpf           (*apvts.getRawParameterValue ("scHpf"));
    engine.setDrive           (*apvts.getRawParameterValue ("drive"));
    {
        const int osIdx = (int) *apvts.getRawParameterValue ("oversampling");
        const int osFactor = (osIdx == 0) ? 1 : (osIdx == 1) ? 2 : 4;
        engine.setOversampling (osFactor);
    }
    engine.setBypass          (apvts.getRawParameterValue ("bypass")->load() > 0.5f);

    // External sidechain detection
    auto* scBus = getBus (true, 1);
    const bool hasExternalSc = (scBus != nullptr && scBus->isEnabled()
                                 && buffer.getNumChannels() > 2);
    externalScActive = hasExternalSc;
    engine.setExternalSidechainEnabled (
        hasExternalSc && apvts.getRawParameterValue ("externalSc")->load() > 0.5f);

    // Demo mode: 60s play / 10s mute cycle when not activated.
    // TONIC_DEV_BUILD is set by CMake for development builds (BUILD_WEBVIEW=ON)
    // so the v2 GUI work isn't blocked by activation. Production builds will
    // not have this flag set — the gate engages normally.
   #if ! TONIC_DEV_BUILD
    if (! licenseManager.isActivated())
    {
        const double sr = getSampleRate();
        const int totalSamples = buffer.getNumSamples();
        const double cycleSeconds = 70.0;
        const double playWindow   = 60.0;

        // Per-instance counter — no longer static, so multiple plugin
        // instances each have their own cycle and DAW restarts begin clean.
        demoSampleCounter_ += totalSamples;
        const double timeInCycleStart = std::fmod (double (demoSampleCounter_ - totalSamples) / sr, cycleSeconds);
        const double timeInCycleEnd   = std::fmod (double (demoSampleCounter_)                 / sr, cycleSeconds);

        const bool startMuted = timeInCycleStart >= playWindow;
        const bool endMuted   = timeInCycleEnd   >= playWindow;

        if (startMuted && endMuted)
        {
            // Whole block sits in the mute window — silent.
            buffer.clear();
            return;
        }

        if (startMuted != endMuted)
        {
            // Block straddles a boundary — apply an equal-power fade across
            // a small region (~32 samples) so we don't get a click.
            const int fadeLen = juce::jmin (32, totalSamples);
            const int fadeStart = startMuted ? 0 : juce::jmax (0, totalSamples - fadeLen);

            // First, run the engine on the un-muted samples
            engine.process (buffer);

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* data = buffer.getWritePointer (ch);
                for (int n = 0; n < totalSamples; ++n)
                {
                    if (startMuted)
                    {
                        // muted → playing : fade IN at the end of the fade region
                        if (n < fadeStart) data[n] = 0.0f;
                        else if (n < fadeStart + fadeLen)
                        {
                            const float t = float (n - fadeStart) / float (fadeLen);
                            data[n] *= std::sin (t * juce::MathConstants<float>::halfPi);
                        }
                    }
                    else
                    {
                        // playing → muted : fade OUT then silence
                        if (n >= fadeStart && n < fadeStart + fadeLen)
                        {
                            const float t = float (n - fadeStart) / float (fadeLen);
                            data[n] *= std::cos (t * juce::MathConstants<float>::halfPi);
                        }
                        else if (n >= fadeStart + fadeLen) data[n] = 0.0f;
                    }
                }
            }
            return;
        }
        // else: whole block is in the play window — fall through to engine.process
    }
   #endif

    engine.process (buffer);
}

juce::AudioProcessorEditor* TonicProcessor::createEditor()
{
   #if TONIC_USE_WEBVIEW
    return new WebViewEditor (*this);
   #else
    return new TonicEditor (*this);
   #endif
}

void TonicProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TonicProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TonicProcessor();
}
