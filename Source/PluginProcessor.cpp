#include "PluginProcessor.h"
#include "PluginEditor.h"

OpenSynth1AudioProcessor::OpenSynth1AudioProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

OpenSynth1AudioProcessor::~OpenSynth1AudioProcessor() {}

void OpenSynth1AudioProcessor::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/) {}

void OpenSynth1AudioProcessor::releaseResources() {}

void OpenSynth1AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midiMessages*/)
{
    buffer.clear();
}

juce::AudioProcessorEditor* OpenSynth1AudioProcessor::createEditor()
{
    return new OpenSynth1AudioProcessorEditor(*this);
}

void OpenSynth1AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void OpenSynth1AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout
OpenSynth1AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"master_volume", 1},
        "Master Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.8f));
    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OpenSynth1AudioProcessor();
}
