cmake_minimum_required(VERSION 3.22)

# Must be set BEFORE project() so the toolchain picks it up on first configure
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(Tonic VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(COPY_AFTER_BUILD "Copy plugins to system folders after build" ON)
option(BUILD_DEMO "Build demo version (always runs in trial mode)" OFF)
option(BUILD_MUSEHUB "Build MuseHub-distribution variant: strip custom activation UI, skip license gate (MuseDRM handles auth)" OFF)
option(BUILD_WEBVIEW "v2.0 spike: replace JUCE GUI with embedded WebView + React app" OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_subdirectory(JUCE)

# Bundled image resources (Carbonated Audio logo, etc.)
juce_add_binary_data(TonicResources
    SOURCES
        Resources/CarbonatedLogo.png
)

# v2.0 webview UI (only built when BUILD_WEBVIEW=ON)
if(BUILD_WEBVIEW)
    set(WEBVIEW_DIST "${CMAKE_CURRENT_SOURCE_DIR}/v2-webview/webview-ui/dist/index.html")
    if(NOT EXISTS "${WEBVIEW_DIST}")
        message(FATAL_ERROR "BUILD_WEBVIEW is ON but webview UI hasn't been built yet.\n"
                            "Run: cd v2-webview/webview-ui && npm install && npm run build\n"
                            "Expected: ${WEBVIEW_DIST}")
    endif()
    juce_add_binary_data(TonicWebViewUI
        NAMESPACE TonicWebUI
        HEADER_NAME TonicWebUIBinary.h
        SOURCES
            ${WEBVIEW_DIST}
    )
endif()

if(APPLE)
    set(AAX_SDK_PATH "/Users/soda/Documents/aax-sdk-2-9-0")
    set(FORMATS VST3 AU AAX Standalone)
elseif(WIN32)
    set(FORMATS VST3 Standalone)
else()
    set(FORMATS VST3 Standalone)
endif()

set(SOURCE_FILES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PresetManager.cpp
    Source/DSP/CompressorEngine.cpp
    Source/UI/TonicLookAndFeel.cpp
    Source/UI/TumblerMeter.cpp
    Source/UI/RockerSwitch.cpp
    Source/Licensing/LicenseManager.cpp
    Source/Licensing/ActivationDialog.cpp
)

if(BUILD_WEBVIEW)
    list(APPEND SOURCE_FILES v2-webview/plugin-src/WebViewEditor.cpp)
endif()

juce_add_plugin(Tonic
    COMPANY_NAME "Carbonated Audio"
    PLUGIN_MANUFACTURER_CODE CbAu
    PLUGIN_CODE Tonc
    FORMATS ${FORMATS}
    PRODUCT_NAME "Tonic"
    BUNDLE_ID "com.carbonatedaudio.tonic"

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    # WebView2 is required by JUCE's WebBrowserComponent on Windows. JUCE
    # downloads the SDK from NuGet during build when this is TRUE, which is
    # exactly what we need for the cyberpunk webview UI on Windows. Macs
    # use WKWebView and ignore this flag.
    NEEDS_WEBVIEW2 TRUE

    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD}

    VST3_CATEGORIES Fx Dynamics
    AU_MAIN_TYPE kAudioUnitType_Effect
    AAX_CATEGORY AAX_ePlugInCategory_Dynamics
)

target_sources(Tonic PRIVATE ${SOURCE_FILES})

if(BUILD_WEBVIEW)
    # Shipping webview build — demo gate is active when not activated, just
    # like the v1 plugin. To bypass the demo gate during development pass
    # -DTONIC_DEV_BUILD=ON on the cmake configure line.
    target_compile_definitions(Tonic
        PUBLIC
            JUCE_WEB_BROWSER=1
            JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JUCE_DISPLAY_SPLASH_SCREEN=0
            JUCE_REPORT_APP_USAGE=0
            TONIC_USE_WEBVIEW=1
    )
    if(TONIC_DEV_BUILD)
        target_compile_definitions(Tonic PUBLIC TONIC_DEV_BUILD=1)
    endif()
else()
    target_compile_definitions(Tonic
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JUCE_DISPLAY_SPLASH_SCREEN=0
            JUCE_REPORT_APP_USAGE=0
    )
endif()

if(BUILD_DEMO)
    target_compile_definitions(Tonic PRIVATE DEMO_BUILD=1)
endif()

if(BUILD_MUSEHUB)
    target_compile_definitions(Tonic PRIVATE TONIC_MUSEHUB=1)
endif()

if(APPLE AND AAX_SDK_PATH)
    target_compile_definitions(Tonic PUBLIC
        JucePlugin_AAXLibs_path="${AAX_SDK_PATH}/Libs"
    )
endif()

target_link_libraries(Tonic
    PRIVATE
        TonicResources
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_cryptography
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(BUILD_WEBVIEW)
    target_link_libraries(Tonic
        PRIVATE
            TonicWebViewUI
            juce::juce_gui_extra
    )
    target_include_directories(Tonic
        PRIVATE
            v2-webview/plugin-src
    )
endif()

target_include_directories(Tonic
    PRIVATE
        Source
)

if(APPLE)
    set_target_properties(Tonic PROPERTIES
        OSX_ARCHITECTURES "arm64;x86_64"
    )
endif()

if(MSVC)
    target_compile_options(Tonic PRIVATE /W4)
else()
    target_compile_options(Tonic PRIVATE -Wall -Wextra -Wpedantic)
endif()
