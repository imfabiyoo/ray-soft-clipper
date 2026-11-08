cmake_minimum_required(VERSION 3.22)
project(ray VERSION 1.0.3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "" FORCE)
endif()

# ON for local dev (installs to the user plug-in folder); OFF in CI.
option(RAY_COPY_PLUGIN "Install plugin to the user plug-in folder after build" ON)

add_subdirectory(JUCE)

juce_add_plugin(ray
    COMPANY_NAME "fabiyoo"
    PLUGIN_MANUFACTURER_CODE Fbyo
    PLUGIN_CODE Ray1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "RayClipper"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${RAY_COPY_PLUGIN}
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect)

juce_generate_juce_header(ray)

target_sources(ray PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(ray PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

juce_add_binary_data(rayAssets SOURCES Assets/ray_ui.png)

target_link_libraries(ray PRIVATE
    rayAssets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
