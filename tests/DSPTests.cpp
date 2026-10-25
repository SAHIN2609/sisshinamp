#include "../Source/AmpEngine.h"
#include <cmath>
#include <cstdio>
#include <array>

struct Stats { double rms=0, peak=0, avgDiff=0; bool finite=true; };
Stats run(int amp, float gain, bool impulse=false, float shift=0, bool micB=true,
          int micAType=0, bool bypass=false, int cab=0, float eq=0, bool pedal=false) {
    sdsp::AmpEngine e;
    e.prepare(48000,512);
    sdsp::Parameters p;
    p.amp=amp;p.cab=cab;p.gain=gain;p.gateOn=false;p.eqOn=true;
    p.trans=shift;p.mb=micB;p.micTypeA=micAType;p.power=!bypass;
    p.eq[5]=eq;p.pedalOn[1]=pedal;
    if(impulse) {
        std::array<float,sdsp::maxIR> ir{};
        ir[0]=.75f;ir[10]=-.12f;ir[50]=.08f;
        e.setImpulse(ir,100);p.irOn=true;
    }
    e.configure(p);
    double sum=0,pk=0,diff=0,prev=0;
    bool finite=true;
    for(int i=0;i<48000;++i) {
        const float t=i/48000.f;
        const float input=.095f*std::sin(2*sdsp::pi*110*t)+.035f*std::sin(2*sdsp::pi*440*t);
        const float y=e.processSample(0,input,p);
        finite=finite && std::isfinite(y);
        if(i>4096) {
            sum+=y*y;
            pk=std::max(pk,static_cast<double>(std::abs(y)));
            diff+=std::abs(y-prev);
        }
        prev=y;
    }
    return {std::sqrt(sum/(48000-4096)),pk,diff/(48000-4096),finite};
}
int main() {
    const auto c=run(0,3),m=run(1,9),t=run(2,7);
    const auto ir=run(1,9,true),shift=run(0,3,false,7),singleMic=run(0,3,false,0,false);
    const auto ribbon=run(0,3,false,0,true,1),bypass=run(0,3,false,0,true,0,true);
    const auto cabinet=run(0,3,false,0,true,0,false,2);
    const auto eq=run(0,3,false,0,true,0,false,0,8);
    const auto pedal=run(0,3,false,0,true,0,false,0,0,true);
    const std::array<Stats,11> stats{c,m,t,ir,shift,singleMic,ribbon,bypass,cabinet,eq,pedal};
    bool ok=true;
    for(const auto& s:stats)ok=ok && s.finite && s.rms>1.e-5 && s.peak<1.35;
    const auto distinct=[](double a,double b){return std::abs(a-b)>1.e-4;};
    ok=ok && distinct(c.rms,m.rms)&&distinct(m.rms,t.rms)&&distinct(ir.rms,m.rms)
          &&distinct(c.rms,singleMic.rms)&&std::abs(ribbon.avgDiff-c.avgDiff)>1.e-6
          &&distinct(c.rms,cabinet.rms)&&distinct(c.rms,eq.rms)&&distinct(c.rms,pedal.rms)
          &&distinct(c.rms,bypass.rms);
    std::printf("Clean RMS %.6f; Metal %.6f; Thick %.6f; IR %.6f; shifted %.6f; ribbon %.6f; bypass %.6f; cab %.6f; eq %.6f; drive %.6f\n",
                c.rms,m.rms,t.rms,ir.rms,shift.rms,ribbon.rms,bypass.rms,cabinet.rms,eq.rms,pedal.rms);
    if(!ok)std::fprintf(stderr,"SishhinDSP DSP regression test failed.\n");
    return ok?0:1;
}
