#pragma once

// Portable audio DSP. This file deliberately has NO JUCE dependency so that
// basic audio tests can run before downloading JUCE / running GitHub Actions.
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace sdsp {
constexpr float pi = 3.14159265358979323846f;
constexpr int maxIR = 768;

struct Parameters {
    int amp = 0, cab = 0, micTypeA = 0, micTypeB = 1;
    float input = 0, gate = -60, trans = 0, dbl = 12, out = 0;
    float gain = 5, bass = 5, mid = 5, treb = 5, pres = 5, master = 5, ao = 5;
    float irlv = 0, irlo = 80, irhi = 12000;
    float apos=.3f, adist=.1f, alv=0, apan=-.3f;
    float bpos=.6f, bdist=.25f, blv=-3, bpan=.3f;
    float arm=-6.f, brm=-6.f;
    std::array<float, 9> eq{};
    std::array<std::array<float,3>,5> pedals{{{{5,5,5}},{{5,5,5}},{{5,5,5}},{{5,5,5}},{{5,5,5}}}};
    bool power=true, gateOn=true, eqOn=true, irOn=false, ma=true, mb=true;
    bool doubler=false, roomAOn=false, roomBOn=false;
    std::array<bool,5> pedalOn{{false,false,false,false,false}};
};

inline float decibels(float db) noexcept { return std::pow(10.0f, db * (1.0f/20.0f)); }
inline float clamp(float x, float lo, float hi) noexcept { return std::max(lo, std::min(hi, x)); }

class Biquad {
public:
    void lowpass(float sr, float freq, float Q=.707f) noexcept;
    void highpass(float sr, float freq, float Q=.707f) noexcept;
    void peak(float sr, float freq, float Q, float db) noexcept;
    void shelf(float sr, float freq, float db, bool high) noexcept;
    float tick(float x) noexcept {
        const float y = b0*x + z1;
        z1 = b1*x - a1*y + z2;
        z2 = b2*x - a2*y;
        return std::isfinite(y) ? y : 0.0f;
    }
    void reset() noexcept { z1 = z2 = 0.0f; }
private:
    void coefficients(float b0_,float b1_,float b2_,float a0_,float a1_,float a2_) noexcept;
    float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
};

class AmpEngine {
public:
    void prepare(double sr, int maxBlockSize);
    void reset() noexcept;
    void configure(const Parameters& p) noexcept;
    void setImpulse(const std::array<float, maxIR>& taps, int length) noexcept;
    void clearImpulse() noexcept;
    float processSample(int ch, float x, const Parameters& p) noexcept;
    bool hasImpulse() const noexcept { return impulseLength > 0; }
private:
    struct Channel {
        Biquad inputHigh, bass, mid, high, presence, body, cabinetHigh, cabinetLow, irHigh, irLow;
        Biquad micAHigh, micALow, micBHigh, micBLow;
        std::array<Biquad,9> eq;
        std::array<float,maxIR> irHistory{};
        int irPos=0;
        std::vector<float> echo, reverb, doubler, pitch;
        size_t pitchWrite=0; float pitchPhase=0;
        size_t echoPos=0,reverbPos=0,doublerPos=0;
        float gateEnv=0, compEnv=0, lastReverb=0;
        void reset() noexcept;
    } channels[2];
    float sampleRate = 48000;
    std::array<float,maxIR> impulse{};
    int impulseLength=0;
    // Derive block-level gains once; avoid expensive per-sample dB conversion.
    float inputGain=1.f, outputGain=1.f, irGain=1.f, gateThreshold=.001f;
    float roomGainA=.5f, roomGainB=.5f;
    float micGainLeft[2] {1.f,1.f}, micGainRight[2] {1.f,1.f};
    float pitchShift(Channel& ch, float x, float semitones) noexcept;
    float runDelay(std::vector<float>& buffer, size_t& pos, float x, float ms, float feedback, float mix) noexcept;
};
} // namespace sdsp
