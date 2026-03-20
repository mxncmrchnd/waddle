cmake_minimum_required(VERSION 3.22)

project(WADDLE VERSION 0.0.1)

set(JUCE_DIR "D:/JUCE" CACHE PATH "Path to JUCE")
add_subdirectory(${JUCE_DIR} JUCE)


juce_add_plugin(Waddle
    VERSION 0.0.1
    COMPANY_NAME "Maxence Marchand"
    BUNDLE_ID "com.maxencemarchand.waddle"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Wadd
    PLUGIN_CODE Wd10
    FORMATS VST3 Standalone
    PRODUCT_NAME "Waddle")

target_sources(Waddle
    PRIVATE
        PluginEditor.cpp
        PluginProcessor.cpp)

target_compile_definitions(Waddle
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Waddle
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
