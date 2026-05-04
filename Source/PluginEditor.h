#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/TonicLookAndFeel.h"
#include "UI/TumblerMeter.h"
#include "UI/RockerSwitch.h"
#include "Licensing/ActivationDialog.h"

class TonicEditor : public juce::AudioProcessorEditor,
                    private juce::Timer
{
public:
    explicit TonicEditor (TonicProcessor&);
    ~TonicEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    TonicProcessor& processor;
    TonicLookAndFeel laf;
    TumblerMeter tumbler;
    juce::Image carbonatedLogo;
    ActivationDialog activationDialog;
    juce::TextButton activateButton;

    // Knob controls
    juce::Slider threshold, ratio, attack, release, gain;
    juce::Slider mix, scHpf, drive;
    juce::Label thresholdL, ratioL, attackL, releaseL, gainL;
    juce::Label thresholdV, ratioV, attackV, releaseV, gainV;
    juce::Label mixL, scHpfL, driveL;
    juce::Label mixV, scHpfV, driveV;

    // Vintage rocker switches (multi-position cycle)
    RockerSwitch timeMode, meterSource, stereoMode, oversampling, externalSc;
    juce::TextButton bypass;
    juce::Label  timeModeL, meterL, stereoL, osL, scL;

    // APVTS attachments
    using SAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SAtt> thresholdAtt, ratioAtt, attackAtt, releaseAtt, gainAtt;
    std::unique_ptr<SAtt> mixAtt, scHpfAtt, driveAtt;
    std::unique_ptr<BAtt> bypassAtt;

    // Live meter values for the tumbler/halo (read each timer tick)
    float curInputLevel    = 0.0f;
    float curOutputLevel   = 0.0f;
    float curGainReduction = 0.0f;
    bool  curClipping      = false;
    float clipFlashAlpha   = 0.0f;

    // Helper to set up a brass knob
    void setupKnob (juce::Slider&, juce::Label& label, juce::Label& value,
                    const juce::String& labelText, const juce::String& suffix,
                    int decimals = 1);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TonicEditor)
};
