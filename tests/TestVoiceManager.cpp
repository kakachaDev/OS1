#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/dsp/VoiceManager.h"
#include <cmath>

namespace
{
    float bufferRMS(const juce::AudioBuffer<float>& buf) noexcept
    {
        double sumSq = 0.0;
        const int n = buf.getNumChannels() * buf.getNumSamples();
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            for (int i = 0; i < buf.getNumSamples(); ++i)
            {
                const float s = buf.getSample(ch, i);
                sumSq += static_cast<double>(s) * s;
            }
        return static_cast<float>(std::sqrt(sumSq / juce::jmax(1, n)));
    }

    bool bufferIsFinite(const juce::AudioBuffer<float>& buf) noexcept
    {
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            for (int i = 0; i < buf.getNumSamples(); ++i)
                if (! std::isfinite(buf.getSample(ch, i))) return false;
        return true;
    }
}

struct VoiceManagerTests : public juce::UnitTest
{
    VoiceManagerTests() : juce::UnitTest("VoiceManager", "DSP") {}

    void runTest() override
    {
        const double sr = 44100.0;
        const int    N  = 512;

        beginTest("Silent with no MIDI input");
        {
            VoiceManager vm;
            vm.prepare(sr, N);
            VoicePatch p;
            vm.applyPatch(p);
            juce::AudioBuffer<float> buf(2, N);
            buf.clear();
            juce::MidiBuffer midi;
            vm.processBlock(midi, buf, N);
            expect(juce::exactlyEqual(bufferRMS(buf), 0.0f), "expected silence with no notes");
        }

        beginTest("Note-on produces finite, audible output");
        {
            VoiceManager vm;
            vm.prepare(sr, N);
            VoicePatch p;
            vm.applyPatch(p);
            juce::AudioBuffer<float> buf(2, N);
            buf.clear();
            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8) 100), 0);
            vm.processBlock(midi, buf, N);
            expect(bufferIsFinite(buf), "output must be finite");
            expect(bufferRMS(buf) > 0.01f, "expected audible output after note-on");
        }

        beginTest("Note-off silences the voice (no release in Plan 2)");
        {
            VoiceManager vm;
            vm.prepare(sr, N);
            VoicePatch p;
            vm.applyPatch(p);
            juce::AudioBuffer<float> buf(2, N);

            juce::MidiBuffer onMidi;
            onMidi.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8) 100), 0);
            buf.clear();
            vm.processBlock(onMidi, buf, N);
            expect(bufferRMS(buf) > 0.01f);

            juce::MidiBuffer offMidi;
            offMidi.addEvent(juce::MidiMessage::noteOff(1, 69), 0);
            buf.clear();
            vm.processBlock(offMidi, buf, N);
            expect(juce::exactlyEqual(bufferRMS(buf), 0.0f), "voice should be silent after note-off");
        }

        beginTest("Sustain pedal holds note through note-off, releases on pedal-up");
        {
            VoiceManager vm;
            vm.prepare(sr, N);
            VoicePatch p;
            vm.applyPatch(p);
            juce::AudioBuffer<float> buf(2, N);

            juce::MidiBuffer m1;
            m1.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
            m1.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 1); // sustain on
            buf.clear();
            vm.processBlock(m1, buf, N);
            expect(bufferRMS(buf) > 0.01f);

            juce::MidiBuffer m2;
            m2.addEvent(juce::MidiMessage::noteOff(1, 60), 0); // released, but pedal down
            buf.clear();
            vm.processBlock(m2, buf, N);
            expect(bufferRMS(buf) > 0.01f, "sustain should hold the note after note-off");

            juce::MidiBuffer m3;
            m3.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 0); // sustain off
            buf.clear();
            vm.processBlock(m3, buf, N);
            expect(juce::exactlyEqual(bufferRMS(buf), 0.0f), "release on pedal-up should silence");
        }

        beginTest("Pitch bend applies to a note triggered after the wheel moves");
        {
            VoiceManager vm;
            vm.prepare(sr, 64);
            VoicePatch p;
            p.osc1Wave  = Oscillator::Waveform::Saw;
            p.osc1Level = 1.0f;
            p.osc2Level = 0.0f;
            vm.applyPatch(p);
            vm.setPitchBendRange(2); // +/-2 semitones

            juce::AudioBuffer<float> warm(2, 64);
            warm.clear();
            juce::MidiBuffer wheel;
            wheel.addEvent(juce::MidiMessage::pitchWheel(1, 16383), 0);
            vm.processBlock(wheel, warm, 64);

            const int M = static_cast<int>(sr); // 1 second
            juce::AudioBuffer<float> buf(2, M);
            buf.clear();
            juce::MidiBuffer noteMidi;
            noteMidi.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8) 100), 0);
            vm.processBlock(noteMidi, buf, M);

            int crossings = 0;
            const float* chan = buf.getReadPointer(0);
            float prev = chan[0];
            for (int i = 1; i < M; ++i)
            {
                if (prev < 0.0f && chan[i] >= 0.0f) ++crossings;
                prev = chan[i];
            }
            expect(crossings > 470,
                   "note triggered under +2 semitone bend should sound ~494 Hz; got "
                   + juce::String(crossings));
        }

        beginTest("Unison detune/spread survives a per-block applyPatch");
        {
            VoiceManager vm;
            vm.prepare(sr, 512);
            VoicePatch p;
            p.osc1Wave  = Oscillator::Waveform::Saw;
            p.osc1Level = 1.0f;
            p.osc2Level = 0.0f;
            vm.applyPatch(p);
            vm.setUnisonVoices(2);
            vm.setUnisonDetune(1.0f); // ~1 semitone spread across the pair
            vm.setUnisonSpread(1.0f); // hard left / hard right

            juce::AudioBuffer<float> b1(2, 512);
            b1.clear();
            juce::MidiBuffer on;
            on.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8) 100), 0);
            vm.processBlock(on, b1, 512);

            // Simulate the processor re-applying the shared patch every block.
            vm.applyPatch(p);

            const int M = 4096;
            juce::AudioBuffer<float> b2(2, M);
            b2.clear();
            juce::MidiBuffer empty;
            vm.processBlock(empty, b2, M);

            double diff = 0.0, energy = 0.0;
            const float* L = b2.getReadPointer(0);
            const float* R = b2.getReadPointer(1);
            for (int i = 0; i < M; ++i)
            {
                diff   += std::abs(L[i] - R[i]);
                energy += std::abs(L[i]) + std::abs(R[i]);
            }
            expect(energy > 0.0, "expected audible unison output");
            // Hard-panned, detuned voices keep L and R clearly different. If a
            // per-block applyPatch wiped per-voice detune/pan (the bug), both
            // voices collapse to centre + same pitch and L == R (ratio ~0).
            expect(diff / energy > 0.2,
                   "unison spread+detune must survive re-applyPatch; L/R ratio="
                   + juce::String(diff / energy));
        }

        beginTest("Poly: two simultaneous notes are finite and audible");
        {
            VoiceManager vm;
            vm.prepare(sr, 512);
            VoicePatch p;
            p.osc1Level = 1.0f;
            p.osc2Level = 0.0f;
            vm.applyPatch(p);
            vm.setPlayMode(VoiceManager::PlayMode::Poly);
            vm.setUnisonVoices(1);
            juce::AudioBuffer<float> buf(2, 512);
            buf.clear();
            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
            midi.addEvent(juce::MidiMessage::noteOn(1, 67, (juce::uint8) 100), 0);
            vm.processBlock(midi, buf, 512);
            expect(bufferIsFinite(buf), "chord output must be finite");
            expect(bufferRMS(buf) > 0.01f, "two-note chord should be audible");
        }

        beginTest("Mono: newest note steals the single voice (pitch follows latest)");
        {
            VoiceManager vm;
            vm.prepare(sr, 64);
            VoicePatch p;
            p.osc1Wave  = Oscillator::Waveform::Saw;
            p.osc1Level = 1.0f;
            p.osc2Level = 0.0f;
            vm.applyPatch(p);
            vm.setPlayMode(VoiceManager::PlayMode::Mono);

            juce::AudioBuffer<float> warm(2, 64);
            warm.clear();
            juce::MidiBuffer m;
            m.addEvent(juce::MidiMessage::noteOn(1, 57, (juce::uint8) 100), 0); // A3 ~220 Hz
            m.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8) 100), 1); // A4 ~440 Hz, steals
            vm.processBlock(m, warm, 64);

            const int M = static_cast<int>(sr);
            juce::AudioBuffer<float> buf(2, M);
            buf.clear();
            juce::MidiBuffer empty;
            vm.processBlock(empty, buf, M);

            int crossings = 0;
            const float* chan = buf.getReadPointer(0);
            float prev = chan[0];
            for (int i = 1; i < M; ++i)
            {
                if (prev < 0.0f && chan[i] >= 0.0f) ++crossings;
                prev = chan[i];
            }
            // Mono must follow the latest note (~440 Hz), not the stolen 220 Hz one.
            expect(crossings > 330,
                   "mono should follow the newest note (~440 Hz); got "
                   + juce::String(crossings));
        }
    }
};

static VoiceManagerTests voiceManagerTestsInstance;
