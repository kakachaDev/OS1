#pragma once
#include <cmath>
#include <juce_core/juce_core.h>

class Oscillator
{
public:
    enum class Waveform { Sine = 0, Triangle, Saw, Square };

    void prepare(double sampleRate) noexcept;
    void setFrequency(float hz) noexcept;
    void setWaveform(Waveform w) noexcept   { waveform = w; }
    void setPulseWidth(float pw) noexcept   { pulseWidth = juce::jlimit(0.01f, 0.99f, pw); }
    void reset() noexcept                   { phase = 0.0f; triAccum = 0.0f; }
    float processSample() noexcept;

private:
    static float polyBlep(float t, float dt) noexcept;

    Waveform waveform       { Waveform::Saw };
    float phase             { 0.0f };
    float phaseIncrement    { 0.0f };
    float sampleRate        { 44100.0f };
    float pulseWidth        { 0.5f };
    float triAccum          { 0.0f }; // integrated square for band-limited triangle
};
