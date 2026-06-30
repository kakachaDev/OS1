#include <juce_core/juce_core.h>
#include "../Source/dsp/SynthFilter.h"
#include <cmath>

struct FilterTests : public juce::UnitTest
{
    FilterTests() : juce::UnitTest("SynthFilter", "DSP") {}

    // Compute RMS of a band using a pure sine tone through the filter
    float measureRMS(SynthFilter& f, float testFreqHz, double sr, int N)
    {
        f.reset();
        float ph = 0.0f, dt = testFreqHz / static_cast<float>(sr);
        float sumSq = 0.0f;
        // Settle filter for 1000 samples first
        for (int i = 0; i < 1000; ++i)
            f.processSample(std::sin(juce::MathConstants<float>::twoPi * (ph += dt)));
        ph = 0.0f;
        for (int i = 0; i < N; ++i)
        {
            float s = f.processSample(std::sin(juce::MathConstants<float>::twoPi * (ph += dt)));
            sumSq += s * s;
        }
        return std::sqrt(sumSq / N);
    }

    void runTest() override
    {
        const double sr = 44100.0;
        const int N = static_cast<int>(sr * 0.5);

        beginTest("LP12 passes low, attenuates high");
        {
            SynthFilter f;
            f.prepare(sr, 512);
            f.setType(SynthFilter::Type::LP12);
            f.setCutoff(1000.0f);
            f.setResonance(0.0f);
            float rmsLow  = measureRMS(f, 200.0f,  sr, N);
            float rmsHigh = measureRMS(f, 8000.0f, sr, N);
            expect(rmsLow > rmsHigh * 3.0f,
                   "LP12: low=" + juce::String(rmsLow) + " high=" + juce::String(rmsHigh));
        }

        beginTest("HP12 passes high, attenuates low");
        {
            SynthFilter f;
            f.prepare(sr, 512);
            f.setType(SynthFilter::Type::HP12);
            f.setCutoff(2000.0f);
            f.setResonance(0.0f);
            float rmsLow  = measureRMS(f, 100.0f,  sr, N);
            float rmsHigh = measureRMS(f, 8000.0f, sr, N);
            expect(rmsHigh > rmsLow * 3.0f,
                   "HP12: high=" + juce::String(rmsHigh) + " low=" + juce::String(rmsLow));
        }

        beginTest("LP24 passes low, attenuates high");
        {
            SynthFilter f;
            f.prepare(sr, 512);
            f.setType(SynthFilter::Type::LP24);
            f.setCutoff(1000.0f);
            f.setResonance(0.0f);
            float rmsLow  = measureRMS(f, 200.0f,  sr, N);
            float rmsHigh = measureRMS(f, 8000.0f, sr, N);
            expect(rmsLow > rmsHigh * 5.0f,
                   "LP24: low=" + juce::String(rmsLow) + " high=" + juce::String(rmsHigh));
        }

        beginTest("LPDL passes low, attenuates high");
        {
            SynthFilter f;
            f.prepare(sr, 512);
            f.setType(SynthFilter::Type::LPDL);
            f.setCutoff(1000.0f);
            f.setResonance(0.0f);
            float rmsLow  = measureRMS(f, 200.0f,  sr, N);
            float rmsHigh = measureRMS(f, 8000.0f, sr, N);
            expect(rmsLow > rmsHigh * 3.0f,
                   "LPDL: low=" + juce::String(rmsLow) + " high=" + juce::String(rmsHigh));
        }

        beginTest("Filter output stays in reasonable range with resonance");
        {
            SynthFilter f;
            f.prepare(sr, 512);
            f.setType(SynthFilter::Type::LP24);
            f.setCutoff(440.0f);
            f.setResonance(0.9f);
            f.reset();
            float ph = 0.0f;
            bool ok = true;
            for (int i = 0; i < static_cast<int>(sr); ++i)
            {
                float s = f.processSample(std::sin(juce::MathConstants<float>::twoPi * ph));
                ph += 440.0f / static_cast<float>(sr);
                if (std::abs(s) > 10.0f) { ok = false; break; }
            }
            expect(ok, "Filter unstable at high resonance");
        }
    }
};

static FilterTests filterTestsInstance;
