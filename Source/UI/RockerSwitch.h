#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Vintage-style multi-position rocker switch.
// Renders a recessed dark slot with a 3D brushed-metal cap. The cap visually
// shifts position based on the current index (left / centre / right for 3-state,
// or left / right for 2-state). A fluoyellow LED dot glows beside the active
// position label.
class RockerSwitch : public juce::Component
{
public:
    RockerSwitch();
    ~RockerSwitch() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    void setOptions (const juce::StringArray& options);
    void setIndex   (int index, juce::NotificationType nt = juce::sendNotification);
    int  getIndex() const { return currentIndex_; }

    std::function<void(int)> onChange;

private:
    juce::StringArray options_ { "OFF", "ON" };
    int currentIndex_ = 0;
    float animatedIndex_ = 0.0f;

    juce::ComponentAnimator animator_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RockerSwitch)
};
