#include "LFO.h"
#include <cmath>

void LFO::prepare(double sr) noexcept
{
    sampleRate   = static_cast<float>(sr);
    phase        = 0.0f;
    smoothCoeff  = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 5.0f / sampleRate);
}

void LFO::setRate(float hz) noexcept
{
    phaseIncrement = juce::jlimit(0.0f, 0.5f, hz / sampleRate);
    // Update smooth coefficient based on rate
    smoothCoeff = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * hz * 2.0f / sampleRate);
}

void LFO::retrigger() noexcept
{
    if (keySync) phase = 0.0f;
}

float LFO::processSample() noexcept
{
    float value = 0.0f;

    switch (waveform)
    {
        case Waveform::Sine:
            value = std::sin(juce::MathConstants<float>::twoPi * phase);
            break;

        case Waveform::Triangle:
            value = phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
            break;

        case Waveform::Saw:
            value = 2.0f * phase - 1.0f;
            break;

        case Waveform::Square:
            value = phase < 0.5f ? 1.0f : -1.0f;
            break;

        case Waveform::SteppedRandom:
            // Generate new random value on each cycle reset
            if (phase < lastPhase)
                steppedValue = random.nextFloat() * 2.0f - 1.0f;
            value = steppedValue;
            break;

        case Waveform::SmoothedRandom:
            if (phase < lastPhase)
                smoothTarget = random.nextFloat() * 2.0f - 1.0f;
            smoothCurrent += smoothCoeff * (smoothTarget - smoothCurrent);
            value = smoothCurrent;
            break;
    }

    lastPhase = phase;
    phase += phaseIncrement;
    if (phase >= 1.0f) phase -= 1.0f;

    return value;
}
