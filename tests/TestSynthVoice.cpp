#include <juce_core/juce_core.h>
#include "../Source/dsp/SynthVoice.h"
#include <cmath>

struct SynthVoiceTests : public juce::UnitTest
{
    SynthVoiceTests() : juce::UnitTest("SynthVoice", "DSP") {}

    void runTest() override
    {
        const double sr = 44100.0;

        beginTest("Inactive voice is silent");
        {
            SynthVoice v;
            v.prepare(sr);
            for (int i = 0; i < 64; ++i)
            {
                auto [l, r] = v.processSample();
                expect(juce::exactlyEqual(l, 0.0f) && juce::exactlyEqual(r, 0.0f));
            }
        }

        beginTest("noteOn yields finite, in-range, audible output");
        {
            SynthVoice v;
            v.prepare(sr);
            VoicePatch p;            // default: Osc1 Saw at full level
            v.applyPatch(p);
            v.noteOn(69, 1.0f);      // A4 = 440 Hz
            expect(v.isActive());
            float sumSq = 0.0f;
            const int N = static_cast<int>(sr);
            for (int i = 0; i < N; ++i)
            {
                auto [l, r] = v.processSample();
                expect(std::isfinite(l) && std::isfinite(r), "non-finite output");
                expect(l >= -2.0f && l <= 2.0f && r >= -2.0f && r <= 2.0f, "out of range");
                sumSq += l * l + r * r;
            }
            expect(std::sqrt(sumSq / (2.0f * N)) > 0.05f, "expected audible output");
        }

        beginTest("noteOff silences voice immediately (no envelope in Plan 2)");
        {
            SynthVoice v;
            v.prepare(sr);
            VoicePatch p;
            v.applyPatch(p);
            v.noteOn(60, 1.0f);
            v.processSample();
            v.noteOff();
            expect(! v.isActive());
            auto [l, r] = v.processSample();
            expect(juce::exactlyEqual(l, 0.0f) && juce::exactlyEqual(r, 0.0f), "voice should be silent after noteOff");
        }

        beginTest("FM carrier tracks Osc1 semitone transposition");
        {
            SynthVoice v;
            v.prepare(sr);
            VoicePatch p;
            p.osc1Wave     = Oscillator::Waveform::Saw;
            p.osc1Level    = 1.0f;
            p.osc1Semitone = 12;     // one octave up -> ~880 Hz carrier
            p.osc2Level    = 0.0f;   // hear Osc1 only
            p.fmAmount     = 0.5f;   // small FM, average pitch ~ carrier
            p.pan          = 0.5f;
            v.applyPatch(p);
            v.noteOn(69, 1.0f);      // A4 = 440 Hz -> carrier ~880 Hz
            int crossings = 0;
            float prev = v.processSample().first;
            for (int i = 1; i < static_cast<int>(sr); ++i)
            {
                float cur = v.processSample().first;
                if (prev < 0.0f && cur >= 0.0f) ++crossings;
                prev = cur;
            }
            expect(crossings > 700,
                   "FM carrier should track Osc1 +12 semitones (~880 Hz); got "
                   + juce::String(crossings) + " crossings");
        }
    }
};

static SynthVoiceTests synthVoiceTestsInstance;
