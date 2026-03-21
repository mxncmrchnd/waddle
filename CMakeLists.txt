cmake_minimum_required(VERSION 3.22)

project(WADDLE VERSION 1.0.0)

set(JUCE_DIR "D:/JUCE" CACHE PATH "Path to JUCE")
add_subdirectory(${JUCE_DIR} JUCE)

set(VST3_COPY_DIR "D:/VST3")

juce_add_plugin(Waddle
    VERSION 1.0.0
    COMPANY_NAME "Maxence Marchand"
    BUNDLE_ID "com.maxencemarchand.waddle"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Wadd
    PLUGIN_CODE Wd10
    FORMATS VST3 Standalone
    PRODUCT_NAME "Waddle")

juce_add_binary_data(WaddleAssets
    HEADER_NAME BinaryData.h
    NAMESPACE BinaryData
    SOURCES
        assets/logo.png
        assets/Geoform.otf)

target_compile_options(WaddleAssets PRIVATE $<TARGET_PROPERTY:Waddle,COMPILE_OPTIONS>)
target_include_directories(WaddleAssets PRIVATE $<TARGET_PROPERTY:Waddle,INCLUDE_DIRECTORIES>)

target_sources(Waddle
    PRIVATE
        src/PluginEditor.cpp
        src/PluginProcessor.cpp)

target_compile_definitions(Waddle
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Waddle
    PRIVATE
    WaddleAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
