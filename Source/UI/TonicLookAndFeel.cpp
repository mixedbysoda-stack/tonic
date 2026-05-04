#include "TonicLookAndFeel.h"

TonicLookAndFeel::TonicLookAndFeel()
    : displayFont_ (juce::FontOptions ("Helvetica Neue", 16.0f, juce::Font::bold)),
      labelFont_   (juce::FontOptions ("Helvetica Neue", 11.0f, juce::Font::bold)),
      monoFont_    (juce::FontOptions ("Menlo", 12.0f, juce::Font::plain))
{
    // Default LookAndFeel colours
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (TonicColors::darkroom));
    setColour (juce::Label::textColourId,                 juce::Colour (TonicColors::ivory));
    setColour (juce::ComboBox::backgroundColourId,        juce::Colour (TonicColors::panel));
    setColour (juce::ComboBox::textColourId,              juce::Colour (TonicColors::ivory));
    setColour (juce::ComboBox::outlineColourId,           juce::Colour (0xff333333));
    setColour (juce::PopupMenu::backgroundColourId,       juce::Colour (TonicColors::panel));
    setColour (juce::PopupMenu::textColourId,             juce::Colour (TonicColors::ivory));
    setColour (juce::PopupMenu::highlightedBackgroundColourId,
                                                          juce::Colour (TonicColors::eviolet));
}

juce::Font TonicLookAndFeel::getLabelFont (juce::Label&)        { return labelFont_; }
juce::Font TonicLookAndFeel::getComboBoxFont (juce::ComboBox&)  { return labelFont_; }

void TonicLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPos,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (2.0f);
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // ── Outer rim (dark) ─────────────────────────────────────────────
    g.setColour (juce::Colour (TonicColors::brassShadow));
    g.fillEllipse (bounds);

    // ── Brushed brass body — radial gradient highlight from upper-left ─
    {
        juce::ColourGradient grad (
            juce::Colour (TonicColors::brassLight), cx - radius * 0.4f, cy - radius * 0.4f,
            juce::Colour (TonicColors::brassDark),  cx + radius * 0.4f, cy + radius * 0.4f,
            true);
        grad.addColour (0.4, juce::Colour (TonicColors::brass));
        g.setGradientFill (grad);
        g.fillEllipse (bounds.reduced (2.0f));
    }

    // ── Brushed lines (subtle horizontal) ────────────────────────────
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addEllipse (bounds.reduced (2.0f));
        g.reduceClipRegion (clip);

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        for (float yy = bounds.getY(); yy < bounds.getBottom(); yy += 2.0f)
            g.drawLine (bounds.getX(), yy, bounds.getRight(), yy, 0.5f);
    }

    // ── Inset shadow inside ──────────────────────────────────────────
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.drawEllipse (bounds.reduced (4.0f), 1.5f);

    // ── Pointer (cream) ──────────────────────────────────────────────
    {
        juce::Path pointer;
        const float pointerLen   = radius * 0.55f;
        const float pointerWidth = juce::jmax (2.0f, radius * 0.08f);
        pointer.addRoundedRectangle (-pointerWidth * 0.5f, -radius + 4.0f,
                                      pointerWidth, pointerLen, pointerWidth * 0.5f);
        g.setColour (juce::Colour (TonicColors::ivory));
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (cx, cy));

        // Glow halo behind pointer when value is high
        g.setColour (juce::Colour (TonicColors::ivory).withAlpha (0.25f));
        g.fillPath (pointer, juce::AffineTransform::rotation (angle)
                                  .scaled (1.4f, 1.0f, 0, -radius + pointerLen * 0.5f)
                                  .translated (cx, cy));
    }

    juce::ignoreUnused (slider);
}

void TonicLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                     bool /*isButtonDown*/,
                                     int /*buttonX*/, int /*buttonY*/,
                                     int /*buttonW*/, int /*buttonH*/,
                                     juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height);
    g.setColour (juce::Colour (TonicColors::panel));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (box.hasKeyboardFocus (false) ? juce::Colour (TonicColors::eviolet)
                                              : juce::Colour (0xff333333));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    // Arrow
    juce::Path arrow;
    auto ax = bounds.getRight() - 14.0f;
    auto ay = bounds.getCentreY();
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (juce::Colour (TonicColors::ivory));
    g.fillPath (arrow);
}
