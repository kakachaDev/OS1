#pragma once
#include <juce_core/juce_core.h>

class NoiseGenerator
{
public:
    float processSample() noexcept
    {
        return random.nextFloat() * 2.0f - 1.0f;
    }

private:
    juce::Random random { 12345 };
};
