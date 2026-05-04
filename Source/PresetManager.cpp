#include "PresetManager.h"

namespace
{
    constexpr const char* kPresetExt = ".tonicpreset";
    constexpr const char* kPresetSubdir = "Carbonated Audio/Tonic/Presets";
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvts)
    : apvts_ (apvts)
{
    buildFactoryPresets();

    auto dir = getUserPresetDir();
    if (! dir.exists())
        dir.createDirectory();
}

juce::File PresetManager::getUserPresetDir()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
             .getChildFile (kPresetSubdir);
}

std::vector<PresetManager::Preset> PresetManager::getAll() const
{
    std::vector<Preset> out;
    out.reserve (factories_.size() + 16);

    for (const auto& f : factories_)
        out.push_back ({ f.name, Source::Factory });

    juce::Array<juce::File> userFiles;
    getUserPresetDir().findChildFiles (userFiles, juce::File::findFiles, false,
                                       juce::String ("*") + kPresetExt);
    userFiles.sort();
    for (auto& f : userFiles)
        out.push_back ({ f.getFileNameWithoutExtension(), Source::User });

    return out;
}

bool PresetManager::load (const juce::String& name)
{
    // Factory first
    for (const auto& f : factories_)
    {
        if (f.name == name)
        {
            auto xml = juce::parseXML (f.xml);
            if (xml == nullptr) return false;
            applyStateXml (*xml);
            currentName_ = name;
            dirty_ = false;
            return true;
        }
    }

    // User
    auto file = getUserPresetDir().getChildFile (name + kPresetExt);
    if (! file.existsAsFile()) return false;

    auto xml = juce::parseXML (file);
    if (xml == nullptr) return false;
    applyStateXml (*xml);
    currentName_ = name;
    dirty_ = false;
    return true;
}

bool PresetManager::save (const juce::String& name)
{
    auto trimmed = name.trim();
    if (trimmed.isEmpty()) return false;

    // Block overwrites of factory presets
    for (const auto& f : factories_)
        if (f.name.equalsIgnoreCase (trimmed)) return false;

    // Strip filesystem-unsafe characters
    auto safe = juce::File::createLegalFileName (trimmed);
    if (safe.isEmpty()) return false;

    auto xml = snapshotXml();
    if (xml == nullptr) return false;

    auto file = getUserPresetDir().getChildFile (safe + kPresetExt);
    if (! xml->writeTo (file)) return false;

    currentName_ = safe;
    dirty_ = false;
    return true;
}

bool PresetManager::deleteUser (const juce::String& name)
{
    // Factory presets are protected
    for (const auto& f : factories_)
        if (f.name == name) return false;

    auto file = getUserPresetDir().getChildFile (name + kPresetExt);
    if (! file.existsAsFile()) return false;
    if (! file.deleteFile()) return false;

    if (currentName_ == name)
    {
        currentName_ = {};
        dirty_ = true;
    }
    return true;
}

void PresetManager::markDirty()
{
    dirty_ = true;
}

void PresetManager::applyStateXml (const juce::XmlElement& xml)
{
    // Walk the PARAM children and call setValueNotifyingHost so DAW automation
    // sees the change too. We don't replaceState() because that bypasses the
    // host's parameter notification path.
    for (auto* paramXml : xml.getChildIterator())
    {
        if (! paramXml->hasTagName ("PARAM")) continue;

        auto id = paramXml->getStringAttribute ("id");
        auto* p = apvts_.getParameter (id);
        if (p == nullptr) continue;

        const float realValue = (float) paramXml->getDoubleAttribute ("value");
        const float norm = p->convertTo0to1 (realValue);
        p->setValueNotifyingHost (norm);
    }
}

std::unique_ptr<juce::XmlElement> PresetManager::snapshotXml() const
{
    // Snapshot only the parameter values — strip everything else (editor
    // size, internal metadata) so presets stay portable.
    auto xml = std::make_unique<juce::XmlElement> ("TonicPreset");
    for (auto* p : apvts_.processor.getParameters())
    {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            auto* paramXml = xml->createNewChildElement ("PARAM");
            paramXml->setAttribute ("id", rp->paramID);
            paramXml->setAttribute ("value", (double) rp->convertFrom0to1 (rp->getValue()));
        }
    }
    return xml;
}

juce::String PresetManager::makeFactoryXml (float threshold, float ratio, float attack,
                                             float release, float gain, int timeMode,
                                             float mix, int stereoMode, float scHpf,
                                             float drive, int oversampling) const
{
    juce::XmlElement xml ("TonicPreset");
    auto add = [&] (const char* id, double v)
    {
        auto* p = xml.createNewChildElement ("PARAM");
        p->setAttribute ("id", id);
        p->setAttribute ("value", v);
    };
    add ("threshold",    threshold);
    add ("ratio",        ratio);
    add ("attack",       attack);
    add ("release",      release);
    add ("gain",         gain);
    add ("timeMode",     timeMode);
    add ("mix",          mix);
    add ("stereoMode",   stereoMode);
    add ("scHpf",        scHpf);
    add ("drive",        drive);
    add ("oversampling", oversampling);
    add ("externalSc",   0.0);
    add ("bypass",       0.0);
    return xml.toString();
}

void PresetManager::buildFactoryPresets()
{
    // (threshold, ratio, attack, release, gain, timeMode 0=FIX 1=MAN 2=F+M,
    //  mix, stereoMode 0=ST 1=Dual 2=MS, scHpf, drive 0..10, OS 0=1x 1=2x 2=4x)

    factories_ = {
        // Bread-and-butter vocal — gentle, transparent, opto-style breathe
        { "Vocal Tonic",     makeFactoryXml (-18.0f, 4.0f, 15.0f,  220.0f, 3.0f, 0, 100.0f, 0, 80.0f,  0.5f, 1) },

        // Mix-bus glue — slow attack, low ratio, light drive for warmth
        { "Bus Glue",        makeFactoryXml (-12.0f, 2.0f, 30.0f,  400.0f, 1.5f, 0, 100.0f, 0, 30.0f,  0.8f, 1) },

        // Heavy pump — aggressive ratio, fast attack, slow release
        { "Heavy Pump",      makeFactoryXml (-22.0f, 8.0f,  3.0f, 1000.0f, 6.0f, 1, 100.0f, 0, 60.0f,  1.2f, 1) },

        // Slow & smooth — F+M, gentle ratio, long release for ballads
        { "Slow & Smooth",   makeFactoryXml (-16.0f, 3.0f, 25.0f,  650.0f, 2.5f, 2, 100.0f, 0, 100.0f, 0.3f, 1) },

        // Aggressive snap — fast attack on lead vox / rap
        { "Aggressive Snap", makeFactoryXml (-20.0f, 6.0f,  1.5f,  120.0f, 4.0f, 1, 100.0f, 0, 120.0f, 1.5f, 1) },

        // Drum bus — punchy, dual-mono, kicked-up SC HPF
        { "Drum Bus",        makeFactoryXml (-14.0f, 4.0f,  8.0f,  180.0f, 2.0f, 1,  85.0f, 1, 150.0f, 1.0f, 1) },

        // Bass tightener — controls low-end, M/S so transients survive
        { "Bass Tightener",  makeFactoryXml (-15.0f, 5.0f, 12.0f,  300.0f, 2.0f, 1, 100.0f, 2, 25.0f,  0.6f, 1) },

        // Mix bus polish — barely-there compression with parallel mix
        { "Mix Bus Polish",  makeFactoryXml (-10.0f, 1.8f, 40.0f,  500.0f, 0.5f, 0,  60.0f, 0, 40.0f,  0.4f, 2) },
    };
}
