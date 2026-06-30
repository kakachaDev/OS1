cmake_minimum_required(VERSION 3.22)
project(OpenSynth1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(_deps/JUCE)

if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
elseif(UNIX)
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

juce_add_plugin(OpenSynth1
    COMPANY_NAME            "OpenSynth1"
    PLUGIN_MANUFACTURER_CODE NPS0
    PLUGIN_CODE             OS01
    FORMATS                 ${PLUGIN_FORMATS}
    PRODUCT_NAME            "OpenSynth1"
    VST3_CATEGORIES         "Instrument" "Synth"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VERSION                 "0.1.0"
)

target_sources(OpenSynth1
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/dsp/Oscillator.cpp
        Source/dsp/SynthVoice.cpp
        Source/dsp/VoiceManager.cpp
)

target_compile_definitions(OpenSynth1
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(OpenSynth1
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ============================================================
# TEST TARGET
# ============================================================
juce_add_console_app(OpenSynth1Tests PRODUCT_NAME "OpenSynth1Tests")

target_sources(OpenSynth1Tests
    PRIVATE
        tests/TestMain.cpp
        tests/TestOscillator.cpp
        tests/TestSynthVoice.cpp
        tests/TestVoiceManager.cpp
        tests/TestEnvelope.cpp
        tests/TestFilter.cpp
        Source/dsp/Oscillator.cpp
        Source/dsp/SynthVoice.cpp
        Source/dsp/VoiceManager.cpp
        Source/dsp/Envelope.cpp
        Source/dsp/SynthFilter.cpp
)

target_compile_definitions(OpenSynth1Tests
    PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
)

target_link_libraries(OpenSynth1Tests
    PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
