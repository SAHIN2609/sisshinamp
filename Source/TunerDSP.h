#pragma once
// Portable guitar tuner algorithm, independent of JUCE and the browser UI.
// Input is a decimated raw DI window, usually 2048 samples at ~12 kHz.
#include <array>
#include <algorithm>
#include <cmath>

namespace sdsp {
inline float detectGuitarPitchHz(const float* data, int count, float sampleRate) noexcept {
    if(data==nullptr || count<1024 || sampleRate<4000.f)return 0.f;
    // Supports seven/eight-string guitar tuning from ~35 Hz through 1100 Hz.
    constexpr int maxSearch=360;
    const int minLag=std::max(2,(int)(sampleRate/1100.f));
    const int maxLag=std::min(maxSearch,(int)(sampleRate/35.f));
    const int window=count-maxLag-1;
    if(window<700 || minLag>=maxLag)return 0.f;
    double mean=0.;
    for(int i=0;i<count;++i)mean+=data[i];
    mean/=count;
    std::array<float,2048> centered{};
    if(count>(int)centered.size())return 0.f;
    double energy=0.;
    for(int i=0;i<count;++i) {
        centered[(size_t)i]=data[i]-(float)mean;
        energy+=(double)centered[(size_t)i]*centered[(size_t)i];
    }
    if(energy/count<.000004)return 0.f; // no audible input signal

    std::array<double,maxSearch+1> normalized{};
    double accumulated=0.;
    for(int tau=1;tau<=maxLag;++tau) {
        double squared=0.;
        for(int i=0;i<window;++i) {
            const double delta=(double)centered[(size_t)i]
                               -(double)centered[(size_t)(i+tau)];
            squared+=delta*delta;
        }
        accumulated+=squared;
        normalized[(size_t)tau]=squared*tau/(accumulated+1.e-14);
    }
    // YIN pitch selection: do not stop at the first threshold crossing,
    // because that reports pitches up to ~10% too sharp. Find the actual
    // local minimum after crossing the threshold.
    int found=0, fallback=0;
    double best=.32;
    for(int tau=minLag;tau<=maxLag;++tau) {
        const double confidence=normalized[(size_t)tau];
        if(confidence<best) {
            best=confidence;
            fallback=tau;
        }
        if(confidence<.16) {
            while(tau+1<=maxLag &&
                  normalized[(size_t)(tau+1)]<normalized[(size_t)tau])++tau;
            found=tau;
            break;
        }
    }
    if(found==0 && best<.30)found=fallback;
    if(found==0)return 0.f;

    double period=found;
    if(found>1 && found<maxLag) {
        const double left=normalized[(size_t)(found-1)];
        const double center=normalized[(size_t)found];
        const double right=normalized[(size_t)(found+1)];
        const double denominator=left-2*center+right;
        if(std::abs(denominator)>1.e-12)
            period+=std::max(-.5,std::min(.5,(left-right)*.5/denominator));
    }
    if(period<=0.)return 0.f;
    const float hz=(float)(sampleRate/period);
    return std::isfinite(hz) && hz>=35.f && hz<=1100.f ? hz:0.f;
}
} // namespace sdsp
