cmake_minimum_required(VERSION 3.22)

project(CHETO VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# En Windows: runtime estatico, asi el plugin no necesita instalar nada extra
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(CHETO
    COMPANY_NAME "Santino"
    PLUGIN_MANUFACTURER_CODE Sant
    PLUGIN_CODE Cht1
    FORMATS VST3 Standalone
    PRODUCT_NAME "CHETO"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx)

# El logo va metido dentro del plugin
juce_add_binary_data(CHETOAssets SOURCES Assets/logo.png)

target_sources(CHETO PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Autotune.h
    Source/EqDisplay.h
    Source/Theme.h
    Source/CHETOLookAndFeel.h)

target_compile_definitions(CHETO PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(CHETO
    PRIVATE
        CHETOAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
