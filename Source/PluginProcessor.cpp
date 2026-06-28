#include "PluginProcessor.h"
#include "PluginEditor.h"

OpenSynth1AudioProcessor::OpenSynth1AudioProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

OpenSynth1AudioProcessor::~OpenSynth1AudioProcessor() {}

void OpenSynth1AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    voiceManager.prepare(sampleRate, samplesPerBlock);
    updateVoiceManagerFromParams();
}

void OpenSynth1AudioProcessor::releaseResources()
{
    voiceManager.allNotesOff();
}

void OpenSynth1AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    updateVoiceManagerFromParams();
    voiceManager.processBlock(midiMessages, buffer, buffer.getNumSamples());

    // Apply master volume
    float masterVol = apvts.getRawParameterValue("master_volume")->load();
    buffer.applyGain(masterVol);
}

void OpenSynth1AudioProcessor::updateVoiceManagerFromParams() noexcept
{
    VoicePatch patch;
    // Waveform indices: 0=Sine, 1=Triangle, 2=Saw, 3=Square
    auto waveFromParam = [](float v) -> Oscillator::Waveform {
        int i = static_cast<int>(v);
        return static_cast<Oscillator::Waveform>(juce::jlimit(0, 3, i));
    };

    patch.osc1Wave     = waveFromParam(*apvts.getRawParameterValue("osc1_waveform"));
    patch.osc1Semitone = static_cast<int>(*apvts.getRawParameterValue("osc1_semitone"));
    patch.osc1Fine     = *apvts.getRawParameterValue("osc1_fine");
    patch.osc1Detune   = *apvts.getRawParameterValue("osc1_detune");
    patch.osc1Level    = *apvts.getRawParameterValue("osc1_level");
    patch.pulseWidth   = *apvts.getRawParameterValue("osc1_pulsewidth");

    patch.osc2Wave     = waveFromParam(*apvts.getRawParameterValue("osc2_waveform"));
    patch.osc2Semitone = static_cast<int>(*apvts.getRawParameterValue("osc2_pitch"));
    patch.osc2Fine     = *apvts.getRawParameterValue("osc2_fine");
    patch.osc2Detune   = *apvts.getRawParameterValue("osc2_detune");
    patch.osc2Level    = *apvts.getRawParameterValue("osc2_level");
    if (*apvts.getRawParameterValue("osc2_on") < 0.5f) patch.osc2Level = 0.0f;
    patch.osc2KeyTrack = *apvts.getRawParameterValue("osc2_key_track") > 0.5f;

    patch.subWave      = waveFromParam(*apvts.getRawParameterValue("sub_wave"));
    patch.subOctave    = static_cast<int>(*apvts.getRawParameterValue("sub_octave"));
    patch.subLevel     = *apvts.getRawParameterValue("sub_level");
    if (*apvts.getRawParameterValue("sub_on")  < 0.5f) patch.subLevel   = 0.0f;
    patch.noiseLevel   = *apvts.getRawParameterValue("noise_level");
    if (*apvts.getRawParameterValue("noise_on")< 0.5f) patch.noiseLevel = 0.0f;

    patch.fmAmount     = *apvts.getRawParameterValue("fm_depth");
    patch.oscSync      = *apvts.getRawParameterValue("osc_sync") > 0.5f;
    patch.ringMod      = *apvts.getRawParameterValue("ring_mod_on") > 0.5f;

    voiceManager.applyPatch(patch);

    int playModeInt = static_cast<int>(*apvts.getRawParameterValue("play_mode"));
    voiceManager.setPlayMode(static_cast<VoiceManager::PlayMode>(playModeInt));
    voiceManager.setUnisonVoices(static_cast<int>(*apvts.getRawParameterValue("voice_count")));
    voiceManager.setUnisonDetune(*apvts.getRawParameterValue("unison_detune"));
    voiceManager.setUnisonSpread(*apvts.getRawParameterValue("unison_spread"));
    voiceManager.setPortamentoTime(*apvts.getRawParameterValue("portamento"));
    voiceManager.setPortamentoMode(*apvts.getRawParameterValue("portamento_auto") > 0.5f);
    voiceManager.setPitchBendRange(static_cast<int>(*apvts.getRawParameterValue("pitch_bend_range")));
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
    using Param  = juce::AudioParameterFloat;
    using ParamI = juce::AudioParameterInt;
    using ParamB = juce::AudioParameterBool;
    using PID    = juce::ParameterID;
    using Range  = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Master
    layout.add(std::make_unique<Param>(PID{"master_volume",1}, "Master Volume", Range(0.0f,1.0f), 0.8f));

    // OSC1
    layout.add(std::make_unique<ParamI>(PID{"osc1_waveform",1}, "Osc1 Waveform", 0, 3, 2)); // default: Saw
    layout.add(std::make_unique<ParamI>(PID{"osc1_semitone",1}, "Osc1 Semitone", -24, 24, 0));
    layout.add(std::make_unique<Param>(PID{"osc1_fine",1}, "Osc1 Fine", Range(-1.0f,1.0f), 0.0f));
    layout.add(std::make_unique<Param>(PID{"osc1_detune",1}, "Osc1 Detune", Range(0.0f,1.0f), 0.0f));
    layout.add(std::make_unique<Param>(PID{"osc1_level",1}, "Osc1 Level", Range(0.0f,1.0f), 1.0f));
    layout.add(std::make_unique<Param>(PID{"osc1_pulsewidth",1}, "Pulse Width", Range(0.01f,0.99f), 0.5f));

    // OSC2
    layout.add(std::make_unique<ParamI>(PID{"osc2_waveform",1}, "Osc2 Waveform", 1, 3, 2)); // Tri/Saw/Square only
    layout.add(std::make_unique<ParamI>(PID{"osc2_pitch",1}, "Osc2 Semitone", -24, 24, 0));
    layout.add(std::make_unique<Param>(PID{"osc2_fine",1}, "Osc2 Fine", Range(-1.0f,1.0f), 0.0f));
    layout.add(std::make_unique<Param>(PID{"osc2_detune",1}, "Osc2 Detune", Range(-1.0f,1.0f), 0.0f));
    layout.add(std::make_unique<Param>(PID{"osc2_level",1}, "Osc2 Level", Range(0.0f,1.0f), 0.0f));
    layout.add(std::make_unique<ParamB>(PID{"osc2_key_track",1}, "Osc2 Key Track", true));
    layout.add(std::make_unique<ParamB>(PID{"osc2_on",1},    "OSC2 On",  false));
    layout.add(std::make_unique<ParamB>(PID{"sub_on",1},     "Sub On",   false));
    layout.add(std::make_unique<ParamB>(PID{"noise_on",1},   "Noise On", false));
    layout.add(std::make_unique<ParamB>(PID{"osc_sync",1}, "Osc Sync", false));
    layout.add(std::make_unique<Param>(PID{"fm_depth",1}, "FM Amount", Range(0.0f,12.0f), 0.0f));
    layout.add(std::make_unique<ParamB>(PID{"ring_mod_on",1}, "Ring Mod", false));

    // Sub + Noise
    layout.add(std::make_unique<ParamI>(PID{"sub_wave",1}, "Sub Waveform", 0, 3, 3));
    layout.add(std::make_unique<ParamI>(PID{"sub_octave",1}, "Sub Octave", -1, 0, -1));
    layout.add(std::make_unique<Param>(PID{"sub_level",1}, "Sub Level", Range(0.0f,1.0f), 0.0f));
    layout.add(std::make_unique<Param>(PID{"noise_level",1}, "Noise Level", Range(0.0f,1.0f), 0.0f));

    // Voice / Global
    layout.add(std::make_unique<ParamI>(PID{"play_mode",1}, "Play Mode", 0, 2, 0)); // 0=Poly,1=Mono,2=Legato
    layout.add(std::make_unique<ParamI>(PID{"voice_count",1}, "Unison Voices", 1, 8, 1));
    layout.add(std::make_unique<Param>(PID{"unison_detune",1}, "Unison Detune", Range(0.0f,1.0f), 0.2f));
    layout.add(std::make_unique<Param>(PID{"unison_spread",1}, "Unison Spread", Range(0.0f,1.0f), 0.5f));
    layout.add(std::make_unique<Param>(PID{"portamento",1}, "Portamento", Range(0.0f,2.0f), 0.0f));
    layout.add(std::make_unique<ParamB>(PID{"portamento_auto",1}, "Portamento Auto", false));
    layout.add(std::make_unique<ParamI>(PID{"pitch_bend_range",1}, "Pitch Bend Range", 1, 12, 2));
    layout.add(std::make_unique<Param>(PID{"master_tune",1}, "Master Tune", Range(-1.0f,1.0f), 0.0f));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OpenSynth1AudioProcessor();
}
