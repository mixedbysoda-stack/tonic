#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/CompressorEngine.h"
#include "Licensing/LicenseManager.h"
#include "PresetManager.h"

class TonicProcessor : public juce::AudioProcessor,
                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    TonicProcessor();
    ~TonicProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Bus layout support — needed for external sidechain
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameters
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Engine access for GUI meters
    CompressorEngine& getEngine() { return engine; }

    // Licensing
    LicenseManager& getLicenseManager() { return licenseManager; }

    // Presets
    PresetManager& getPresetManager() { return presetManager; }

    // External sidechain state for GUI indicator
    bool isExternalSideChainActive() const { return externalScActive; }

private:
    void parameterChanged (const juce::String& id, float newValue) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    CompressorEngine engine;
    LicenseManager licenseManager;
    PresetManager presetManager { apvts };
    bool externalScActive = false;

    // Demo cycle state — per-instance, NOT static. Reset in prepareToPlay so
    // each session starts at the top of the play window, not mid-mute.
    long long demoSampleCounter_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TonicProcessor)
};
