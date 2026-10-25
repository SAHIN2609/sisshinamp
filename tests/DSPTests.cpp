#include "../Source/AmpEngine.h"
#include <cmath>
#include <cstdio>
#include <vector>

struct Stats { double rms=0, peak=0; };
Stats run(int amp, float gain, bool impulse=false, float shift=0, bool micB=true) {
    sdsp::AmpEngine e;
    e.prepare(48000,512);
    sdsp::Parameters p;
    p.amp=amp; p.gain=gain; p.gateOn=false; p.eqOn=true;p.trans=shift;p.mb=micB;
    if(impulse) {
        std::array<float,sdsp::maxIR> ir{};
        ir[0]=.75f; ir[10]=-.12f; ir[50]=.08f;
        e.setImpulse(ir, 100); p.irOn=true;
    }
    e.configure(p);
    double sum=0, pk=0;
    for(int i=0;i<48000;++i) {
        const float t=i/48000.f;
        const float input=.095f*std::sin(2*sdsp::pi*110*t)+.035f*std::sin(2*sdsp::pi*440*t);
        const float y=e.processSample(0,input,p);
        if(!std::isfinite(y))return {-1,-1};
        if(i>4096) {sum+=y*y; pk=std::max(pk,static_cast<double>(std::abs(y)));}
    }
    return {std::sqrt(sum/(48000-4096)),pk};
}
int main() {
    const auto c=run(0,3), m=run(1,9), t=run(2,7), ir=run(1,9,true);
    const auto shift=run(0,3,false,7), mic=run(0,3,false,0,false);
    std::printf("Clean RMS %.6f; Metal RMS %.6f; Thick RMS %.6f; IR RMS %.6f; shifted RMS %.6f; one-mic RMS %.6f\n",c.rms,m.rms,t.rms,ir.rms,shift.rms,mic.rms);
    const bool ok=c.rms>1.e-5&&m.rms>1.e-5&&t.rms>1.e-5&&shift.rms>1.e-5&&mic.rms>1.e-5&&
        std::abs(c.rms-m.rms)>1.e-4&&std::abs(m.rms-t.rms)>1.e-4&&
        std::abs(ir.rms-m.rms)>1.e-4&&std::abs(c.rms-mic.rms)>1.e-4;
    if(!ok)std::fprintf(stderr,"DSP test failed.\n");
    return ok?0:1;
}
