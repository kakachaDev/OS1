#include "SynthFilter.h"
#include <cmath>

void SynthFilter::prepare(double sr, int maxBlockSize) noexcept
{
    sampleRate = static_cast<float>(sr);
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = static_cast<uint32_t>(maxBlockSize);
    spec.numChannels      = 1;
    svf.prepare(spec);
    reset();
}

void SynthFilter::setType(Type t) noexcept
{
    type = t;
    switch (t)
    {
        case Type::LP12:
            svf.setType(juce::dsp::StateVariableTPTFilterType::lowpass);   break;
        case Type::HP12:
            svf.setType(juce::dsp::StateVariableTPTFilterType::highpass);  break;
        case Type::BP12:
            svf.setType(juce::dsp::StateVariableTPTFilterType::bandpass);  break;
        default: break;
    }
}

void SynthFilter::setCutoff(float hz) noexcept
{
    cutoff = juce::jlimit(20.0f, 20000.0f, hz);
    updateSVFCoeffs();
    updateMoogCoeffs();
    updateDiodeCoeffs();
}

void SynthFilter::setResonance(float r) noexcept
{
    resonance = juce::jlimit(0.0f, 1.0f, r);
    updateSVFCoeffs();
    updateMoogCoeffs();
    updateDiodeCoeffs();
}

void SynthFilter::setNoteHz(float hz) noexcept
{
    noteHz = hz;
    updateSVFCoeffs();
    updateMoogCoeffs();
    updateDiodeCoeffs();
}

void SynthFilter::reset() noexcept
{
    svf.reset();
    moog.reset();
    diode.reset();
}

float SynthFilter::processSample(float input) noexcept
{
    float out = 0.0f;

    switch (type)
    {
        case Type::LP12:
        case Type::HP12:
        case Type::BP12:
            out = svf.processSample(0, input);
            break;
        case Type::LP24:
            out = moog.processSample(input);
            break;
        case Type::LPDL:
            out = diode.processSample(input);
            break;
    }

    // Saturation post-filter (applies to all types)
    if (saturation > 0.0f)
        out = out + saturation * (softClip(out * (1.0f + saturation * 2.0f)) - out);

    return out;
}

void SynthFilter::updateSVFCoeffs() noexcept
{
    if (type == Type::LP12 || type == Type::HP12 || type == Type::BP12)
    {
        svf.setCutoffFrequency(effectiveCutoff());
        // JUCE SVF resonance: 1/sqrt(2) = no resonance, higher = more resonance
        // Map our 0-1 to ~0.707 to 10.0
        float q = 0.707f + resonance * 9.293f;
        svf.setResonance(q);
    }
}

void SynthFilter::updateMoogCoeffs() noexcept
{
    moog.update(effectiveCutoff(), resonance, sampleRate);
}

void SynthFilter::updateDiodeCoeffs() noexcept
{
    diode.update(effectiveCutoff(), resonance, sampleRate);
}

float SynthFilter::softClip(float x) noexcept
{
    // Continuous cubic soft clip: maps ±1→±1 with zero-derivative join
    if (x >  1.0f) return  1.0f;
    if (x < -1.0f) return -1.0f;
    return 1.5f * x - 0.5f * x * x * x;
}

// ========== Moog Ladder ==========
// 4 cascaded unity-gain one-pole lowpasses with global resonance feedback.
// Soft-saturated feedback keeps it bounded up to self-oscillation.
float SynthFilter::MoogLadder::processSample(float x) noexcept
{
    float in = std::tanh(x - r * buf[3]); // feedback + soft clip (analog-ish, bounded)
    buf[0] += g * (in     - buf[0]);
    buf[1] += g * (buf[0] - buf[1]);
    buf[2] += g * (buf[1] - buf[2]);
    buf[3] += g * (buf[2] - buf[3]);
    return buf[3];
}

void SynthFilter::MoogLadder::update(float cutoffHz, float res, float sr) noexcept
{
    float fc = juce::jlimit(20.0f, sr * 0.45f, cutoffHz);
    g = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * fc / sr);
    g = juce::jlimit(0.0f, 0.99f, g);
    r = juce::jlimit(0.0f, 3.99f, res * 4.0f); // <4 stays stable; tanh bounds the rest
}

// ========== Diode Ladder (TB-303) ==========
// Based on: Bram de Jong / simplified model
// Character: darker than Moog, resonance doesn't reduce bass as much
float SynthFilter::DiodeLadder::processSample(float x) noexcept
{
    // 4-stage with asymmetric clipping characteristic of diode ladder
    float fb = res * stage[3];
    x = x - fb;

    auto diodeTanh = [](float v) noexcept -> float {
        // Asymmetric soft clip (diode approximation: clips harder on negative)
        if (v >= 0.0f)
            return v / (1.0f + std::abs(v));
        return v / (1.0f + std::abs(v) * 2.0f);
    };

    stage[0] += a * (diodeTanh(x)          - diodeTanh(stage[0]));
    stage[1] += a * (diodeTanh(stage[0])   - diodeTanh(stage[1]));
    stage[2] += a * (diodeTanh(stage[1])   - diodeTanh(stage[2]));
    stage[3] += a * (diodeTanh(stage[2])   - diodeTanh(stage[3]));

    return stage[3];
}

void SynthFilter::DiodeLadder::update(float cutoffHz, float res, float sr) noexcept
{
    float f = juce::jlimit(20.0f, sr * 0.49f, cutoffHz) / sr;
    a   = juce::MathConstants<float>::pi * f;
    a   = juce::jlimit(0.0f, 1.0f, a);
    this->res = res * 4.5f; // TB-303 allows slightly higher resonance
}
