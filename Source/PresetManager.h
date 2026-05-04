#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Tonic preset manager.
//
// Owns 8 baked-in factory presets and an open set of user presets stored as
// XML files at:
//   ~/Documents/Carbonated Audio/Tonic/Presets/<name>.tonicpreset
//
// Presets snapshot the APVTS state — every parameter except editor size and
// licensing. Loading a preset restores those parameter values via the normal
// setValueNotifyingHost() path so DAW automation lanes update too.
class PresetManager
{
public:
    enum class Source { Factory, User };

    struct Preset
    {
        juce::String name;
        Source       source;
    };

    PresetManager (juce::AudioProcessorValueTreeState& apvts);

    // List of all presets (factory first, then user, alphabetical within group).
    std::vector<Preset> getAll() const;

    // Currently loaded preset name. Empty when state has been edited since the
    // last load/save (i.e. user has touched a knob).
    juce::String getCurrentName() const { return currentName_; }

    // Load by exact name (case-sensitive). Returns true on success.
    bool load (const juce::String& name);

    // Save current APVTS state as a user preset. If a preset with that name
    // already exists, it's overwritten. Returns true on success.
    bool save (const juce::String& name);

    // Delete a user preset by name. Factory presets are protected — returns
    // false if attempted. Returns true on success.
    bool deleteUser (const juce::String& name);

    // Mark current state as "dirty" — call from a parameter listener so the
    // UI can show the user has modified the loaded preset.
    void markDirty();

    bool isDirty() const { return dirty_; }

    // Where user presets live on disk.
    static juce::File getUserPresetDir();

private:
    juce::AudioProcessorValueTreeState& apvts_;
    juce::String currentName_;
    bool dirty_ = false;

    // Apply an APVTS XML element to the live parameters via host notifications
    void applyStateXml (const juce::XmlElement& xml);

    // Snapshot current APVTS state to an XML element (caller owns)
    std::unique_ptr<juce::XmlElement> snapshotXml() const;

    // Factory preset XML strings — built once on construction
    struct FactoryPreset { juce::String name; juce::String xml; };
    std::vector<FactoryPreset> factories_;

    void buildFactoryPresets();
    juce::String makeFactoryXml (float threshold, float ratio, float attack,
                                 float release, float gain, int timeMode,
                                 float mix, int stereoMode, float scHpf,
                                 float drive, int oversampling) const;
};
