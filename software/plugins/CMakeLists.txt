cmake_minimum_required(VERSION 3.15)

project(OpenSamplerPlugins VERSION 0.1.0)

# Set JUCE path (update this to your JUCE location)
set(JUCE_DIR "/path/to/JUCE")

# Add JUCE
add_subdirectory(${JUCE_DIR} JUCE)

# Create plugin target
juce_add_plugin(OpenSamplerPlugin
    COMPANY_NAME "YourCompany"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE OSP1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "OpenSamplerPlugin"
)

target_sources(OpenSamplerPlugin PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h
)

target_link_libraries(OpenSamplerPlugin PRIVATE juce::juce_audio_processors juce::juce_audio_utils)