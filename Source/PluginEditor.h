#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class OpenSynth1AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OpenSynth1AudioProcessorEditor(OpenSynth1AudioProcessor&);
    ~OpenSynth1AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OpenSynth1AudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenSynth1AudioProcessorEditor)
};
