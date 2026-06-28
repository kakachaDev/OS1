#pragma once
#include <juce_core/juce_core.h>

class NoiseGenerator
{
public:
    // White noise is stateless / free-running; prepare and reset are no-ops,
    // provided so every DSP block exposes a uniform prepare/reset interface.
    void prepare(double /*sampleRate*/) noexcept {}
    void reset() noexcept {}

    float processSample() noexcept
    {
        return random.nextFloat() * 2.0f - 1.0f;
    }

private:
    juce::Random random { 12345 };
};
