#include "../Source/TunerDSP.h"
#include <array>
#include <cmath>
#include <cstdio>

int main() {
    constexpr int count=2048;
    constexpr float rate=12000.f;
    constexpr float pi=3.14159265358979323846f;
    std::array<float,count> buffer{};
    bool success=true;
    const std::array<float,11> notes={{
        36.71f,41.2034f,55.f,61.7354f,82.4069f,110.f,146.832f,
        196.f,246.942f,440.f,880.f
    }};
    for(const float note:notes) {
        for(int i=0;i<count;++i) {
            const float t=(float)i/rate;
            buffer[(size_t)i]=.10f*std::sin(2.f*pi*note*t)
                             +.025f*std::sin(4.f*pi*note*t+.22f)
                             +.012f*std::sin(6.f*pi*note*t-.17f);
        }
        const float detected=sdsp::detectGuitarPitchHz(buffer.data(),count,rate);
        const float cents=detected>0.f?1200.f*std::log2(detected/note):999.f;
        const bool okay=std::isfinite(cents)&&std::abs(cents)<16.f;
        success=success&&okay;
        std::printf("%s  %.2f Hz -> %.2f Hz (%+.1f cents)\n",
                    okay?"PASS":"FAIL",note,detected,cents);
    }
    buffer.fill(0.f);
    success=success&&(sdsp::detectGuitarPitchHz(buffer.data(),count,rate)==0.f);
    for(int i=0;i<count;++i)buffer[(size_t)i]=.00008f*std::sin(2.f*pi*110.f*i/rate);
    success=success&&(sdsp::detectGuitarPitchHz(buffer.data(),count,rate)==0.f);
    if(!success)std::fprintf(stderr,"Guitar tuner regression tests failed.\n");
    return success?0:1;
}
