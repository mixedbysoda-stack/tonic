#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Tonic brand palette — matches the React reference exactly.
namespace TonicColors
{
    inline constexpr juce::uint32 darkroom    = 0xff0A0F1F; // background
    inline constexpr juce::uint32 chassis     = 0xff121623; // chassis face
    inline constexpr juce::uint32 chassisRim  = 0xff2A2F42; // chassis border
    inline constexpr juce::uint32 panel       = 0xff1A1A1A; // QoL strip background
    inline constexpr juce::uint32 brassLight  = 0xffE8D4A8;
    inline constexpr juce::uint32 brass       = 0xffC9A876;
    inline constexpr juce::uint32 brassDark   = 0xff8B7547;
    inline constexpr juce::uint32 brassShadow = 0xff5C4A2A;
    inline constexpr juce::uint32 ivory       = 0xffF5EFE0;
    inline constexpr juce::uint32 fluoyellow  = 0xffD4FF00;
    inline constexpr juce::uint32 eviolet     = 0xff7B2FFF;
    inline constexpr juce::uint32 tubeamber   = 0xffFFB347;
    inline constexpr juce::uint32 clipping    = 0xffFF2D7E;
}

class TonicLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TonicLookAndFeel();
    ~TonicLookAndFeel() override = default;

    // Knobs — brushed brass with cream pointer + fluoyellow value text
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;

    // ComboBox (preset selector) — dark with violet focus border
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

private:
    juce::Font displayFont_;  // Orbitron-ish for wordmarks
    juce::Font labelFont_;    // Inter-ish for labels
    juce::Font monoFont_;     // Mono for values

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TonicLookAndFeel)
};
