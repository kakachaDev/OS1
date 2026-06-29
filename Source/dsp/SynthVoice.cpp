#include "SynthVoice.h"
#include <cmath>

int64_t SynthVoice::globalClock = 0;

void SynthVoice::prepare(double sr) noexcept
{
    osc1up.prepare(sr);
    osc1down.prepare(sr);
    osc2.prepare(sr);
    sub.prepare(sr);
    noise.prepare(sr);
}

void SynthVoice::noteOn(int note, float vel) noexcept
{
    midiNote    = note;
    velocity    = vel;
    frequencyHz = midiNoteToHz(note);
    active      = true;
    startTime   = ++globalClock;
    osc1up.reset();
    osc1down.reset();
    osc2.reset();
    sub.reset();
    noise.reset();
    applyFrequencies();
}

void SynthVoice::noteOff() noexcept
{
    // No envelope in Plan 2 — silence immediately
    active = false;
}

void SynthVoice::applyPatch(const VoicePatch& p) noexcept
{
    patch = p;
    osc1up.setWaveform(p.osc1Wave);
    osc1down.setWaveform(p.osc1Wave);
    osc1up.setPulseWidth(p.pulseWidth);
    osc1down.setPulseWidth(p.pulseWidth);
    osc2.setWaveform(p.osc2Wave == Oscillator::Waveform::Sine
                     ? Oscillator::Waveform::Saw  // Osc2 has no Sine — fall back to Saw
                     : p.osc2Wave);
    osc2.setPulseWidth(p.pulseWidth);
    sub.setWaveform(p.subWave);
    sub.setOctave(p.subOctave);
    if (active) applyFrequencies();
}

void SynthVoice::reset() noexcept
{
    active = false;
    osc1up.reset();
    osc1down.reset();
    osc2.reset();
    sub.reset();
    noise.reset();
}

void SynthVoice::applyFrequencies() noexcept
{
    const float semitoneRatio = std::pow(2.0f, 1.0f / 12.0f);

    // Osc1 frequency: base + semitone offset + fine
    float osc1Hz = frequencyHz
        * std::pow(semitoneRatio,
                   static_cast<float>(patch.osc1Semitone) + patch.osc1Fine + unisonDetune);

    // Osc1 detune: split into up and down copies
    float detuneRatio = std::pow(semitoneRatio, patch.osc1Detune * 0.5f);
    // Cache so per-sample FM can re-modulate Osc1 without losing the
    // semitone/fine transposition (osc1BaseHz) or the detune split.
    osc1BaseHz      = osc1Hz;
    osc1DetuneRatio = detuneRatio;
    osc1up.setFrequency(osc1Hz * detuneRatio);
    osc1down.setFrequency(osc1Hz / detuneRatio);

    // Osc2 frequency
    float osc2Hz = (patch.osc2KeyTrack ? frequencyHz : 440.0f)
        * std::pow(semitoneRatio, static_cast<float>(patch.osc2Semitone) + patch.osc2Fine + patch.osc2Detune);
    osc2.setFrequency(osc2Hz);

    // Sub
    sub.setNoteFrequency(osc1Hz);
}

std::pair<float, float> SynthVoice::processSample() noexcept
{
    if (!active) return { 0.0f, 0.0f };

    // Osc2 sample (may FM-modulate Osc1)
    float o2 = osc2.processSample();

    // FM: Osc2 modulates Osc1 pitch (Osc2 -> Osc1). Modulate around the cached
    // Osc1 base (which includes semitone/fine) and keep the detune split intact.
    if (patch.fmAmount > 0.0f)
    {
        const float fmBase = osc1BaseHz
            * std::pow(2.0f, patch.fmAmount * o2 / 12.0f);
        osc1up.setFrequency(fmBase * osc1DetuneRatio);
        osc1down.setFrequency(fmBase / osc1DetuneRatio);
    }

    // Osc1 sample (average of up and down detune copies)
    float o1 = (osc1up.processSample() + osc1down.processSample()) * 0.5f;

    // Ring mod: replace o2 in mix with o1*o2
    float osc2mix = patch.ringMod ? (o1 * o2) : o2;

    // Sub and noise
    float subSample   = sub.processSample();
    float noiseSample = noise.processSample();

    // Mix (sub does NOT go through ring mod, per Synth1 spec)
    float mono = o1        * patch.osc1Level
               + osc2mix   * patch.osc2Level
               + subSample * patch.subLevel
               + noiseSample * patch.noiseLevel;

    mono *= velocity;

    // Stereo pan (equal power)
    float panL = std::sqrt(1.0f - voicePan);
    float panR = std::sqrt(voicePan);

    return { mono * panL, mono * panR };
}

void SynthVoice::setUnisonDetune(float semitones) noexcept
{
    unisonDetune = semitones;
    if (active) applyFrequencies();
}

float SynthVoice::midiNoteToHz(int note) noexcept
{
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}
