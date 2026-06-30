#pragma once
#include <juce_dsp/juce_dsp.h>

class SynthFilter
{
public:
    enum class Type { LP12=0, LP24, HP12, BP12, LPDL };

    void prepare(double sampleRate, int maxBlockSize) noexcept;
    void setType(Type t) noexcept;
    void setCutoff(float hz) noexcept;
    void setResonance(float r) noexcept; // 0.0–1.0
    void setSaturation(float s) noexcept { saturation = juce::jlimit(0.0f, 1.0f, s); }
    void setKeyTracking(float kt) noexcept { keyTrack = juce::jlimit(0.0f, 1.0f, kt); }
    void setNoteHz(float hz) noexcept;   // current voice fundamental
    void reset() noexcept;
    float processSample(float input) noexcept;

private:
    void updateSVFCoeffs() noexcept;
    void updateMoogCoeffs() noexcept;
    void updateDiodeCoeffs() noexcept;
    static float softClip(float x) noexcept;

    Type  type       { Type::LP12 };
    float cutoff     { 8000.0f };
    float resonance  { 0.0f };
    float saturation { 0.0f };
    float keyTrack   { 0.0f };
    float noteHz     { 440.0f };
    float sampleRate { 44100.0f };

    // ---- LP12 / HP12 / BP12: JUCE Trapezoidal SVF ----
    juce::dsp::StateVariableTPTFilter<float> svf;

    // ---- LP24: Moog Ladder (Huovilainen simplified) ----
    struct MoogLadder
    {
        float buf[4]  { 0,0,0,0 };
        float p { 0.7f }, r { 0.0f };
        float processSample(float x) noexcept;
        void  update(float cutoffHz, float resonance, float sr) noexcept;
        void  reset() noexcept { std::fill(buf, buf+4, 0.0f); }
    } moog;

    // ---- LPDL: TB-303 Diode Ladder ----
    struct DiodeLadder
    {
        float stage[4] { 0,0,0,0 };
        float a { 0.5f }, res { 0.0f };
        float processSample(float x) noexcept;
        void  update(float cutoffHz, float resonance, float sr) noexcept;
        void  reset() noexcept { std::fill(stage, stage+4, 0.0f); }
    } diode;
};
