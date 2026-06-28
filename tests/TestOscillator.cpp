#include <juce_core/juce_core.h>
#include "../Source/dsp/Oscillator.h"

struct OscillatorTests : public juce::UnitTest
{
    OscillatorTests() : juce::UnitTest("Oscillator", "DSP") {}

    void runTest() override
    {
        const double sr = 44100.0;
        const float  freq = 440.0f;
        const int    N    = static_cast<int>(sr); // 1 second

        beginTest("Saw stays in [-1.1, 1.1]");
        {
            Oscillator osc;
            osc.prepare(sr);
            osc.setWaveform(Oscillator::Waveform::Saw);
            osc.setFrequency(freq);
            for (int i = 0; i < N; ++i)
            {
                float s = osc.processSample();
                expect(s >= -1.1f && s <= 1.1f,
                       "Saw out of range: " + juce::String(s));
            }
        }

        beginTest("Square stays in [-1.1, 1.1]");
        {
            Oscillator osc;
            osc.prepare(sr);
            osc.setWaveform(Oscillator::Waveform::Square);
            osc.setFrequency(freq);
            for (int i = 0; i < N; ++i)
            {
                float s = osc.processSample();
                expect(s >= -1.1f && s <= 1.1f,
                       "Square out of range: " + juce::String(s));
            }
        }

        beginTest("Sine RMS ~= 0.707");
        {
            Oscillator osc;
            osc.prepare(sr);
            osc.setWaveform(Oscillator::Waveform::Sine);
            osc.setFrequency(freq);
            float sumSq = 0.0f;
            for (int i = 0; i < N; ++i) { float s = osc.processSample(); sumSq += s * s; }
            expectWithinAbsoluteError(std::sqrt(sumSq / N), 0.707f, 0.01f);
        }

        beginTest("Triangle steady-state stays in [-1.1, 1.1]");
        {
            Oscillator osc;
            osc.prepare(sr);
            osc.setWaveform(Oscillator::Waveform::Triangle);
            osc.setFrequency(freq);
            // Warm-up: discard output so the leaky integrator settles past the
            // one-cycle start-up transient (primed from 0) before we measure.
            for (int i = 0; i < 200; ++i) osc.processSample();
            for (int i = 0; i < N; ++i)
            {
                float s = osc.processSample();
                expect(s >= -1.1f && s <= 1.1f,
                       "Triangle out of range: " + juce::String(s));
            }
        }

        beginTest("Frequency change takes effect");
        {
            Oscillator osc;
            osc.prepare(sr);
            osc.setWaveform(Oscillator::Waveform::Sine);

            // Phase 1: count positive zero crossings over 1 sec at 220 Hz ~= 220.
            osc.setFrequency(220.0f);
            int crossings220 = 0;
            float prev = osc.processSample();
            for (int i = 1; i < N; ++i)
            {
                float cur = osc.processSample();
                if (prev < 0.0f && cur >= 0.0f) ++crossings220;
                prev = cur;
            }
            // Allow +-5 crossings tolerance
            expect(std::abs(crossings220 - 220) <= 5,
                   "Expected ~220 crossings, got " + juce::String(crossings220));

            // Phase 2: change frequency to 440 Hz and re-measure; the setter must
            // respond to updates after the first call, so crossings ~= 440.
            osc.setFrequency(440.0f);
            int crossings440 = 0;
            prev = osc.processSample();
            for (int i = 1; i < N; ++i)
            {
                float cur = osc.processSample();
                if (prev < 0.0f && cur >= 0.0f) ++crossings440;
                prev = cur;
            }
            // Allow +-8 crossings tolerance
            expect(std::abs(crossings440 - 440) <= 8,
                   "Expected ~440 crossings after change, got " + juce::String(crossings440));
        }

        beginTest("reset() restores fresh phase state");
        {
            // Advance one oscillator, reset it, and compare its next sample to a
            // freshly-prepared oscillator with identical settings. reset() must
            // restore phase=0 and triAccum=0 (documented semantics).
            Oscillator advanced;
            advanced.prepare(sr);
            advanced.setWaveform(Oscillator::Waveform::Saw);
            advanced.setFrequency(freq);
            for (int i = 0; i < 100; ++i) advanced.processSample();
            advanced.reset();

            Oscillator fresh;
            fresh.prepare(sr);
            fresh.setWaveform(Oscillator::Waveform::Saw);
            fresh.setFrequency(freq);

            expectWithinAbsoluteError(advanced.processSample(),
                                      fresh.processSample(), 1.0e-6f);
        }

        beginTest("setPulseWidth clamps extreme values");
        {
            // Pulse width is clamped to [0.01, 0.99]; extremes must not produce
            // NaN/degenerate output and must stay in range for a Square wave.
            for (float pw : { 0.0f, 1.0f })
            {
                Oscillator osc;
                osc.prepare(sr);
                osc.setWaveform(Oscillator::Waveform::Square);
                osc.setFrequency(freq);
                osc.setPulseWidth(pw);
                for (int i = 0; i < N; ++i)
                {
                    float s = osc.processSample();
                    expect(! std::isnan(s) && ! std::isinf(s),
                           "Square produced non-finite output at pw=" + juce::String(pw));
                    expect(s >= -1.1f && s <= 1.1f,
                           "Square out of range at pw=" + juce::String(pw)
                               + ": " + juce::String(s));
                }
            }
        }
    }
};

static OscillatorTests oscillatorTestsInstance;

#include "../Source/dsp/SubOscillator.h"
#include "../Source/dsp/NoiseGenerator.h"

struct SubOscTests : public juce::UnitTest
{
    SubOscTests() : juce::UnitTest("SubOscillator", "DSP") {}

    void runTest() override
    {
        beginTest("Sub at -1 oct is half frequency of note");
        SubOscillator sub;
        sub.prepare(44100.0);
        sub.setWaveform(Oscillator::Waveform::Square);
        sub.setOctave(-1);
        sub.setNoteFrequency(440.0f); // note is 440 Hz, sub should be 220 Hz
        int crossings = 0;
        float prev = sub.processSample();
        for (int i = 1; i < 44100; ++i)
        {
            float cur = sub.processSample();
            if (prev < 0.0f && cur >= 0.0f) ++crossings;
            prev = cur;
        }
        expect(std::abs(crossings - 220) <= 5,
               "Expected ~220 crossings at -1oct/440Hz, got " + juce::String(crossings));

        beginTest("setOctave re-applies frequency without re-setting note");
        {
            SubOscillator s2;
            s2.prepare(44100.0);
            s2.setWaveform(Oscillator::Waveform::Square);
            s2.setOctave(0);
            s2.setNoteFrequency(440.0f); // 440 Hz at octave 0
            s2.setOctave(-1);            // must now drop to 220 Hz, no re-set of note
            int c = 0;
            float p = s2.processSample();
            for (int i = 1; i < 44100; ++i)
            {
                float cur = s2.processSample();
                if (p < 0.0f && cur >= 0.0f) ++c;
                p = cur;
            }
            expect(std::abs(c - 220) <= 5,
                   "Expected ~220 crossings after setOctave(-1), got " + juce::String(c));
        }

        beginTest("Noise stays in [-1, 1]");
        NoiseGenerator ng;
        for (int i = 0; i < 44100; ++i)
        {
            float s = ng.processSample();
            expect(s >= -1.0f && s <= 1.0f);
        }
    }
};

static SubOscTests subOscTestsInstance;
