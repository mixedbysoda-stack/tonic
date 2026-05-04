#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "../../Source/PluginProcessor.h"

// Tonic v2.0 — WebView editor.
// All 14 plugin parameters bound to the React UI via JUCE 8's WebSliderRelay /
// WebComboBoxRelay / WebToggleButtonRelay system. A 30Hz Timer pushes meter
// data (input/output/GR/clipping) to the UI as a `tonicMeters` event so the
// React side can drive audio-reactive visuals (particles, halos, scanlines).
//
// Sizing: free-form resize was unreliable across hosts (Standalone bypasses
// the JUCE bounds constrainer entirely, leading to off-aspect windows that
// letterboxed the GUI). Replaced with 4 discrete scale presets — Large 100%,
// Medium 75%, Small 50%, Mini 35% — exposed via the `setEditorSize` native
// function. The chosen size is stored on the APVTS ValueTree as `editorW`/
// `editorH` props so it persists across sessions.
class WebViewEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit WebViewEditor (TonicProcessor&);
    ~WebViewEditor() override;

    void resized() override;

    // Native function handlers (JS → C++). Public so the registration
    // lambdas in buildWebViewOptions can dispatch into them.
    juce::var handleListPresets   (const juce::Array<juce::var>& args);
    juce::var handleLoadPreset    (const juce::Array<juce::var>& args);
    juce::var handleSavePreset    (const juce::Array<juce::var>& args);
    juce::var handleDeletePreset  (const juce::Array<juce::var>& args);
    juce::var handleSetEditorSize   (const juce::Array<juce::var>& args);
    juce::var handleGetEditorSize   (const juce::Array<juce::var>& args);
    juce::var handleGetLicenseState (const juce::Array<juce::var>& args);
    juce::var handleOpenExternalUrl (const juce::Array<juce::var>& args);

    // Async — returns a Promise on the JS side. The native function
    // registration captures the deferred completion and resolves it from
    // LicenseManager::tryActivate's network callback.
    void beginActivate (const juce::String& key,
                        std::function<void (bool, const juce::String&)> done);

    void emitLicenseState();

private:
    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> serveResource (const juce::String& url);
    void persistEditorSize();
    void emitPresetState();
    void emitSizeState();

    // Snap any width to the nearest discrete preset (Large/Medium/Small/Mini).
    static int snapToPresetWidth (int width);

    TonicProcessor& tonicProcessor;

    // ------------------- Slider relays (8 float params) -------------------
    juce::WebSliderRelay thresholdRelay { "threshold" };
    juce::WebSliderRelay ratioRelay     { "ratio" };
    juce::WebSliderRelay attackRelay    { "attack" };
    juce::WebSliderRelay releaseRelay   { "release" };
    juce::WebSliderRelay gainRelay      { "gain" };
    juce::WebSliderRelay mixRelay       { "mix" };
    juce::WebSliderRelay scHpfRelay     { "scHpf" };
    juce::WebSliderRelay driveRelay     { "drive" };

    // ------------------- Combo relays (4 choice params) -------------------
    juce::WebComboBoxRelay timeModeRelay     { "timeMode" };
    juce::WebComboBoxRelay meterSourceRelay  { "meterSource" };
    juce::WebComboBoxRelay stereoModeRelay   { "stereoMode" };
    juce::WebComboBoxRelay oversamplingRelay { "oversampling" };

    // ------------------- Toggle relays (2 bool params) -------------------
    juce::WebToggleButtonRelay externalScRelay { "externalSc" };
    juce::WebToggleButtonRelay bypassRelay     { "bypass" };

    // ------------------- Parameter attachments -------------------
    juce::WebSliderParameterAttachment thresholdAttachment;
    juce::WebSliderParameterAttachment ratioAttachment;
    juce::WebSliderParameterAttachment attackAttachment;
    juce::WebSliderParameterAttachment releaseAttachment;
    juce::WebSliderParameterAttachment gainAttachment;
    juce::WebSliderParameterAttachment mixAttachment;
    juce::WebSliderParameterAttachment scHpfAttachment;
    juce::WebSliderParameterAttachment driveAttachment;

    juce::WebComboBoxParameterAttachment timeModeAttachment;
    juce::WebComboBoxParameterAttachment meterSourceAttachment;
    juce::WebComboBoxParameterAttachment stereoModeAttachment;
    juce::WebComboBoxParameterAttachment oversamplingAttachment;

    juce::WebToggleButtonParameterAttachment externalScAttachment;
    juce::WebToggleButtonParameterAttachment bypassAttachment;

    // ------------------- Webview itself -------------------
    juce::WebBrowserComponent webView;

    // The activation gate is rendered IN the webview (React component
    // ActivationOverlay). C++ exposes LicenseManager via the getLicenseState
    // / activate native functions and the `tonicLicense` event — no JUCE
    // dialog needed, so no z-order workaround.

    bool lastActivated_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebViewEditor)
};
