#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_cryptography/juce_cryptography.h>
#include <atomic>
#include <functional>

class LicenseManager
{
public:
    LicenseManager();
    ~LicenseManager() = default;

#if DEMO_BUILD
    bool isActivated() const { return false; }
#elif defined(TONIC_MUSEHUB)
    // MuseHub distribution — MuseDRM handles authorization, custom license gate is bypassed.
    bool isActivated() const { return true; }
#else
    bool isActivated() const { return activated.load (std::memory_order_relaxed); }
#endif
    const std::atomic<bool>* getActivatedFlagPtr() const { return &activated; }

    void tryActivate (const juce::String& key,
                      std::function<void (bool success, const juce::String& message)> callback);

    juce::String getMachineID() const { return machineID; }

private:
    void loadStoredActivation();
    static juce::String computeMachineID();

    std::atomic<bool> activated { false };
    juce::String machineID;

    std::unique_ptr<juce::PropertiesFile> properties;
    juce::CriticalSection propertiesLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LicenseManager)
};
