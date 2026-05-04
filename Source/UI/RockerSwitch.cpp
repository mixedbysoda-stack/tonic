#include "RockerSwitch.h"
#include "TonicLookAndFeel.h"

RockerSwitch::RockerSwitch()
{
    setInterceptsMouseClicks (true, false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void RockerSwitch::resized() {}

void RockerSwitch::setOptions (const juce::StringArray& options)
{
    if (options.size() < 2) return;
    options_ = options;
    currentIndex_ = juce::jlimit (0, options_.size() - 1, currentIndex_);
    animatedIndex_ = (float) currentIndex_;
    repaint();
}

void RockerSwitch::setIndex (int index, juce::NotificationType nt)
{
    index = juce::jlimit (0, options_.size() - 1, index);
    if (index == currentIndex_) return;
    currentIndex_ = index;

    // Smoothly animate the cap from its current animated position to the new target
    juce::Timer::callAfterDelay (16, [this] {
        // Drive a simple manual easing in 50ms via repaint
        struct Anim : public juce::Timer
        {
            RockerSwitch& self;
            float target;
            Anim (RockerSwitch& s, float t) : self (s), target (t) { startTimerHz (60); }
            void timerCallback() override
            {
                const float diff = target - self.animatedIndex_;
                if (std::abs (diff) < 0.005f)
                {
                    self.animatedIndex_ = target;
                    self.repaint();
                    delete this;
                    return;
                }
                self.animatedIndex_ += diff * 0.25f;
                self.repaint();
            }
        };
        new Anim (*this, (float) currentIndex_);
    });

    if (nt != juce::dontSendNotification && onChange)
        onChange (currentIndex_);
    repaint();
}

void RockerSwitch::mouseDown (const juce::MouseEvent&)
{
    setIndex ((currentIndex_ + 1) % options_.size());
}

void RockerSwitch::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Reserve space at bottom for the active-position label
    auto labelArea = bounds.removeFromBottom (16.0f);
    auto switchArea = bounds.reduced (4.0f);

    const int   numPositions = options_.size();
    const float w = switchArea.getWidth();
    const float h = switchArea.getHeight();

    // ── Recessed slot (dark inset) ───────────────────────────────────
    juce::Rectangle<float> slot = switchArea;
    g.setColour (juce::Colour (0xff0a0d18));
    g.fillRoundedRectangle (slot, 6.0f);

    // Inner shadow on slot top edge
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (slot.reduced (1.0f), 6.0f, 1.5f);

    // ── Position dots (LED indicators per position) ──────────────────
    for (int i = 0; i < numPositions; ++i)
    {
        const float t = (numPositions == 1) ? 0.5f : (float) i / float (numPositions - 1);
        const float dotX = slot.getX() + 8.0f + t * (slot.getWidth() - 16.0f);
        const float dotY = slot.getBottom() - 5.0f;

        const bool active = (i == currentIndex_);
        g.setColour (active ? juce::Colour (TonicColors::fluoyellow).withAlpha (0.9f)
                            : juce::Colour (0xff333333));
        g.fillEllipse (dotX - 1.5f, dotY - 1.5f, 3.0f, 3.0f);
        if (active)
        {
            g.setColour (juce::Colour (TonicColors::fluoyellow).withAlpha (0.4f));
            g.fillEllipse (dotX - 3.0f, dotY - 3.0f, 6.0f, 6.0f);
        }
    }

    // ── Cap (brushed metal lever that slides between positions) ──────
    const float capW = w / numPositions - 4.0f;
    const float capH = h - 14.0f; // leave room for LEDs at bottom
    const float trackW = w - capW - 8.0f;
    const float capX = slot.getX() + 4.0f
                       + (numPositions == 1 ? 0.0f
                                            : (animatedIndex_ / float (numPositions - 1)) * trackW);
    const float capY = slot.getY() + 3.0f;
    juce::Rectangle<float> cap (capX, capY, capW, capH);

    // Cap shadow
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (cap.translated (0, 2), 4.0f);

    // Cap body — brushed brass-grey gradient
    juce::ColourGradient capGrad (
        juce::Colour (0xffE0D5BE), cap.getX(), cap.getY(),
        juce::Colour (0xff6A5A3D), cap.getX(), cap.getBottom(),
        false);
    capGrad.addColour (0.45, juce::Colour (0xffB39768));
    g.setGradientFill (capGrad);
    g.fillRoundedRectangle (cap, 4.0f);

    // Cap brushed lines
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (cap, 4.0f);
        g.reduceClipRegion (clip);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        for (float yy = cap.getY(); yy < cap.getBottom(); yy += 1.5f)
            g.drawLine (cap.getX(), yy, cap.getRight(), yy, 0.4f);
    }

    // Cap top highlight + bottom shadow
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.drawLine (cap.getX() + 4, cap.getY() + 2, cap.getRight() - 4, cap.getY() + 2, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawLine (cap.getX() + 4, cap.getBottom() - 2, cap.getRight() - 4, cap.getBottom() - 2, 1.0f);

    // Cap rim
    g.setColour (juce::Colour (0xff5a4a2a));
    g.drawRoundedRectangle (cap, 4.0f, 0.8f);

    // Tiny fluoyellow LED on the cap to show it's active when not at index 0
    if (currentIndex_ > 0)
    {
        const float ledR = 2.0f;
        const float ledX = cap.getCentreX();
        const float ledY = cap.getCentreY();
        g.setColour (juce::Colour (TonicColors::fluoyellow).withAlpha (0.7f));
        g.fillEllipse (ledX - ledR, ledY - ledR, ledR * 2.0f, ledR * 2.0f);
        g.setColour (juce::Colour (TonicColors::fluoyellow).withAlpha (0.3f));
        g.fillEllipse (ledX - ledR * 2, ledY - ledR * 2, ledR * 4, ledR * 4);
    }

    // ── Active position label ────────────────────────────────────────
    g.setColour (juce::Colour (TonicColors::fluoyellow));
    g.setFont (juce::FontOptions ("Menlo", 10.0f, juce::Font::bold));
    g.drawText (options_[currentIndex_], labelArea.toNearestInt(),
                juce::Justification::centred, false);
}
