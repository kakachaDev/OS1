#include "VoiceManager.h"
#include <algorithm>
#include <cmath>

void VoiceManager::prepare(double sr, int /*maxBlockSize*/) noexcept
{
    sampleRate = sr;
    for (auto& v : voices) v.prepare(sr);
    sustainNotes.fill(false);
}

void VoiceManager::applyPatch(const VoicePatch& patch) noexcept
{
    currentPatch = patch;
    for (auto& v : voices) v.applyPatch(patch);
}

void VoiceManager::processBlock(const juce::MidiBuffer& midi,
                                  juce::AudioBuffer<float>& out,
                                  int numSamples) noexcept
{
    int startSample = 0;
    for (const auto meta : midi)
    {
        int pos = meta.samplePosition;
        if (pos > startSample)
            renderVoices(out, startSample, pos - startSample);
        startSample = pos;

        auto msg = meta.getMessage();
        if      (msg.isNoteOn())          noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())         noteOff(msg.getNoteNumber());
        else if (msg.isAllNotesOff())     allNotesOff();
        else if (msg.isSustainPedalOn())  setSustain(true);
        else if (msg.isSustainPedalOff()) setSustain(false);
        else if (msg.isPitchWheel())      setPitchBend(msg.getPitchWheelValue());
    }
    renderVoices(out, startSample, numSamples - startSample);
}

void VoiceManager::renderVoices(juce::AudioBuffer<float>& out, int start, int count) noexcept
{
    const float norm = 1.0f / std::max(1.0f, static_cast<float>(unisonVoices));

    for (int s = start; s < start + count; ++s)
    {
        float L = 0.0f, R = 0.0f;
        for (auto& v : voices)
        {
            if (v.isActive())
            {
                auto [l, r] = v.processSample();
                L += l;
                R += r;
            }
        }
        out.addSample(0, s, L * norm);
        out.addSample(1, s, R * norm);
    }
}

void VoiceManager::noteOn(int note, float velocity) noexcept
{
    sustainNotes[static_cast<size_t>(note)] = false;

    if (playMode == PlayMode::Mono || playMode == PlayMode::Legato)
    {
        // Mono/Legato: one voice plays the newest note. Per-sample portamento
        // glide arrives in Plan 3; for now the voice sits at the correct target pitch.
        allNotesOff();
        auto& v = voices[0];
        v.applyPatch(currentPatch);
        v.setUnisonDetune(0.0f);
        v.setPan(0.5f);
        v.noteOn(note, velocity);
        applyBendToVoice(v, note); // start at the current bent pitch
        lastPlayedNote = note;
        return;
    }

    // Poly mode with unison: allocate unisonVoices slots per note. Per-voice
    // detune and pan live in dedicated SynthVoice members so a per-block
    // applyPatch refresh cannot wipe them.
    for (int u = 0; u < unisonVoices; ++u)
    {
        int idx = findFreeVoice();
        if (idx < 0) idx = stealOldestVoice();

        // Unison detune: spread evenly across [-unisonDetune/2, +unisonDetune/2]
        float detuneOffset = 0.0f;
        if (unisonVoices > 1)
            detuneOffset = unisonDetune * (static_cast<float>(u) / (unisonVoices - 1) - 0.5f);

        // Unison pan spread
        float panPos = 0.5f;
        if (unisonVoices > 1)
            panPos = unisonSpread * (static_cast<float>(u) / (unisonVoices - 1)) + (1.0f - unisonSpread) * 0.5f;

        auto& v = voices[static_cast<size_t>(idx)];
        v.applyPatch(currentPatch);
        v.setUnisonDetune(detuneOffset);
        v.setPan(panPos);
        v.noteOn(note, velocity);
        applyBendToVoice(v, note); // start at the current bent pitch
    }
    lastPlayedNote = note;
}

void VoiceManager::noteOff(int note) noexcept
{
    if (sustainHeld)
    {
        sustainNotes[static_cast<size_t>(note)] = true;
        return;
    }
    for (auto& v : voices)
        if (v.isActive() && v.getMidiNote() == note)
            v.noteOff();
}

void VoiceManager::setSustain(bool on) noexcept
{
    sustainHeld = on;
    if (!on)
    {
        // Release all notes held by sustain
        for (int n = 0; n < 128; ++n)
            if (sustainNotes[static_cast<size_t>(n)]) { noteOff(n); sustainNotes[static_cast<size_t>(n)] = false; }
    }
}

void VoiceManager::setPitchBend(int rawValue) noexcept
{
    // rawValue: 0-16383, centre = 8192
    pitchBendSemis = static_cast<float>(rawValue - 8192) / 8192.0f
                     * static_cast<float>(pitchBendRange);
    // Apply to all active voices (re-centring the wheel resets them to dry pitch)
    for (auto& v : voices)
        if (v.isActive())
            applyBendToVoice(v, v.getMidiNote());
}

void VoiceManager::applyBendToVoice(SynthVoice& v, int note) noexcept
{
    // Override the voice's base frequency with the current pitch-bend amount.
    // Unison detune (each voice's unisonDetune) survives this override.
    v.setFrequencyOverride(SynthVoice::midiNoteToHz(note)
                           * std::pow(2.0f, pitchBendSemis / 12.0f));
}

void VoiceManager::allNotesOff() noexcept
{
    for (auto& v : voices) if (v.isActive()) v.noteOff();
    sustainNotes.fill(false);
    lastPlayedNote = -1;
}

int VoiceManager::findFreeVoice() noexcept
{
    for (int i = 0; i < kMaxVoices; ++i)
        if (!voices[static_cast<size_t>(i)].isActive()) return i;
    return -1;
}

int VoiceManager::stealOldestVoice() noexcept
{
    int oldest = 0;
    int64_t oldestTime = voices[0].getStartTime();
    for (int i = 1; i < kMaxVoices; ++i)
        if (voices[static_cast<size_t>(i)].getStartTime() < oldestTime)
        {
            oldestTime = voices[static_cast<size_t>(i)].getStartTime();
            oldest = i;
        }
    voices[static_cast<size_t>(oldest)].reset();
    return oldest;
}
