#pragma once
#include <juce_core/juce_core.h>

class Envelope
{
public:
    struct Params
    {
        float attack  { 0.01f }; // seconds
        float decay   { 0.1f  };
        float sustain { 0.8f  }; // 0–1
        float release { 0.3f  };
        bool  isADR   { false }; // true = ModEnv (sustain always 0)
    };

    void prepare(double sampleRate) noexcept;
    void setParameters(const Params& p) noexcept { params = p; }
    void noteOn()  noexcept;
    void noteOff() noexcept;
    void reset()   noexcept;
    bool isActive() const noexcept { return state != State::Idle; }
    float processSample() noexcept;

private:
    enum class State { Idle, Attack, Decay, Sustain, Release };

    Params params;
    State  state       { State::Idle };
    float  output      { 0.0f };
    float  sampleRate  { 44100.0f };

    // Per-state coefficients (recalculated on noteOn/noteOff)
    float attackCoeff  { 0.0f };
    float decayCoeff   { 0.0f };
    float releaseCoeff { 0.0f };

    static float calcCoeff(float timeSecs, float sampleRate) noexcept;
};
