#include "ActivationDialog.h"

ActivationDialog::ActivationDialog (LicenseManager& lm)
    : licenseManager (lm),
      buyLink  ("Buy Tonic",  juce::URL ("https://carbonatedaudio.com/tonic")),
      helpLink ("Need help?",     juce::URL ("mailto:support@carbonatedaudio.com"))
{
    setInterceptsMouseClicks (true, true);

#if DEMO_BUILD
    // --- Demo mode UI ---
    titleLabel.setText ("Tonic Demo", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (24.0f).boldened());
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd4d4d8));
    addAndMakeVisible (titleLabel);

    instructionLabel.setText ("Full-featured demo \xe2\x80\x94 audio mutes for 10s every 60s.\nPurchase to remove the limitation.",
                              juce::dontSendNotification);
    instructionLabel.setFont (juce::Font (13.0f));
    instructionLabel.setJustificationType (juce::Justification::centred);
    instructionLabel.setColour (juce::Label::textColourId, juce::Colour (0xffa1a1aa));
    addAndMakeVisible (instructionLabel);

    // Hide key input and activate button in demo mode
    keyInput.setVisible (false);
    activateButton.setVisible (false);

    // Status shows demo badge
    statusLabel.setText ("DEMO VERSION", juce::dontSendNotification);
    statusLabel.setFont (juce::Font (14.0f).boldened());
    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xfffbbf24)); // amber
    addAndMakeVisible (statusLabel);

    // "Try It" button dismisses the dialog
    activateButton.setButtonText ("Try It");
    activateButton.onClick = [this]() { setVisible (false); };
    activateButton.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff3b82f6));
    activateButton.setColour (juce::TextButton::textColourOnId,  juce::Colours::white);
    activateButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    activateButton.setVisible (true);
    addAndMakeVisible (activateButton);

    // Links
    buyLink.setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff3b82f6));
    buyLink.setFont (juce::Font (13.0f), false);
    addAndMakeVisible (buyLink);

    helpLink.setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff52525b));
    helpLink.setFont (juce::Font (13.0f), false);
    addAndMakeVisible (helpLink);

#else
    // --- Full version UI ---
    // Title
    titleLabel.setText ("Activate Tonic", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (24.0f).boldened());
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd4d4d8));
    addAndMakeVisible (titleLabel);

    // Instruction
    instructionLabel.setText ("Enter the serial number from your purchase confirmation email.",
                              juce::dontSendNotification);
    instructionLabel.setFont (juce::Font (13.0f));
    instructionLabel.setJustificationType (juce::Justification::centred);
    instructionLabel.setColour (juce::Label::textColourId, juce::Colour (0xffa1a1aa));
    addAndMakeVisible (instructionLabel);

    // Key input — monospace, paste-friendly, styled to match plugin aesthetic
    keyInput.setMultiLine (false);
    keyInput.setReturnKeyStartsNewLine (false);
    keyInput.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    keyInput.setTextToShowWhenEmpty ("XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX", juce::Colour (0xff52525b));
    keyInput.setJustification (juce::Justification::centred);
    keyInput.setColour (juce::TextEditor::backgroundColourId,      juce::Colour (0xff0a0a0a));
    keyInput.setColour (juce::TextEditor::outlineColourId,         juce::Colour (0xff3f3f46));
    keyInput.setColour (juce::TextEditor::textColourId,            juce::Colour (0xffef4444)); // LCD red
    keyInput.setColour (juce::TextEditor::focusedOutlineColourId,  juce::Colour (0xff3b82f6)); // blue focus
    keyInput.onReturnKey = [this]() { onActivateClicked(); };
    addAndMakeVisible (keyInput);

    // Activate button — blue accent to match threshold meter
    activateButton.setButtonText ("Activate");
    activateButton.onClick = [this]() { onActivateClicked(); };
    activateButton.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff3b82f6));
    activateButton.setColour (juce::TextButton::textColourOnId,  juce::Colours::white);
    activateButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (activateButton);

    // Status label
    statusLabel.setText ("", juce::dontSendNotification);
    statusLabel.setFont (juce::Font (13.0f));
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    // Links
    buyLink.setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff3b82f6));
    buyLink.setFont (juce::Font (13.0f), false);
    addAndMakeVisible (buyLink);

    helpLink.setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff52525b));
    helpLink.setFont (juce::Font (13.0f), false);
    addAndMakeVisible (helpLink);
#endif
}

void ActivationDialog::paint (juce::Graphics& g)
{
    // Dark overlay covering entire plugin
    g.fillAll (juce::Colour (0xee1a1b1e));

    // Centered card — gunmetal style matching plugin
    auto cardBounds = getLocalBounds().toFloat().withSizeKeepingCentre (420.0f, 370.0f);

    // Card shadow
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (cardBounds.translated (0.0f, 4.0f), 12.0f);

    // Card background
    g.setColour (juce::Colour (0xff2a2b2e));
    g.fillRoundedRectangle (cardBounds, 12.0f);

    // Card border
    g.setColour (juce::Colour (0xff52525b));
    g.drawRoundedRectangle (cardBounds, 12.0f, 1.0f);

    // Top highlight
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (int (cardBounds.getY() + 1),
                          cardBounds.getX() + 12, cardBounds.getRight() - 12);
}

void ActivationDialog::resized()
{
    auto cardBounds = getLocalBounds().withSizeKeepingCentre (420, 370);
    auto area = cardBounds.reduced (30);

    titleLabel.setBounds (area.removeFromTop (32));
    area.removeFromTop (8);

    instructionLabel.setBounds (area.removeFromTop (20));
    area.removeFromTop (20);

    keyInput.setBounds (area.removeFromTop (36));
    area.removeFromTop (16);

    activateButton.setBounds (area.removeFromTop (40).reduced (90, 0));
    area.removeFromTop (12);

    statusLabel.setBounds (area.removeFromTop (20));
    area.removeFromTop (16);

    auto linkArea = area.removeFromTop (20);
    auto halfW = linkArea.getWidth() / 2;
    buyLink.setBounds (linkArea.removeFromLeft (halfW));
    helpLink.setBounds (linkArea);
}

void ActivationDialog::visibilityChanged()
{
#if ! DEMO_BUILD
    if (isVisible())
        keyInput.grabKeyboardFocus();
#endif
}

void ActivationDialog::onActivateClicked()
{
    auto key = keyInput.getText().trim();

    if (key.isEmpty())
    {
        statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffef4444));
        statusLabel.setText ("Please enter your serial number", juce::dontSendNotification);
        return;
    }

    activateButton.setEnabled (false);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffa1a1aa));
    statusLabel.setText ("Activating...", juce::dontSendNotification);

    auto safeThis = juce::Component::SafePointer<ActivationDialog> (this);

    licenseManager.tryActivate (key, [safeThis] (bool success, const juce::String& message)
    {
        if (safeThis == nullptr)
            return;

        if (success)
        {
            safeThis->statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff10b981));
            safeThis->statusLabel.setText (message, juce::dontSendNotification);
        }
        else
        {
            safeThis->statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffef4444));
            safeThis->statusLabel.setText (message, juce::dontSendNotification);
            safeThis->activateButton.setEnabled (true);
        }
    });
}
