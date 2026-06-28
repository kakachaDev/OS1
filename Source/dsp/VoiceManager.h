#pragma once
#include "SynthVoice.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

class VoiceManager
{
public:
    enum class PlayMode { Poly = 0, Mono, Legato };

    static constexpr int kMaxVoices = 32;

    void prepare(double sampleRate, int maxBlockSize) noexcept;
    void applyPatch(const VoicePatch& patch) noexcept;
    void processBlock(const juce::MidiBuffer& midi,
                      juce::AudioBuffer<float>& output,
                      int numSamples) noexcept;

    void setPlayMode(PlayMode m)         noexcept { playMode = m; }
    void setUnisonVoices(int n)          noexcept { unisonVoices = juce::jlimit(1, 8, n); }
    void setUnisonDetune(float semitones)noexcept { unisonDetune = semitones; }
    void setUnisonSpread(float s)        noexcept { unisonSpread = juce::jlimit(0.0f, 1.0f, s); }
    void setPortamentoTime(float secs)   noexcept { portamentoTime = secs; }
    void setPortamentoMode(bool autoMode)noexcept { portamentoAuto = autoMode; }
    void setPitchBendRange(int semitones)noexcept { pitchBendRange = semitones; }
    void allNotesOff() noexcept;

private:
    void noteOn(int note, float velocity) noexcept;
    void noteOff(int note) noexcept;
    void setPitchBend(int value) noexcept; // raw MIDI 0-16383
    void applyBendToVoice(SynthVoice& v, int note) noexcept; // apply current bend to a voice
    void setSustain(bool on) noexcept;
    void renderVoices(juce::AudioBuffer<float>& out, int start, int count) noexcept;

    // Find a free voice slot. Returns index or -1 if none free (will steal).
    int findFreeVoice() noexcept;
    // Steal the oldest active voice.
    int stealOldestVoice() noexcept;

    std::array<SynthVoice, kMaxVoices> voices;
    VoicePatch currentPatch;

    double sampleRate     { 44100.0 };
    PlayMode playMode     { PlayMode::Poly };
    int unisonVoices      { 1 };
    float unisonDetune    { 0.0f };  // total semitone spread
    float unisonSpread    { 0.0f };  // stereo spread 0-1

    float portamentoTime  { 0.0f };  // seconds
    bool  portamentoAuto  { false }; // true = legato-only glide
    float pitchBendSemis  { 0.0f }; // current bend in semitones
    int   pitchBendRange  { 2 };

    bool  sustainHeld     { false };
    // Notes held by sustain pedal (noteOff received but sustain on)
    std::array<bool, 128> sustainNotes {};

    // Portamento: current playing frequency (for glide)
    float portaCurrentHz  { 440.0f };
    float portaTargetHz   { 440.0f };
    float portaCoeff      { 1.0f };  // per-sample glide coefficient
    int   lastPlayedNote  { -1 };
};
