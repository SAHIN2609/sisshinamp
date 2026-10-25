#include "AmpEngine.h"

namespace sdsp {
namespace {
float normFreq(float sr, float hz) noexcept { return 2.0f*pi*clamp(hz, 10, sr*.475f)/sr; }
float soft(float x) noexcept { return std::tanh(x); }
}

void Biquad::coefficients(float B0,float B1,float B2,float A0,float A1,float A2) noexcept {
    A0 = std::max(1.e-12f, A0);
    b0=B0/A0; b1=B1/A0; b2=B2/A0; a1=A1/A0; a2=A2/A0;
}
void Biquad::lowpass(float sr, float freq, float Q) noexcept {
    const float w=normFreq(sr,freq), c=std::cos(w), a=std::sin(w)/(2*Q);
    coefficients((1-c)*.5f,1-c,(1-c)*.5f,1+a,-2*c,1-a);
}
void Biquad::highpass(float sr, float freq, float Q) noexcept {
    const float w=normFreq(sr,freq), c=std::cos(w), a=std::sin(w)/(2*Q);
    coefficients((1+c)*.5f,-(1+c),(1+c)*.5f,1+a,-2*c,1-a);
}
void Biquad::peak(float sr, float freq, float Q, float db) noexcept {
    const float w=normFreq(sr,freq), c=std::cos(w), a=std::sin(w)/(2*std::max(.2f,Q));
    const float A=std::pow(10.f,db/40.f);
    coefficients(1+a*A,-2*c,1-a*A,1+a/A,-2*c,1-a/A);
}
void Biquad::shelf(float sr, float freq, float db, bool high) noexcept {
    const float w=normFreq(sr,freq), c=std::cos(w), s=std::sin(w), A=std::pow(10.f,db/40.f);
    const float q=2*std::sqrt(A)*s*.70710678f;
    if(high) coefficients(A*((A+1)+(A-1)*c+q),-2*A*((A-1)+(A+1)*c),A*((A+1)+(A-1)*c-q),
                          (A+1)-(A-1)*c+q,2*((A-1)-(A+1)*c),(A+1)-(A-1)*c-q);
    else coefficients(A*((A+1)-(A-1)*c+q),2*A*((A-1)-(A+1)*c),A*((A+1)-(A-1)*c-q),
                      (A+1)+(A-1)*c+q,-2*((A-1)+(A+1)*c),(A+1)+(A-1)*c-q);
}
void AmpEngine::Channel::reset() noexcept {
    inputHigh.reset(); bass.reset(); mid.reset(); high.reset(); presence.reset(); body.reset();
    cabinetHigh.reset(); cabinetLow.reset(); irHigh.reset(); irLow.reset();
    micAHigh.reset(); micALow.reset(); micBHigh.reset(); micBLow.reset();
    for(auto& band:eq) band.reset();
    irHistory.fill(0); irPos=0;
    std::fill(echo.begin(), echo.end(), 0.f);
    std::fill(reverb.begin(), reverb.end(), 0.f);
    std::fill(doubler.begin(), doubler.end(), 0.f);
    std::fill(pitch.begin(), pitch.end(), 0.f);
    pitchWrite=0;pitchPhase=0;
    echoPos=reverbPos=doublerPos=0; gateEnv=compEnv=lastReverb=0;
}
void AmpEngine::prepare(double sr, int /*maxBlockSize*/) {
    sampleRate = clamp(static_cast<float>(sr), 8000, 192000);
    for(auto& c:channels) {
        c.echo.assign(static_cast<size_t>(sampleRate*1.25f)+2, 0.f);
        c.reverb.assign(static_cast<size_t>(sampleRate*.45f)+2, 0.f);
        c.doubler.assign(static_cast<size_t>(sampleRate*.07f)+2, 0.f);
        c.pitch.assign(static_cast<size_t>(sampleRate*.3f)+2,0.f);
        c.reset();
    }
}
void AmpEngine::reset() noexcept { for(auto& c:channels) c.reset(); }
void AmpEngine::configure(const Parameters& p) noexcept {
    inputGain=decibels(p.input);
    const float master=(.05f+.095f*clamp(p.master,0.f,10.f))*(.1f+.09f*clamp(p.ao,0.f,10.f));
    outputGain=master*decibels(p.out);
    irGain=decibels(p.irlv);
    roomGainA=decibels(p.arm);roomGainB=decibels(p.brm);
    gateThreshold=decibels(p.gate);
    const float gains[2]={decibels(p.alv)*(.9f-.32f*p.apos),decibels(p.blv)*(.9f-.32f*p.bpos)};
    const float pans[2]={p.apan,p.bpan};
    for(int i=0;i<2;++i) {
        micGainLeft[i]=gains[i]*clamp(1.f-pans[i]*.65f,.2f,1.7f);
        micGainRight[i]=gains[i]*clamp(1.f+pans[i]*.65f,.2f,1.7f);
    }
    constexpr float freqs[]={65,125,250,500,1000,2000,4000,8000,16000};
    const float lp[]={9600.f,6700.f,4900.f};
    const float hp[]={65.f,90.f,72.f};
    const int cab=std::max(0,std::min(2,p.cab));
    for(auto& c:channels) {
        c.inputHigh.highpass(sampleRate, hp[std::max(0,std::min(2,p.amp))]);
        c.bass.shelf(sampleRate,140, (p.bass-5)*2.4f + (p.amp==2?3.f:0.f),false);
        c.mid.peak(sampleRate, 750, .8f,(p.mid-5)*2.4f+(p.amp==2?2.f:-.5f));
        c.high.shelf(sampleRate,3000,(p.treb-5)*2.1f,true);
        c.presence.peak(sampleRate,3900,.7f,(p.pres-5)*1.7f);
        c.body.peak(sampleRate, (cab==2?140.f:cab==1?240.f:340.f),.9f, cab==2?4.f:cab==1?2.f:0.f);
        c.cabinetHigh.lowpass(sampleRate,std::min(lp[cab],p.irhi));
        c.cabinetLow.highpass(sampleRate,std::max(25.f,p.irlo));
        c.irHigh.lowpass(sampleRate, p.irhi);
        c.irLow.highpass(sampleRate,p.irlo);
        // Four microphone responses matching the original HTML options.
        constexpr float micTop[]={7900.f,5400.f,12000.f,8800.f};
        constexpr float micBottom[]={65.f,42.f,85.f,70.f};
        const int ia=std::max(0,std::min(3,p.micTypeA)),ib=std::max(0,std::min(3,p.micTypeB));
        c.micAHigh.lowpass(sampleRate,clamp(micTop[ia]-p.adist*4400.f-p.apos*1700.f,1500.f,15000.f));
        c.micALow.highpass(sampleRate,clamp(micBottom[ia]+p.adist*95.f,30.f,400.f));
        c.micBHigh.lowpass(sampleRate,clamp(micTop[ib]-p.bdist*4200.f-p.bpos*1700.f,1500.f,15000.f));
        c.micBLow.highpass(sampleRate,clamp(micBottom[ib]+p.bdist*100.f,30.f,400.f));
        for(int i=0;i<9;++i) c.eq[i].peak(sampleRate,freqs[i],1.0f,p.eq[static_cast<size_t>(i)]);
    }
}
void AmpEngine::setImpulse(const std::array<float,maxIR>& taps, int length) noexcept {
    impulse=taps; impulseLength=std::max(0,std::min(maxIR,length));
    for(auto& ch:channels){ch.irHistory.fill(0);ch.irPos=0;}
}
void AmpEngine::clearImpulse() noexcept { impulseLength=0; }

float AmpEngine::runDelay(std::vector<float>& b, size_t& pos, float x, float ms, float feedback, float mix) noexcept {
    if(b.empty()) return x;
    const size_t delay=std::max<size_t>(1,std::min(b.size()-1, static_cast<size_t>(ms*.001f*sampleRate)));
    const size_t read=(pos+b.size()-delay)%b.size();
    const float past=b[read];
    b[pos]=x+past*clamp(feedback,0,.84f);
    pos=(pos+1)%b.size();
    return x*(1-mix)+past*mix;
}
float AmpEngine::pitchShift(Channel& ch, float x, float semitones) noexcept {
    if (std::abs(semitones)<.5f || ch.pitch.empty()) return x;
    auto& line=ch.pitch;
    const int n=(int)line.size();
    line[ch.pitchWrite]=x;
    const float ratio=std::pow(2.f,semitones/12.f);
    const float window=sampleRate*.055f;
    ch.pitchPhase+=std::abs(1.f-ratio)/window;
    ch.pitchPhase-=std::floor(ch.pitchPhase);
    auto voice=[&](float offset) {
        float phase=ch.pitchPhase+offset;phase-=std::floor(phase);
        const float delay=window*(1.f+(ratio>1.f?1.f-phase:phase));
        float read=(float)ch.pitchWrite-delay;
        while(read<0)read+=n;
        const int a=(int)read%n,b=(a+1)%n;
        const float v=line[(size_t)a]*(1.f-(read-std::floor(read)))+line[(size_t)b]*(read-std::floor(read));
        const float envelope=std::sin(pi*phase);
        return v*envelope*envelope;
    };
    const float y=voice(0.f)+voice(.5f);
    ch.pitchWrite=(ch.pitchWrite+1)%line.size();
    return y;
}
float AmpEngine::processSample(int chIndex,float x,const Parameters& p) noexcept {
    if(!p.power) return x; // power is a hard true bypass
    Channel& ch=channels[std::max(0,std::min(1,chIndex))];
    x *= inputGain;
    x=ch.inputHigh.tick(x);
    if(std::abs(p.trans)>.5f)x=pitchShift(ch,x,p.trans);
    // Envelope-following gate: soft attenuation, so it does not hard chop sustain.
    ch.gateEnv=std::max(std::abs(x),ch.gateEnv*.9992f);
    if(p.gateOn) {
        const float threshold=gateThreshold;
        const float gateGain=clamp(ch.gateEnv/(threshold+1.e-8f),0.035f,1.f);
        x*=gateGain;
    }
    if(p.pedalOn[0]) { // BLOOM: optical-style sustained compression
        ch.compEnv=std::max(std::abs(x),ch.compEnv*.99975f);
        const float amount=p.pedals[0][0]/10.f;
        const float comp=1.f/(1.f+ch.compEnv*(1.5f+18.f*amount));
        const float wet=x*comp*(1.3f+3.5f*amount)*(.4f+p.pedals[0][2]*.12f);
        x=x*(1-p.pedals[0][1]/10.f)+wet*(p.pedals[0][1]/10.f);
    }
    if(p.pedalOn[2]) { // WEIGHT: low-mid saturation + boost
        float warm=soft(x*(1.f+p.pedals[2][0]*.18f));
        x+=warm*(p.pedals[2][1]/10.f)*.32f;
        x*=1.f+p.pedals[2][2]*.12f;
    }
    if(p.pedalOn[1]) { // EDGE: preamp overdrive
        const float amount=p.pedals[1][0]/10.f;
        const float distorted=soft(x*(1.3f+amount*13.f))*(.65f+.065f*p.pedals[1][2]);
        x=x*(1-amount*.86f)+distorted*amount*.86f;
    }
    const float drive=clamp(p.gain,0,10)/10.f;
    const int amp=std::max(0,std::min(2,p.amp));
    if(amp==0) { // SAKURA CLEAN: high headroom, soft knee, preserved attack
        x=soft(x*(1.f+drive*2.7f)) / (1.f+drive*.55f);
        x*=.78f;
    } else if(amp==1) { // TAKI METAL: boosted preamp, tight multi-stage clipping
        const float boosted=soft(x*(3.f+drive*20.f));
        const float stage2=soft((boosted*.86f+x*.20f)*(2.6f+drive*8.f));
        x=soft(stage2*2.5f)*.43f;
    } else { // TAKE THICK: asymmetrical, low-mid-rich saturation
        const float warm=soft(x*(2.3f+drive*7.5f)+.17f)-soft(.17f);
        x=soft(warm*1.7f+x*.3f)*.64f;
    }
    x=ch.bass.tick(x);
    x=ch.mid.tick(x);
    x=ch.high.tick(x);
    x=ch.presence.tick(x);
    x=ch.body.tick(x);
    x=ch.cabinetLow.tick(ch.cabinetHigh.tick(x));
    // Parallel simulated microphones; both are controllable from the CAB page.
    const auto* gains=chIndex==0?micGainLeft:micGainRight;
    float a=p.ma?ch.micALow.tick(ch.micAHigh.tick(x))*gains[0]:0.f;
    float b=p.mb?ch.micBLow.tick(ch.micBHigh.tick(x))*gains[1]:0.f;
    x=(a+b)*(p.ma&&p.mb?.60f:1.f);
    const float roomSend = (p.roomAOn ? a*roomGainA : 0.f)
                         + (p.roomBOn ? b*roomGainB : 0.f);
    if(p.irOn && impulseLength>0) { // limited first 256 IR taps, zero allocation in audio callback
        ch.irHistory[static_cast<size_t>(ch.irPos)]=x;
        float sum=0;
        for(int i=0;i<impulseLength;++i) {
            const int ix=(ch.irPos+maxIR-i)%maxIR;
            sum+=ch.irHistory[static_cast<size_t>(ix)]*impulse[static_cast<size_t>(i)];
        }
        ch.irPos=(ch.irPos+1)%maxIR;
        x=ch.irLow.tick(ch.irHigh.tick(sum*irGain));
    }
    if(p.eqOn) for(auto& band:ch.eq) x=band.tick(x);
    if(p.doubler) x=runDelay(ch.doubler,ch.doublerPos,x,clamp(p.dbl,1,30),0,.18f);
    if(p.pedalOn[3]) {
        const float time=70.f+p.pedals[3][0]*75.f;
        const float fb=p.pedals[3][1]*.075f;
        const float mix=p.pedals[3][2]*.055f;
        x=runDelay(ch.echo,ch.echoPos,x,time,fb,mix);
    }
    if(p.pedalOn[4] || p.roomAOn || p.roomBOn) {
        const float decay=p.pedalOn[4] ? p.pedals[4][0]/10.f : .35f;
        const float time=80.f+decay*120.f;
        const float feedback=.25f+decay*.45f;
        const float wet=runDelay(ch.reverb,ch.reverbPos,
                                 (p.pedalOn[4]?x:0.f)+roomSend,
                                 time,feedback,1.f);
        const float haloMix=p.pedalOn[4]?clamp(p.pedals[4][2]*.047f,0.f,.47f):0.f;
        x=x*(1.f-haloMix)+wet*haloMix;
        if(p.roomAOn || p.roomBOn) x+=wet*.22f;
    }
    x*=outputGain;
    return std::isfinite(x)?clamp(x,-1.3f,1.3f):0.f;
}
} // namespace sdsp
