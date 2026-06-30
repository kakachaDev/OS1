#pragma once
#include <juce_core/juce_core.h>

class LFO
{
public:
    enum class Waveform { Sine=0, Triangle, Saw, Square, SteppedRandom, SmoothedRandom };

    void prepare(double sampleRate) noexcept;
    void setRate(float hz) noexcept;
    void setWaveform(Waveform w) noexcept { waveform = w; }
    void setKeySync(bool ks) noexcept     { keySync = ks; }
    void retrigger() noexcept;            // call on noteOn when keySync=true
    float processSample() noexcept;

private:
    Waveform waveform       { Waveform::Sine };
    float phase             { 0.0f };
    float phaseIncrement    { 0.0f };
    float sampleRate        { 44100.0f };
    bool  keySync           { false };

    // For smoothed random
    float smoothTarget      { 0.0f };
    float smoothCurrent     { 0.0f };
    float smoothCoeff       { 0.0f };
    float lastPhase         { 0.0f };
    float steppedValue      { 0.0f };
    juce::Random random     { 99991 };
};
