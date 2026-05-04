#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    // Wire a RockerSwitch to an AudioParameterChoice
    static void wireChoiceRocker (RockerSwitch& rs,
                                  juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& paramID,
                                  const juce::StringArray& labels)
    {
        auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramID));
        jassert (p != nullptr);

        rs.setOptions (labels);
        rs.setIndex (p->getIndex(), juce::dontSendNotification);

        rs.onChange = [p] (int newIndex) {
            p->beginChangeGesture();
            *p = newIndex;
            p->endChangeGesture();
        };
    }

    // Wire a RockerSwitch to an AudioParameterBool (using two-position labels)
    static void wireBoolRocker (RockerSwitch& rs,
                                juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& paramID,
                                const juce::StringArray& labels)
    {
        auto* p = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (paramID));
        jassert (p != nullptr);

        rs.setOptions (labels);
        rs.setIndex (p->get() ? 1 : 0, juce::dontSendNotification);

        rs.onChange = [p] (int newIndex) {
            p->beginChangeGesture();
            *p = (newIndex != 0);
            p->endChangeGesture();
        };
    }
}

TonicEditor::TonicEditor (TonicProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      activationDialog (p.getLicenseManager())
{
    setLookAndFeel (&laf);
    addAndMakeVisible (tumbler);

    // Load bundled Carbonated Audio logo
    carbonatedLogo = juce::ImageFileFormat::loadFrom (BinaryData::CarbonatedLogo_png,
                                                       BinaryData::CarbonatedLogo_pngSize);

    // ── Activation flow ───────────────────────────────────────────────
    // Dialog added invisibly — toFront when shown so it always paints on top
    addChildComponent (activationDialog);

    addAndMakeVisible (activateButton);
    activateButton.setButtonText ("ACTIVATE");
    activateButton.setColour (juce::TextButton::buttonColourId, juce::Colour (TonicColors::panel));
    activateButton.setColour (juce::TextButton::textColourOffId, juce::Colour (TonicColors::fluoyellow));
    activateButton.onClick = [this]() {
        activationDialog.setVisible (true);
        activationDialog.toFront (true);
    };

    activateButton.setVisible (! processor.getLicenseManager().isActivated());

    setSize (900, 560);

    auto& a = processor.getAPVTS();

    // ── Main 5 knobs ───────────────────────────────────────────────────
    setupKnob (threshold, thresholdL, thresholdV, "THRESHOLD", " dB", 1);
    setupKnob (ratio,     ratioL,     ratioV,     "RATIO",     ":1",   1);
    setupKnob (attack,    attackL,    attackV,    "ATTACK",    " ms",  1);
    setupKnob (release,   releaseL,   releaseV,   "RELEASE",   " ms",  0);
    setupKnob (gain,      gainL,      gainV,      "GAIN",      " dB",  1);

    thresholdAtt = std::make_unique<SAtt> (a, "threshold", threshold);
    ratioAtt     = std::make_unique<SAtt> (a, "ratio",     ratio);
    attackAtt    = std::make_unique<SAtt> (a, "attack",    attack);
    releaseAtt   = std::make_unique<SAtt> (a, "release",   release);
    gainAtt      = std::make_unique<SAtt> (a, "gain",      gain);

    // QoL knobs
    setupKnob (mix,   mixL,   mixV,   "MIX",    "%",    0);
    setupKnob (scHpf, scHpfL, scHpfV, "SC HPF", " Hz",  0);
    setupKnob (drive, driveL, driveV, "DRIVE",  "",     1);

    mixAtt   = std::make_unique<SAtt> (a, "mix",   mix);
    scHpfAtt = std::make_unique<SAtt> (a, "scHpf", scHpf);
    driveAtt = std::make_unique<SAtt> (a, "drive", drive);

    // ── Vintage rocker switches ───────────────────────────────────────
    auto setupRocker = [this] (RockerSwitch& rs, juce::Label& lbl, const juce::String& lblText) {
        addAndMakeVisible (rs);
        addAndMakeVisible (lbl);
        lbl.setText (lblText, juce::dontSendNotification);
        lbl.setJustificationType (juce::Justification::centred);
        lbl.setFont (juce::FontOptions ("Helvetica Neue", 10.0f, juce::Font::bold));
        lbl.setColour (juce::Label::textColourId, juce::Colour (TonicColors::ivory));
    };

    setupRocker (timeMode,     timeModeL, "TIME MODE");
    setupRocker (meterSource,  meterL,    "METER");
    setupRocker (stereoMode,   stereoL,   "STEREO");
    setupRocker (oversampling, osL,       "OVERSAMPLE");
    setupRocker (externalSc,   scL,       "SIDECHAIN");

    wireChoiceRocker (timeMode,     a, "timeMode",     { "FIX", "MAN", "F+M" });
    wireChoiceRocker (meterSource,  a, "meterSource",  { "IN",  "GR",  "OUT" });
    wireChoiceRocker (stereoMode,   a, "stereoMode",   { "ST",  "DUAL", "M/S" });
    wireChoiceRocker (oversampling, a, "oversampling", { "1x",  "2x",  "4x" });
    wireBoolRocker   (externalSc,   a, "externalSc",   { "INT", "EXT" });

    // Bypass — small text button top-right
    addAndMakeVisible (bypass);
    bypass.setClickingTogglesState (true);
    bypass.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (TonicColors::clipping));
    bypass.setColour (juce::TextButton::buttonColourId,    juce::Colour (TonicColors::panel));
    bypass.setColour (juce::TextButton::textColourOnId,    juce::Colour (TonicColors::ivory));
    bypass.setColour (juce::TextButton::textColourOffId,   juce::Colour (TonicColors::fluoyellow));
    bypassAtt = std::make_unique<BAtt> (a, "bypass", bypass);
    auto refreshByp = [this]() { bypass.setButtonText (bypass.getToggleState() ? "BYP" : "IN"); };
    bypass.onStateChange = refreshByp;
    refreshByp();

    // Auto-show activation dialog on first load if not licensed.
    // Done after all child components are added so toFront actually puts it
    // on top of the knob row, tumbler, etc.
    if (! processor.getLicenseManager().isActivated())
    {
        activationDialog.setVisible (true);
        activationDialog.toFront (true);
    }

    startTimerHz (30); // meter polling
}

TonicEditor::~TonicEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TonicEditor::setupKnob (juce::Slider& s, juce::Label& label, juce::Label& value,
                             const juce::String& labelText, const juce::String& suffix,
                             int decimals)
{
    addAndMakeVisible (s);
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                           juce::MathConstants<float>::pi * 2.75f, true);

    addAndMakeVisible (label);
    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions ("Helvetica Neue", 10.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (TonicColors::ivory));

    addAndMakeVisible (value);
    value.setJustificationType (juce::Justification::centred);
    value.setFont (juce::FontOptions ("Menlo", 11.0f, juce::Font::plain));
    value.setColour (juce::Label::textColourId, juce::Colour (TonicColors::fluoyellow));
    value.getProperties().set ("suffix", suffix);
    value.getProperties().set ("decimals", decimals);

    // Update value label whenever the slider moves
    s.onValueChange = [&s, &value, suffix, decimals]() {
        value.setText (juce::String (s.getValue(), decimals) + suffix,
                       juce::dontSendNotification);
    };
    // Trigger once
    value.setText (juce::String (s.getValue(), decimals) + suffix, juce::dontSendNotification);
}

void TonicEditor::timerCallback()
{
    auto& engine = processor.getEngine();
    curInputLevel    = engine.getInputLevel();
    curOutputLevel   = engine.getOutputLevel();
    curGainReduction = juce::jlimit (0.0f, 1.0f, engine.getGainReduction() / 24.0f);
    curClipping      = engine.getClipping();

    if (curClipping)        clipFlashAlpha = 0.6f;
    else if (clipFlashAlpha > 0.0f) clipFlashAlpha = juce::jmax (0.0f, clipFlashAlpha - 0.05f);

    // Drive the tumbler hero meter
    tumbler.setOutputLevel   (curOutputLevel);
    tumbler.setGainReduction (curGainReduction);

    // License state — auto-hide activation UI once activated
    const bool active = processor.getLicenseManager().isActivated();
    if (active)
    {
        if (activateButton.isVisible())   activateButton.setVisible (false);
        if (activationDialog.isVisible()) activationDialog.setVisible (false);
    }
    else
    {
        if (! activateButton.isVisible()) activateButton.setVisible (true);
    }

    // Force value-label refresh too (for host automation / preset loads)
    auto refresh = [] (juce::Slider& s, juce::Label& v) {
        const auto suffix   = v.getProperties()["suffix"].toString();
        const auto decimals = (int) v.getProperties()["decimals"];
        v.setText (juce::String (s.getValue(), decimals) + suffix, juce::dontSendNotification);
    };
    refresh (threshold, thresholdV);
    refresh (ratio,     ratioV);
    refresh (attack,    attackV);
    refresh (release,   releaseV);
    refresh (gain,      gainV);
    refresh (mix,       mixV);
    refresh (scHpf,     scHpfV);
    refresh (drive,     driveV);

    repaint();
}

void TonicEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.fillAll (juce::Colour (TonicColors::darkroom));

    // UV halo (pulses with input level)
    {
        const float haloAlpha = 0.18f + curInputLevel * 0.45f;
        const float r = bounds.getWidth() * 0.5f;
        juce::ColourGradient halo (
            juce::Colour (TonicColors::eviolet).withAlpha (haloAlpha),
            bounds.getCentreX(), bounds.getCentreY(),
            juce::Colours::transparentBlack,
            bounds.getCentreX() + r, bounds.getCentreY() + r,
            true);
        g.setGradientFill (halo);
        g.fillRect (bounds);
    }

    // Chassis — rounded rect with rim
    auto chassis = bounds.reduced (12.0f);
    g.setColour (juce::Colour (TonicColors::chassisRim));
    g.fillRoundedRectangle (chassis, 14.0f);
    g.setColour (juce::Colour (TonicColors::chassis));
    g.fillRoundedRectangle (chassis.reduced (4.0f), 12.0f);

    // Top bar background
    auto topBar = chassis.reduced (4.0f).removeFromTop (62.0f);
    g.setColour (juce::Colour (TonicColors::darkroom).withAlpha (0.8f));
    g.fillRoundedRectangle (topBar, 8.0f);

    // Wordmark
    g.setColour (juce::Colour (TonicColors::fluoyellow));
    g.setFont (juce::FontOptions ("Helvetica Neue", 38.0f, juce::Font::bold));
    auto wordmarkArea = topBar.reduced (24.0f, 8.0f);
    g.drawText ("TONIC", wordmarkArea, juce::Justification::centredLeft, false);

    // Tagline
    g.setColour (juce::Colour (TonicColors::ivory).withAlpha (0.6f));
    g.setFont (juce::FontOptions ("Helvetica Neue", 11.0f, juce::Font::plain));
    auto taglineX = wordmarkArea.getX() + 130.0f;
    g.drawText ("OPTO-TUBE VOCAL COMPRESSOR",
                juce::Rectangle<float> (taglineX, wordmarkArea.getY(),
                                        wordmarkArea.getWidth() - 130.0f, wordmarkArea.getHeight()),
                juce::Justification::centredLeft, false);

    // QoL strip background
    auto qol = chassis.reduced (4.0f).removeFromBottom (76.0f);
    g.setColour (juce::Colour (TonicColors::panel).withAlpha (0.6f));
    g.fillRoundedRectangle (qol, 8.0f);
    g.setColour (juce::Colour (TonicColors::chassisRim));
    g.drawRoundedRectangle (qol, 8.0f, 1.0f);

    // (Tumbler is now its own Component — see TumblerMeter — placed in resized())

    // Carbonated Audio logo — beneath the tumbler, small + soft
    if (carbonatedLogo.isValid())
    {
        const float logoH = 26.0f;
        const float aspect = (float) carbonatedLogo.getWidth() / (float) carbonatedLogo.getHeight();
        const float logoW = logoH * aspect;
        auto logoArea = juce::Rectangle<float> (
            getWidth() * 0.5f - logoW * 0.5f,
            getHeight() - 76.0f - logoH - 42.0f,  // higher above the QoL strip
            logoW, logoH);

        juce::Graphics::ScopedSaveState ss (g);
        g.setOpacity (0.85f);
        g.drawImage (carbonatedLogo, logoArea, juce::RectanglePlacement::centred);
    }

    // Clipping flash overlay
    if (clipFlashAlpha > 0.0f)
    {
        g.setColour (juce::Colour (TonicColors::clipping).withAlpha (clipFlashAlpha));
        g.fillRoundedRectangle (chassis.reduced (4.0f), 12.0f);
    }
}

void TonicEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16.0f);

    // Top bar (62px reserved by paint())
    bounds.removeFromTop (62);

    // Bottom QoL strip (76px reserved by paint())
    auto qol = bounds.removeFromBottom (76).reduced (16.0f, 12.0f);

    // Top right corner of the top bar — bypass + preset
    bypass.setBounds (getWidth() - 80, 28, 56, 26);

    // ACTIVATE button — only shown if not licensed
    activateButton.setBounds (getWidth() - 170, 28, 80, 26);

    // Activation dialog covers the full editor — its paint() draws the modal
    // overlay across the whole plugin and centers a 420x370 card inside.
    activationDialog.setBounds (getLocalBounds());

    // Main knob row
    auto knobRow = bounds.removeFromTop (200);
    const int knobsAreaW = knobRow.getWidth() - 200; // leave space for tubes left/right
    auto knobsArea = knobRow.withSizeKeepingCentre (knobsAreaW, knobRow.getHeight());
    const int knobW = knobsArea.getWidth() / 5;
    const int knobSize = 78;

    auto layoutKnob = [&] (juce::Slider& s, juce::Label& l, juce::Label& v, juce::Rectangle<int> col) {
        l.setBounds (col.removeFromTop (16));
        v.setBounds (col.removeFromBottom (18));
        auto inner = col.withSizeKeepingCentre (knobSize, knobSize);
        s.setBounds (inner);
    };

    auto col0 = knobsArea.removeFromLeft (knobW); layoutKnob (threshold, thresholdL, thresholdV, col0);
    auto col1 = knobsArea.removeFromLeft (knobW); layoutKnob (ratio,     ratioL,     ratioV,     col1);
    auto col2 = knobsArea.removeFromLeft (knobW); layoutKnob (attack,    attackL,    attackV,    col2);
    auto col3 = knobsArea.removeFromLeft (knobW); layoutKnob (release,   releaseL,   releaseV,   col3);
    auto col4 = knobsArea;                        layoutKnob (gain,      gainL,      gainV,      col4);

    // Middle row — toggles flank the central tumbler area
    auto midRow = bounds.removeFromTop (110);
    const int toggleColW = 90;
    auto midLeft  = midRow.removeFromLeft (toggleColW + 60);
    auto midRight = midRow.removeFromRight (toggleColW + 60);

    auto positionRocker = [] (RockerSwitch& rs, juce::Label& lbl, juce::Rectangle<int> col) {
        col = col.withSizeKeepingCentre (78, 70);
        lbl.setBounds (col.removeFromTop (14));
        rs.setBounds (col.reduced (4, 2));
    };
    positionRocker (timeMode,    timeModeL, midLeft.withSizeKeepingCentre (toggleColW, 90));
    positionRocker (meterSource, meterL,    midRight.withSizeKeepingCentre (toggleColW, 90));

    // Tumbler hero meter — central area between the two toggle columns
    const int tumblerW = 90;
    const int tumblerH = 130;
    tumbler.setBounds (juce::Rectangle<int> (0, 0, tumblerW, tumblerH)
                          .withCentre ({ getWidth() / 2,
                                          midRow.getY() + midRow.getHeight() / 2 + 16 }));

    // QoL strip layout — small knobs + toggles in a row
    const int qolColW = qol.getWidth() / 6;
    const int smallKnob = 48;

    auto layoutSmall = [&] (juce::Slider& s, juce::Label& l, juce::Label& v, juce::Rectangle<int> col) {
        l.setBounds (col.removeFromTop (12));
        v.setBounds (col.removeFromBottom (14));
        s.setBounds (col.withSizeKeepingCentre (smallKnob, smallKnob));
    };

    auto qcol0 = qol.removeFromLeft (qolColW); layoutSmall (mix, mixL, mixV, qcol0);
    auto qcol1 = qol.removeFromLeft (qolColW); positionRocker (stereoMode,   stereoL, qcol1);
    auto qcol2 = qol.removeFromLeft (qolColW); layoutSmall (scHpf, scHpfL, scHpfV, qcol2);
    auto qcol3 = qol.removeFromLeft (qolColW); layoutSmall (drive, driveL, driveV, qcol3);
    auto qcol4 = qol.removeFromLeft (qolColW); positionRocker (oversampling, osL,     qcol4);
    auto qcol5 = qol;                          positionRocker (externalSc,   scL,     qcol5);
}
