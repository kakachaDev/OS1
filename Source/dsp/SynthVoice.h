#pragma once
#include "Oscillator.h"
#include "SubOscillator.h"
#include "NoiseGenerator.h"
#include <utility>

struct VoicePatch
{
    Oscillator::Waveform osc1Wave  { Oscillator::Waveform::Saw };
    int   osc1Semitone  { 0 };
    float osc1Fine      { 0.0f };
    float osc1Detune    { 0.0f };
    float osc1Level     { 1.0f };
    float pulseWidth    { 0.5f };

    Oscillator::Waveform osc2Wave  { Oscillator::Waveform::Saw };
    int   osc2Semitone  { 0 };
    float osc2Fine      { 0.0f };
    float osc2Detune    { 0.0f };
    float osc2Level     { 0.0f };
    bool  osc2KeyTrack  { true };

    Oscillator::Waveform subWave   { Oscillator::Waveform::Square };
    int   subOctave     { -1 };
    float subLevel      { 0.0f };
    float noiseLevel    { 0.0f };

    float fmAmount      { 0.0f };
    bool  oscSync       { false };
    bool  ringMod       { false };

};

class SynthVoice
{
public:
    void prepare(double sampleRate) noexcept;
    void noteOn(int midiNote, float velocity) noexcept;
    void noteOff() noexcept;
    void applyPatch(const VoicePatch& p) noexcept;
    void reset() noexcept;

    bool isActive()    const noexcept { return active; }
    bool isReleasing() const noexcept { return false; } // Plan 3 adds envelopes
    int  getMidiNote() const noexcept { return midiNote; }
    int64_t getStartTime() const noexcept { return startTime; }

    // Returns {left, right} sample
    std::pair<float, float> processSample() noexcept;

    // Called by VoiceManager to override frequency (portamento / unison detune)
    void setFrequencyOverride(float hz) noexcept { frequencyHz = hz; applyFrequencies(); }

    // Per-voice unison state — set by VoiceManager at note-on and NOT touched by
    // applyPatch, so a per-block shared-patch refresh cannot wipe unison.
    void setUnisonDetune(float semitones) noexcept; // semitone offset added to Osc1
    void setPan(float p) noexcept { voicePan = juce::jlimit(0.0f, 1.0f, p); }

    static float midiNoteToHz(int note) noexcept;

private:
    void applyFrequencies() noexcept;

    Oscillator     osc1up, osc1down; // detune split
    Oscillator     osc2;
    SubOscillator  sub;
    NoiseGenerator noise;

    VoicePatch patch;
    int     midiNote    { -1 };
    float   velocity    { 1.0f };
    float   frequencyHz { 440.0f };
    float   osc1BaseHz      { 440.0f }; // Osc1 freq incl. semitone+fine, cached for FM carrier
    float   osc1DetuneRatio { 1.0f };   // cached detune split ratio, preserved under FM
    float   unisonDetune    { 0.0f };   // per-voice unison detune offset (semitones)
    float   voicePan        { 0.5f };   // per-voice stereo position (0=L, 1=R)
    bool    active      { false };
    int64_t startTime   { 0 };
    static int64_t globalClock;
};
