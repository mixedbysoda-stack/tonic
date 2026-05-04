#include "TumblerMeter.h"
#include "TonicLookAndFeel.h"

TumblerMeter::TumblerMeter()
{
    setOpaque (false);
    startTimerHz (60);
}

TumblerMeter::~TumblerMeter()
{
    stopTimer();
}

void TumblerMeter::resized() {}

void TumblerMeter::timerCallback()
{
    // Smooth the output level for nicer liquid motion
    smoothedOutput_ += (targetOutput_ - smoothedOutput_) * 0.18f;

    // Spawn bubbles at a rate proportional to gain reduction
    // Higher GR = more frenzied bubbles
    const float spawnRate = gainReduction_ * 8.0f; // bubbles per frame at GR=1.0
    spawnAccum_ += spawnRate;
    while (spawnAccum_ >= 1.0f && bubbles_.size() < 60)
    {
        spawnAccum_ -= 1.0f;
        Bubble b;
        b.x = 0.15f + rng_.nextFloat() * 0.7f;          // stay inside tumbler
        b.y = 1.0f;                                      // start at bottom
        b.radius = 1.5f + rng_.nextFloat() * 3.0f;
        b.speed = (0.3f + gainReduction_ * 1.5f + rng_.nextFloat() * 0.4f) / 60.0f;
        b.life = 1.0f;
        bubbles_.push_back (b);
    }

    // Move bubbles upward + age
    const float liquidTopY = 1.0f - smoothedOutput_;
    for (auto& b : bubbles_)
    {
        b.y -= b.speed;
        // Bubbles only exist within the liquid — fade out as they near the
        // surface, fully gone above it.
        if (b.y <= liquidTopY + 0.04f)
            b.life -= 0.08f;
    }

    bubbles_.erase (std::remove_if (bubbles_.begin(), bubbles_.end(),
                        [] (const Bubble& b) { return b.life <= 0.0f || b.y <= 0.05f; }),
                    bubbles_.end());

    repaint();
}

void TumblerMeter::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (4.0f);

    // ── Tumbler shape ────────────────────────────────────────────────
    // Wider at top, slightly tapered at bottom — classic rocks-glass profile.
    const float topW   = area.getWidth();
    const float botW   = area.getWidth() * 0.86f;
    const float botInset = (topW - botW) * 0.5f;
    const float top    = area.getY();
    const float bot    = area.getBottom();
    const float radius = 8.0f;

    juce::Path glass;
    glass.startNewSubPath (area.getX(), top + 4.0f);
    glass.lineTo (area.getX(), bot - radius);
    glass.quadraticTo (area.getX(), bot, area.getX() + botInset, bot);
    glass.lineTo (area.getRight() - botInset, bot);
    glass.quadraticTo (area.getRight(), bot, area.getRight(), bot - radius);
    glass.lineTo (area.getRight(), top + 4.0f);

    // ── Liquid (clipped to glass) ────────────────────────────────────
    const float liquidH = smoothedOutput_ * (area.getHeight() - 8.0f);
    auto liquidRect = juce::Rectangle<float> (area.getX(), bot - liquidH, area.getWidth(), liquidH);

    {
        juce::Graphics::ScopedSaveState ss (g);
        // Build a closed glass path for clipping
        juce::Path clipPath = glass;
        clipPath.lineTo (area.getRight(), bot);
        clipPath.lineTo (area.getX(), bot);
        clipPath.closeSubPath();
        g.reduceClipRegion (clipPath);

        // Liquid fill — fluoyellow with vertical gradient (lighter at top)
        juce::ColourGradient liquidGrad (
            juce::Colour (TonicColors::fluoyellow).withAlpha (0.95f), liquidRect.getX(), liquidRect.getY(),
            juce::Colour (TonicColors::fluoyellow).withAlpha (0.75f), liquidRect.getX(), liquidRect.getBottom(),
            false);
        g.setGradientFill (liquidGrad);
        g.fillRect (liquidRect);

        // Inner glow on the liquid (warm bloom)
        juce::ColourGradient innerGlow (
            juce::Colour (TonicColors::fluoyellow).withAlpha (0.5f),
            liquidRect.getCentreX(), liquidRect.getY() + liquidRect.getHeight() * 0.3f,
            juce::Colours::transparentBlack,
            liquidRect.getCentreX() + liquidRect.getWidth() * 0.6f, liquidRect.getCentreY(),
            true);
        g.setGradientFill (innerGlow);
        g.fillRect (liquidRect);

        // Bubbles (only those still alive)
        for (auto& b : bubbles_)
        {
            const float x = area.getX() + b.x * area.getWidth();
            const float y = top + b.y * area.getHeight();
            const float r = b.radius;
            const float alpha = juce::jlimit (0.0f, 0.8f, b.life * 0.85f);

            // Bubble body
            g.setColour (juce::Colours::white.withAlpha (alpha * 0.7f));
            g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);

            // Highlight
            g.setColour (juce::Colours::white.withAlpha (alpha));
            g.fillEllipse (x - r * 0.45f, y - r * 0.6f, r * 0.6f, r * 0.6f);
        }

        // Foam meniscus — appears when GR > 0.7
        if (gainReduction_ > 0.7f && liquidH > 4.0f)
        {
            const float foamAlpha = juce::jlimit (0.0f, 1.0f, (gainReduction_ - 0.7f) / 0.3f);
            const float foamY = liquidRect.getY();
            g.setColour (juce::Colours::white.withAlpha (foamAlpha * 0.9f));

            juce::Path foam;
            const int bubblesPerRow = 14;
            for (int i = 0; i < bubblesPerRow; ++i)
            {
                const float fx = area.getX() + (i + 0.5f) * area.getWidth() / bubblesPerRow;
                const float fr = 2.5f + (rng_.nextFloat() - 0.5f) * 1.5f;
                foam.addEllipse (fx - fr, foamY - fr, fr * 2.0f, fr * 2.0f);
            }
            g.fillPath (foam);

            // Foam glow
            g.setColour (juce::Colour (TonicColors::fluoyellow).withAlpha (foamAlpha * 0.4f));
            g.fillRect (area.getX(), foamY - 3.0f, area.getWidth(), 4.0f);
        }
    }

    // ── Liquid surface highlight (sits on top of the meniscus) ───────
    if (smoothedOutput_ > 0.02f)
    {
        const float surfaceY = bot - liquidH;
        g.setColour (juce::Colour (TonicColors::fluoyellow).withAlpha (0.9f));
        g.drawLine (area.getX() + 1.0f, surfaceY, area.getRight() - 1.0f, surfaceY, 1.5f);
    }

    // ── Glass outline + highlights ───────────────────────────────────
    // Subtle tinted reflection on left edge
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    juce::Path leftHighlight;
    leftHighlight.startNewSubPath (area.getX() + 6.0f, top + 14.0f);
    leftHighlight.quadraticTo (area.getX() + 4.0f, top + area.getHeight() * 0.5f,
                                area.getX() + 7.0f, bot - 16.0f);
    g.strokePath (leftHighlight, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));

    // Glass body outline
    g.setColour (juce::Colour (TonicColors::ivory).withAlpha (0.55f));
    g.strokePath (glass, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // Top rim (slightly thicker)
    g.setColour (juce::Colour (TonicColors::ivory).withAlpha (0.7f));
    g.drawLine (area.getX(), top + 4.0f, area.getRight(), top + 4.0f, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawLine (area.getX(), top + 6.0f, area.getRight(), top + 6.0f, 1.0f);

    // ── Overflow graphic — when GR > 0.9, liquid appears to seep over the rim ──
    if (gainReduction_ > 0.9f)
    {
        const float ovAlpha = (gainReduction_ - 0.9f) / 0.1f;
        g.setColour (juce::Colour (TonicColors::fluoyellow).withAlpha (ovAlpha * 0.85f));

        juce::Path drips;
        drips.startNewSubPath (area.getX() + 8.0f, top + 4.0f);
        drips.quadraticTo (area.getX() + 4.0f, top + 12.0f, area.getX() + 6.0f, top + 22.0f);
        drips.startNewSubPath (area.getRight() - 8.0f, top + 4.0f);
        drips.quadraticTo (area.getRight() - 4.0f, top + 12.0f, area.getRight() - 6.0f, top + 22.0f);
        g.strokePath (drips, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }
}
