#include "Envelope.h"
#include <cmath>

void Envelope::prepare(double sr) noexcept
{
    sampleRate = static_cast<float>(sr);
    reset();
}

void Envelope::noteOn() noexcept
{
    attackCoeff  = calcCoeff(params.attack,  sampleRate);
    decayCoeff   = calcCoeff(params.decay,   sampleRate);
    releaseCoeff = calcCoeff(params.release, sampleRate);
    state = State::Attack;
}

void Envelope::noteOff() noexcept
{
    if (state != State::Idle)
    {
        releaseCoeff = calcCoeff(params.release, sampleRate);
        state = State::Release;
    }
}

void Envelope::reset() noexcept
{
    state  = State::Idle;
    output = 0.0f;
}

float Envelope::processSample() noexcept
{
    float sustain = params.isADR ? 0.0f : params.sustain;

    switch (state)
    {
        case State::Idle:
            output = 0.0f;
            break;

        case State::Attack:
            output += attackCoeff * (1.02f - output);
            if (output >= 1.0f)
            {
                output = 1.0f;
                state  = State::Decay;
            }
            break;

        case State::Decay:
            output += decayCoeff * (sustain - output);
            if (params.isADR && output <= 0.001f)
            {
                output = 0.0f;
                state  = State::Idle;
            }
            else if (!params.isADR && std::abs(output - sustain) < 0.0001f)
            {
                output = sustain;
                state  = State::Sustain;
            }
            break;

        case State::Sustain:
            output = sustain;
            break;

        case State::Release:
            output += releaseCoeff * (0.0f - output);
            if (output <= 0.001f)
            {
                output = 0.0f;
                state  = State::Idle;
            }
            break;
    }
    return output;
}

float Envelope::calcCoeff(float timeSecs, float sr) noexcept
{
    if (timeSecs <= 0.0001f) return 1.0f;
    // Coefficient for one-pole smoother reaching target in ~timeSecs
    return 1.0f - std::exp(-1.0f / (timeSecs * sr));
}
