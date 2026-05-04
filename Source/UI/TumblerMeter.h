#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Hero meter — a glass tumbler of glowing tonic water.
// Liquid level reflects output level; bubble density scales with gain
// reduction; foam appears at the meniscus when GR is high; chassis "overflow"
// graphic when GR is extreme.
class TumblerMeter : public juce::Component, private juce::Timer
{
public:
    TumblerMeter();
    ~TumblerMeter() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Update from the editor's meter Timer
    void setOutputLevel    (float v) { targetOutput_ = juce::jlimit (0.0f, 1.0f, v); }
    void setGainReduction  (float v) { gainReduction_ = juce::jlimit (0.0f, 1.0f, v); }

private:
    void timerCallback() override;

    struct Bubble
    {
        float x;        // local x position (0..1)
        float y;        // local y position (0..1, 0 = top)
        float radius;
        float speed;    // upward velocity per frame (0..1 / second-ish)
        float life;     // 0..1
    };

    std::vector<Bubble> bubbles_;
    juce::Random rng_;

    float targetOutput_   = 0.0f;
    float smoothedOutput_ = 0.0f;
    float gainReduction_  = 0.0f;
    float spawnAccum_     = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TumblerMeter)
};
