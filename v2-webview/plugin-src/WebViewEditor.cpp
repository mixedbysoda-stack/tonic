#include "WebViewEditor.h"

// Webview UI lives in its own BinaryData namespace (TonicWebUI) so it doesn't
// collide with TonicResources at link time.
#include <TonicWebUIBinary.h>

namespace
{
    // Discrete editor scale presets. The chassis is laid out at exactly the
    // Large size, and the React side scales its transform from there. We
    // expose these on the JS bridge as { name, width, height } objects.
    struct SizePreset { const char* name; int width; int height; };
    constexpr SizePreset kSizePresets[] = {
        { "Large",  1080, 640 },
        { "Medium",  810, 480 },
        { "Small",   540, 320 },
        { "Mini",    378, 224 },
    };
    constexpr int kBaseWidth  = 1080;
    constexpr int kBaseHeight = 640;

    // Helper: bind one APVTS param to a relay by string ID.
    juce::RangedAudioParameter& param (TonicProcessor& p, const juce::String& id)
    {
        auto* prm = p.getAPVTS().getParameter (id);
        jassert (prm != nullptr);
        return *prm;
    }
}

int WebViewEditor::snapToPresetWidth (int width)
{
    int best = kSizePresets[0].width;
    int bestDiff = std::abs (width - best);
    for (const auto& p : kSizePresets)
    {
        const int d = std::abs (width - p.width);
        if (d < bestDiff) { best = p.width; bestDiff = d; }
    }
    return best;
}

// Build the WebBrowserComponent::Options once with all 14 relays attached,
// using only platform-appropriate backend settings.
//
// Native functions registered:
//   listPresets    () → { presets, current, dirty }
//   loadPreset     (name) → bool
//   savePreset     (name) → bool
//   deletePreset   (name) → bool
//   setEditorSize  (width, height) → bool
//   getEditorSize  () → { width, height, sizes }
static juce::WebBrowserComponent::Options buildWebViewOptions (
    juce::WebBrowserComponent::ResourceProvider provider,
    juce::WebSliderRelay& threshold, juce::WebSliderRelay& ratio,
    juce::WebSliderRelay& attack, juce::WebSliderRelay& release,
    juce::WebSliderRelay& gain, juce::WebSliderRelay& mix,
    juce::WebSliderRelay& scHpf, juce::WebSliderRelay& drive,
    juce::WebComboBoxRelay& timeMode, juce::WebComboBoxRelay& meterSource,
    juce::WebComboBoxRelay& stereoMode, juce::WebComboBoxRelay& oversampling,
    juce::WebToggleButtonRelay& externalSc, juce::WebToggleButtonRelay& bypass,
    WebViewEditor& editor)
{
    juce::WebBrowserComponent::Options opts;

   #if JUCE_WINDOWS
    opts = opts
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}
                                     .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)));
   #endif

    using NF = juce::WebBrowserComponent::NativeFunctionCompletion;
    using Args = const juce::Array<juce::var>&;

    return opts.withNativeIntegrationEnabled()
               .withResourceProvider (std::move (provider))
               .withOptionsFrom (threshold)
               .withOptionsFrom (ratio)
               .withOptionsFrom (attack)
               .withOptionsFrom (release)
               .withOptionsFrom (gain)
               .withOptionsFrom (mix)
               .withOptionsFrom (scHpf)
               .withOptionsFrom (drive)
               .withOptionsFrom (timeMode)
               .withOptionsFrom (meterSource)
               .withOptionsFrom (stereoMode)
               .withOptionsFrom (oversampling)
               .withOptionsFrom (externalSc)
               .withOptionsFrom (bypass)
               .withNativeFunction (juce::Identifier ("listPresets"),
                   [&editor] (Args args, NF complete) { complete (editor.handleListPresets (args)); })
               .withNativeFunction (juce::Identifier ("loadPreset"),
                   [&editor] (Args args, NF complete) { complete (editor.handleLoadPreset (args)); })
               .withNativeFunction (juce::Identifier ("savePreset"),
                   [&editor] (Args args, NF complete) { complete (editor.handleSavePreset (args)); })
               .withNativeFunction (juce::Identifier ("deletePreset"),
                   [&editor] (Args args, NF complete) { complete (editor.handleDeletePreset (args)); })
               .withNativeFunction (juce::Identifier ("setEditorSize"),
                   [&editor] (Args args, NF complete) { complete (editor.handleSetEditorSize (args)); })
               .withNativeFunction (juce::Identifier ("getEditorSize"),
                   [&editor] (Args args, NF complete) { complete (editor.handleGetEditorSize (args)); })
               .withNativeFunction (juce::Identifier ("getLicenseState"),
                   [&editor] (Args args, NF complete) { complete (editor.handleGetLicenseState (args)); })
               .withNativeFunction (juce::Identifier ("openExternalUrl"),
                   [&editor] (Args args, NF complete) { complete (editor.handleOpenExternalUrl (args)); })
               .withNativeFunction (juce::Identifier ("activate"),
                   [safe = juce::Component::SafePointer<WebViewEditor> (&editor)]
                   (Args args, NF complete)
                   {
                       if (safe == nullptr) { complete ({}); return; }
                       const auto key = args.size() > 0 ? args[0].toString() : juce::String();
                       safe->beginActivate (key,
                           [complete, safe] (bool success, const juce::String& message)
                           {
                               juce::DynamicObject::Ptr o = new juce::DynamicObject();
                               o->setProperty ("success", success);
                               o->setProperty ("message", message);
                               complete (juce::var (o.get()));
                               if (safe != nullptr) safe->emitLicenseState();
                           });
                   });
}

WebViewEditor::WebViewEditor (TonicProcessor& p)
    : juce::AudioProcessorEditor (&p),
      tonicProcessor (p),
      thresholdAttachment    (param (p, "threshold"),    thresholdRelay,    nullptr),
      ratioAttachment        (param (p, "ratio"),        ratioRelay,        nullptr),
      attackAttachment       (param (p, "attack"),       attackRelay,       nullptr),
      releaseAttachment      (param (p, "release"),      releaseRelay,      nullptr),
      gainAttachment         (param (p, "gain"),         gainRelay,         nullptr),
      mixAttachment          (param (p, "mix"),          mixRelay,          nullptr),
      scHpfAttachment        (param (p, "scHpf"),        scHpfRelay,        nullptr),
      driveAttachment        (param (p, "drive"),        driveRelay,        nullptr),
      timeModeAttachment     (param (p, "timeMode"),     timeModeRelay,     nullptr),
      meterSourceAttachment  (param (p, "meterSource"),  meterSourceRelay,  nullptr),
      stereoModeAttachment   (param (p, "stereoMode"),   stereoModeRelay,   nullptr),
      oversamplingAttachment (param (p, "oversampling"), oversamplingRelay, nullptr),
      externalScAttachment   (param (p, "externalSc"),   externalScRelay,   nullptr),
      bypassAttachment       (param (p, "bypass"),       bypassRelay,       nullptr),
      webView (buildWebViewOptions (
                   [this] (const juce::String& url) { return serveResource (url); },
                   thresholdRelay, ratioRelay, attackRelay, releaseRelay,
                   gainRelay, mixRelay, scHpfRelay, driveRelay,
                   timeModeRelay, meterSourceRelay, stereoModeRelay, oversamplingRelay,
                   externalScRelay, bypassRelay, *this))
{
    addAndMakeVisible (webView);

    lastActivated_ = tonicProcessor.getLicenseManager().isActivated();

    // Discrete sizing only — no draggable corner. Size is chosen by the
    // user from the React SizePicker which calls back through setEditorSize.
    setResizable (false, false);

    // Restore the saved size, snapping to the nearest preset so any legacy
    // off-aspect persisted size lands on a valid one.
    auto state = tonicProcessor.getAPVTS().state;
    const int savedW = (int) state.getProperty ("editorW", kBaseWidth);
    const int snappedW = snapToPresetWidth (savedW);
    int snappedH = kBaseHeight;
    for (const auto& sp : kSizePresets)
        if (sp.width == snappedW) { snappedH = sp.height; break; }

    setSize (snappedW, snappedH);

    // Boot the React app — JUCE's resource provider intercepts this URL and
    // returns the bundled index.html.
    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    startTimerHz (30);

    juce::Timer::callAfterDelay (250, [this]() {
        emitPresetState();
        emitSizeState();
        emitLicenseState();
    });
}

WebViewEditor::~WebViewEditor()
{
    stopTimer();
}

void WebViewEditor::resized()
{
    webView.setBounds (getLocalBounds());
    persistEditorSize();
}

void WebViewEditor::persistEditorSize()
{
    auto state = tonicProcessor.getAPVTS().state;
    state.setProperty ("editorW", getWidth(),  nullptr);
    state.setProperty ("editorH", getHeight(), nullptr);
}

void WebViewEditor::timerCallback()
{
    // Detect license-state changes (activation completing, deactivation by
    // an external tool, etc.) and push them out as a tonicLicense event so
    // the React overlay can show/hide itself.
    const bool nowActivated = tonicProcessor.getLicenseManager().isActivated();
    if (nowActivated != lastActivated_)
    {
        lastActivated_ = nowActivated;
        emitLicenseState();
    }

    auto& engine = tonicProcessor.getEngine();

    juce::DynamicObject::Ptr payload = new juce::DynamicObject();
    payload->setProperty ("input",            (double) juce::jlimit (0.0f, 1.0f, engine.getInputLevel()));
    payload->setProperty ("output",           (double) juce::jlimit (0.0f, 1.0f, engine.getOutputLevel()));
    payload->setProperty ("inputL",           (double) juce::jlimit (0.0f, 1.0f, engine.getInputLevelL()));
    payload->setProperty ("inputR",           (double) juce::jlimit (0.0f, 1.0f, engine.getInputLevelR()));
    payload->setProperty ("outputL",          (double) juce::jlimit (0.0f, 1.0f, engine.getOutputLevelL()));
    payload->setProperty ("outputR",          (double) juce::jlimit (0.0f, 1.0f, engine.getOutputLevelR()));
    payload->setProperty ("gainReductionDb",  (double) engine.getGainReduction());
    payload->setProperty ("clipping",         engine.getClipping());

    webView.emitEventIfBrowserIsVisible (juce::Identifier ("tonicMeters"), juce::var (payload.get()));
}

std::optional<juce::WebBrowserComponent::Resource>
WebViewEditor::serveResource (const juce::String& url)
{
    auto path = url;
    if (path.isEmpty() || path == "/" || path.endsWithIgnoreCase ("/index.html") || path == "index.html")
    {
        int dataSize = 0;
        const char* data = TonicWebUI::getNamedResource ("index_html", dataSize);
        if (data == nullptr || dataSize <= 0) return std::nullopt;

        juce::WebBrowserComponent::Resource resource;
        resource.data.assign (
            reinterpret_cast<const std::byte*> (data),
            reinterpret_cast<const std::byte*> (data) + dataSize);
        resource.mimeType = "text/html";
        return resource;
    }
    return std::nullopt;
}

// =====================================================================
// Native function handlers (JS → C++)
// =====================================================================

juce::var WebViewEditor::handleListPresets (const juce::Array<juce::var>& /*args*/)
{
    auto& mgr = tonicProcessor.getPresetManager();

    juce::var presetArray;
    for (const auto& p : mgr.getAll())
    {
        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        o->setProperty ("name",   p.name);
        o->setProperty ("source", p.source == PresetManager::Source::Factory ? "factory" : "user");
        presetArray.append (juce::var (o.get()));
    }

    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty ("presets", presetArray);
    root->setProperty ("current", mgr.getCurrentName());
    root->setProperty ("dirty",   mgr.isDirty());
    return juce::var (root.get());
}

juce::var WebViewEditor::handleLoadPreset (const juce::Array<juce::var>& args)
{
    if (args.size() < 1) return juce::var (false);
    const auto name = args[0].toString();
    const bool ok = tonicProcessor.getPresetManager().load (name);
    if (ok) emitPresetState();
    return juce::var (ok);
}

juce::var WebViewEditor::handleSavePreset (const juce::Array<juce::var>& args)
{
    if (args.size() < 1) return juce::var (false);
    const auto name = args[0].toString();
    const bool ok = tonicProcessor.getPresetManager().save (name);
    if (ok) emitPresetState();
    return juce::var (ok);
}

juce::var WebViewEditor::handleDeletePreset (const juce::Array<juce::var>& args)
{
    if (args.size() < 1) return juce::var (false);
    const auto name = args[0].toString();
    const bool ok = tonicProcessor.getPresetManager().deleteUser (name);
    if (ok) emitPresetState();
    return juce::var (ok);
}

juce::var WebViewEditor::handleSetEditorSize (const juce::Array<juce::var>& args)
{
    if (args.size() < 2) return juce::var (false);
    const int reqW = (int) args[0];
    const int reqH = (int) args[1];

    // Validate against the discrete preset list — silently snap to closest.
    int targetW = snapToPresetWidth (reqW);
    int targetH = kBaseHeight;
    for (const auto& sp : kSizePresets)
        if (sp.width == targetW) { targetH = sp.height; break; }

    // Ignore stray repeated requests at the same size.
    if (getWidth() == targetW && getHeight() == targetH)
        return juce::var (true);

    juce::ignoreUnused (reqH);
    setSize (targetW, targetH);
    emitSizeState();
    return juce::var (true);
}

juce::var WebViewEditor::handleGetEditorSize (const juce::Array<juce::var>& /*args*/)
{
    juce::var sizes;
    for (const auto& sp : kSizePresets)
    {
        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        o->setProperty ("name",   juce::String (sp.name));
        o->setProperty ("width",  sp.width);
        o->setProperty ("height", sp.height);
        sizes.append (juce::var (o.get()));
    }

    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty ("width",  getWidth());
    root->setProperty ("height", getHeight());
    root->setProperty ("sizes",  sizes);
    return juce::var (root.get());
}

void WebViewEditor::emitPresetState()
{
    webView.emitEventIfBrowserIsVisible (juce::Identifier ("tonicPresets"),
                                          handleListPresets ({}));
}

juce::var WebViewEditor::handleGetLicenseState (const juce::Array<juce::var>& /*args*/)
{
    auto& mgr = tonicProcessor.getLicenseManager();
    juce::DynamicObject::Ptr o = new juce::DynamicObject();
    o->setProperty ("activated", mgr.isActivated());
    o->setProperty ("machineID", mgr.getMachineID());
   #if DEMO_BUILD
    o->setProperty ("isDemo", true);
   #else
    o->setProperty ("isDemo", false);
   #endif
    return juce::var (o.get());
}

void WebViewEditor::emitLicenseState()
{
    webView.emitEventIfBrowserIsVisible (juce::Identifier ("tonicLicense"),
                                          handleGetLicenseState ({}));
}

void WebViewEditor::beginActivate (const juce::String& key,
                                    std::function<void (bool, const juce::String&)> done)
{
    tonicProcessor.getLicenseManager().tryActivate (key, std::move (done));
}

juce::var WebViewEditor::handleOpenExternalUrl (const juce::Array<juce::var>& args)
{
    // Hand the URL to the OS so it opens in the user's default browser /
    // mail client. Without this, an in-page <a> would either no-op (the
    // webview blocks new windows by default) or replace the React UI.
    if (args.size() < 1) return juce::var (false);
    const auto url = args[0].toString();
    if (url.isEmpty()) return juce::var (false);

    // Whitelist the schemes we expect (https, mailto). Belt-and-braces so
    // a hostile actor reaching invokeNative can't trigger arbitrary URL
    // handlers — though this is a closed plugin webview, not the open web.
    const auto lower = url.toLowerCase();
    if (! (lower.startsWith ("https://") || lower.startsWith ("http://") || lower.startsWith ("mailto:")))
        return juce::var (false);

    return juce::var (juce::URL (url).launchInDefaultBrowser());
}

void WebViewEditor::emitSizeState()
{
    webView.emitEventIfBrowserIsVisible (juce::Identifier ("tonicSize"),
                                          handleGetEditorSize ({}));
}
