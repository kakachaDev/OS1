#pragma once
#include "Oscillator.h"

class SubOscillator
{
public:
    void prepare(double sampleRate) noexcept { osc.prepare(sampleRate); }

    void setNoteFrequency(float hz) noexcept
    {
        // octave: 0 = same pitch, -1 = one octave below
        noteHz = hz;
        applyFrequency();
    }

    void setWaveform(Oscillator::Waveform w) noexcept { osc.setWaveform(w); }

    void setOctave(int oct) noexcept   // 0 or -1
    {
        octave = juce::jlimit(-1, 0, oct);
        applyFrequency(); // re-apply immediately so an octave change is not stale
    }

    void reset() noexcept { osc.reset(); }
    float processSample() noexcept { return osc.processSample(); }

private:
    void applyFrequency() noexcept
    {
        osc.setFrequency(noteHz * (octave == 0 ? 1.0f : 0.5f));
    }

    Oscillator osc;
    float noteHz { 0.0f };
    int   octave { -1 };
};
