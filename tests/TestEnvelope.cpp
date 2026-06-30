#include <juce_core/juce_core.h>
#include "../Source/dsp/Envelope.h"

struct EnvelopeTests : public juce::UnitTest
{
    EnvelopeTests() : juce::UnitTest("Envelope", "DSP") {}

    void runTest() override
    {
        const double sr = 44100.0;

        beginTest("Output is 0 before noteOn");
        {
            Envelope env;
            env.prepare(sr);
            for (int i = 0; i < 100; ++i)
                expectWithinAbsoluteError(env.processSample(), 0.0f, 0.001f);
        }

        beginTest("Output reaches 1 during attack");
        {
            Envelope env;
            env.prepare(sr);
            Envelope::Params p;
            p.attack  = 0.1f;
            p.decay   = 0.1f;
            p.sustain = 0.5f;
            p.release = 0.1f;
            env.setParameters(p);
            env.noteOn();
            float peak = 0.0f;
            for (int i = 0; i < static_cast<int>(0.5 * sr); ++i)
                peak = std::max(peak, env.processSample());
            expectWithinAbsoluteError(peak, 1.0f, 0.01f);
        }

        beginTest("Sustain level held after decay");
        {
            Envelope env;
            env.prepare(sr);
            Envelope::Params p;
            p.attack = 0.001f; p.decay = 0.01f; p.sustain = 0.6f; p.release = 0.01f;
            env.setParameters(p);
            env.noteOn();
            float last = 0.0f;
            for (int i = 0; i < static_cast<int>(0.5 * sr); ++i)
                last = env.processSample();
            expectWithinAbsoluteError(last, 0.6f, 0.02f);
        }

        beginTest("Output reaches ~0 after noteOff + release");
        {
            Envelope env;
            env.prepare(sr);
            Envelope::Params p;
            p.attack = 0.001f; p.decay = 0.001f; p.sustain = 1.0f; p.release = 0.05f;
            env.setParameters(p);
            env.noteOn();
            for (int i = 0; i < static_cast<int>(0.1 * sr); ++i) env.processSample();
            env.noteOff();
            float last = 1.0f;
            for (int i = 0; i < static_cast<int>(1.0 * sr); ++i)
                last = env.processSample();
            expectWithinAbsoluteError(last, 0.0f, 0.01f);
            expect(!env.isActive());
        }

        beginTest("ADR (ModEnv) decays to 0 without noteOff");
        {
            Envelope env;
            env.prepare(sr);
            Envelope::Params p;
            p.attack = 0.001f; p.decay = 0.05f; p.sustain = 1.0f; p.isADR = true;
            env.setParameters(p);
            env.noteOn();
            float last = 1.0f;
            for (int i = 0; i < static_cast<int>(1.0 * sr); ++i)
                last = env.processSample();
            expectWithinAbsoluteError(last, 0.0f, 0.01f);
        }
    }
};

static EnvelopeTests envelopeTestsInstance;
