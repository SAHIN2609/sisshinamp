cmake_minimum_required(VERSION 3.22)
project(SishhinDSP VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SDSP_BUILD_PLUGIN "Build JUCE VST3 and standalone" ON)
option(SDSP_BUILD_TESTS "Build portable DSP tests" ON)

add_library(sishhin_dsp STATIC Source/AmpEngine.cpp Source/AmpEngine.h)
target_compile_features(sishhin_dsp PUBLIC cxx_std_17)
if (MSVC)
    target_compile_options(sishhin_dsp PRIVATE /W4)
else()
    target_compile_options(sishhin_dsp PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(SDSP_BUILD_TESTS)
    enable_testing()
    add_executable(sdsp_test tests/DSPTests.cpp)
    target_link_libraries(sdsp_test PRIVATE sishhin_dsp)
    add_test(NAME DSPModels COMMAND sdsp_test)
endif()

if(SDSP_BUILD_PLUGIN)
    include(FetchContent)
    # A pinned, released JUCE version; no Visual Studio 2022 hard-coded generator.
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.16-security GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(SishhinDSP
        COMPANY_NAME "SishhinDSP"
        BUNDLE_ID "com.sishhin.dsp"
        PLUGIN_MANUFACTURER_CODE Ssdp
        PLUGIN_CODE ShDn
        PRODUCT_NAME "SishhinDSP"
        FORMATS VST3 Standalone
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE
        NEEDS_WEBVIEW2 TRUE
    )
    juce_generate_juce_header(SishhinDSP)
    juce_add_binary_data(SishhinAssets SOURCES Assets/sishhin_logo.png Assets/interface.html Assets/bridge.js)
    target_sources(SishhinDSP PRIVATE
        Source/PluginProcessor.h Source/PluginProcessor.cpp
        Source/PluginEditor.h Source/PluginEditor.cpp
    )
    target_include_directories(SishhinDSP PRIVATE Source)
    target_compile_definitions(SishhinDSP PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )
    target_link_libraries(SishhinDSP PRIVATE
        sishhin_dsp SishhinAssets
        juce::juce_audio_utils juce::juce_audio_processors
        juce::juce_gui_extra juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )
endif()
