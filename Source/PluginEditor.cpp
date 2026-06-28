#include "PluginEditor.h"

OpenSynth1AudioProcessorEditor::OpenSynth1AudioProcessorEditor(OpenSynth1AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(800, 500);
}

OpenSynth1AudioProcessorEditor::~OpenSynth1AudioProcessorEditor() {}

void OpenSynth1AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("OpenSynth1", getLocalBounds(), juce::Justification::centred);
}

void OpenSynth1AudioProcessorEditor::resized() {}
