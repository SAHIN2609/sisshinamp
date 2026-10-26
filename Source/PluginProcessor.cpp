#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>
#include <cstring>

namespace {
using Layout=juce::AudioProcessorValueTreeState::ParameterLayout;
using PF=juce::AudioParameterFloat;
using PB=juce::AudioParameterBool;
using PC=juce::AudioParameterChoice;
void addF(Layout& layout, const char* id, const char* label, float low,float high,float def, float step=.01f) {
    layout.add(std::make_unique<PF>(juce::ParameterID(id,1), label,
        juce::NormalisableRange<float>(low,high,step),def));
}
void addB(Layout& layout,const char* id,const char* label,bool def) {
    layout.add(std::make_unique<PB>(juce::ParameterID(id,1),label,def));
}
void setParam(juce::AudioProcessorValueTreeState& a,const juce::String& id,float value) {
    if(auto* p=a.getParameter(id)) {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(value));
        p->endChangeGesture();
    }
}
}

Layout SishhinDSPAudioProcessor::createLayout() {
    Layout p;
    p.add(std::make_unique<PC>(juce::ParameterID("amp",1),"AMP",juce::StringArray{"Sakura Clean","Taki Metal","Take Thick"},0));
    p.add(std::make_unique<PC>(juce::ParameterID("cab",1),"CAB",juce::StringArray{"1x12 Open","4x12 Modern","2x12 Oversized"},0));
    const juce::StringArray micNames{"Dynamic 57","Ribbon 121","Condenser 87","Dynamic 421"};
    p.add(std::make_unique<PC>(juce::ParameterID("micaType",1),"MIC A MODEL",micNames,0));
    p.add(std::make_unique<PC>(juce::ParameterID("micbType",1),"MIC B MODEL",micNames,1));
    addF(p,"input","INPUT",-12,12,0,.1f);
    addF(p,"gate","GATE",-80,0,-60,.1f);
    addF(p,"trans","TRANSPOSE",-12,12,0,1);
    addF(p,"dbl","DOUBLER",0,30,12,.1f);
    addF(p,"out","OUTPUT",-12,12,0,.1f);
    addF(p,"gain","GAIN",0,10,5);
    addF(p,"bass","BASS",0,10,5);
    addF(p,"mid","MIDDLE",0,10,5);
    addF(p,"treb","TREBLE",0,10,5);
    addF(p,"pres","PRESENCE",0,10,5);
    addF(p,"master","MASTER",0,10,5);
    addF(p,"ao","AMP OUTPUT",0,10,5);
    addF(p,"irlv","IR LEVEL",-12,12,0,.1f);
    addF(p,"irlo","IR LOW CUT",20,500,80,1);
    addF(p,"irhi","IR HIGH CUT",2000,20000,12000,1);
    addF(p,"arm","MIC A ROOM LEVEL",-60,6,-6,.1f);
    addF(p,"brm","MIC B ROOM LEVEL",-60,6,-6,.1f);
    for (auto prefix : {"a","b"}) {
        addF(p,(juce::String(prefix)+"pos").toRawUTF8(), (juce::String(prefix)+" POSITION").toRawUTF8(),0,1,.3f);
        addF(p,(juce::String(prefix)+"dist").toRawUTF8(),(juce::String(prefix)+" DISTANCE").toRawUTF8(),0,1,.1f);
        addF(p,(juce::String(prefix)+"lv").toRawUTF8(),(juce::String(prefix)+" MIC LEVEL").toRawUTF8(),-12,12,0);
        addF(p,(juce::String(prefix)+"pan").toRawUTF8(),(juce::String(prefix)+" PAN").toRawUTF8(),-1,1,0);
    }
    for (int i=0;i<9;++i) {
        const auto id="e"+juce::String(i);
        addF(p,id.toRawUTF8(),("EQ BAND "+juce::String(i+1)).toRawUTF8(),-12,12,0,.1f);
    }
    for(int i=0;i<5;++i) for(int j=0;j<3;++j) {
        const auto id="p"+juce::String(i)+"_"+juce::String(j);
        addF(p,id.toRawUTF8(),id.toRawUTF8(),0,10,5);
    }
    addB(p,"power","AMP POWER",true);
    addB(p,"gateOn","NOISE GATE ON",true);
    addB(p,"eqOn","GRAPHIC EQ ON",true);
    addB(p,"irOn","IR ON",false);
    addB(p,"ma","MIC A ON",true);
    addB(p,"mb","MIC B ON",true);
    addB(p,"raOn","MIC A ROOM ON",false);
    addB(p,"rbOn","MIC B ROOM ON",false);
    addB(p,"doubler","DOUBLER ON",false);
    addB(p,"stereo","STEREO INPUT",false);
    for(int i=0;i<5;++i) {
        const auto id="p"+juce::String(i)+"On";
        addB(p,id.toRawUTF8(),("PEDAL "+juce::String(i)+" ON").toRawUTF8(),false);
    }
    return p;
}

SishhinDSPAudioProcessor::SishhinDSPAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Guitar In",juce::AudioChannelSet::stereo(),true)
                                  .withOutput("Out",juce::AudioChannelSet::stereo(),true)),
      apvts(*this,nullptr,"SISHHINDSP",createLayout()) {selectFactoryPreset(0);}

void SishhinDSPAudioProcessor::prepareToPlay(double sr,int maxBlock) {engine.prepare(sr,maxBlock);floatScratch.setSize(2,maxBlock);}
void SishhinDSPAudioProcessor::releaseResources() {engine.reset();}
bool SishhinDSPAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto in=layouts.getMainInputChannelSet(), out=layouts.getMainOutputChannelSet();
    return (in==juce::AudioChannelSet::mono() || in==juce::AudioChannelSet::stereo()) &&
           (out==juce::AudioChannelSet::mono() || out==juce::AudioChannelSet::stereo());
}
float SishhinDSPAudioProcessor::get(const char* id) const noexcept {
    if(const auto* v=apvts.getRawParameterValue(id)) return v->load();
    return 0.f;
}
sdsp::Parameters SishhinDSPAudioProcessor::collectParameters() const noexcept {
    sdsp::Parameters p;
    p.amp=(int)get("amp");p.cab=(int)get("cab");
    p.micTypeA=(int)get("micaType");p.micTypeB=(int)get("micbType");
    p.input=get("input");p.gate=get("gate");p.trans=get("trans");p.dbl=get("dbl");p.out=get("out");
    p.gain=get("gain");p.bass=get("bass");p.mid=get("mid");p.treb=get("treb");p.pres=get("pres");
    p.master=get("master");p.ao=get("ao");p.irlv=get("irlv");p.irlo=get("irlo");p.irhi=get("irhi");
    p.apos=get("apos");p.adist=get("adist");p.alv=get("alv");p.apan=get("apan");
    p.bpos=get("bpos");p.bdist=get("bdist");p.blv=get("blv");p.bpan=get("bpan");
    p.arm=get("arm");p.brm=get("brm");
    p.roomAOn=get("raOn")>.5f;p.roomBOn=get("rbOn")>.5f;
    for(int i=0;i<9;++i) p.eq[(size_t)i]=get(("e"+juce::String(i)).toRawUTF8());
    for(int i=0;i<5;++i) for(int j=0;j<3;++j)
        p.pedals[(size_t)i][(size_t)j]=get(("p"+juce::String(i)+"_"+juce::String(j)).toRawUTF8());
    p.power=get("power")>.5f;p.gateOn=get("gateOn")>.5f;p.eqOn=get("eqOn")>.5f;
    p.irOn=get("irOn")>.5f;p.ma=get("ma")>.5f;p.mb=get("mb")>.5f;p.doubler=get("doubler")>.5f;
    for(int i=0;i<5;++i) p.pedalOn[(size_t)i]=get(("p"+juce::String(i)+"On").toRawUTF8())>.5f;
    return p;
}
void SishhinDSPAudioProcessor::pushIRToEngine() noexcept {
    if (!impulseUpdate) return;
    if (!irLock.tryEnter()) return;
    if (impulseUpdate) {
        if(pendingLength==0) engine.clearImpulse();
        else engine.setImpulse(pendingImpulse,pendingLength);
        impulseUpdate=false;
    }
    irLock.exit();
}
void SishhinDSPAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals guard;
    for(const auto metadata:midi) {
        const auto msg=metadata.getMessage();
        if(msg.isProgramChange()) {requestedProgram.store(msg.getProgramChangeNumber()%72);triggerAsyncUpdate();}
    }
    pushIRToEngine();
    const auto p=collectParameters();
    engine.configure(p);
    const int chs=b.getNumChannels();
    const int inputs=getTotalNumInputChannels();
    if(chs==0 || inputs==0) { b.clear(); lastInputPeak.store(0.f); lastOutputPeak.store(0.f); return; }
    const bool monoMode=get("stereo")<.5f;
    float inPeak=0.0f, outPeak=0.0f;
    for(int i=0;i<b.getNumSamples();++i) {
        float x0=b.getSample(0,i);
        float x1=(inputs>1 && chs>1)?b.getSample(1,i):x0;
        // Hosts may allocate a stereo bus for a guitar connected only to left.
        // Never halve that signal against an inactive/silent right channel.
        if(monoMode && inputs>1) {
            const auto mono = std::abs(x1)<1.e-6f ? x0
                            : std::abs(x0)<1.e-6f ? x1 : (x0+x1)*.5f;
            x0=x1=mono;
        }
        inPeak=std::max(inPeak,std::max(std::abs(x0),std::abs(x1)));
        const auto y0=engine.processSample(0,x0,p);
        b.setSample(0,i,y0);
        outPeak=std::max(outPeak,std::abs(y0));
        if(chs>1) {
            const auto y1=engine.processSample(1,x1,p);
            b.setSample(1,i,y1);
            outPeak=std::max(outPeak,std::abs(y1));
        }
    }
    for(int c=2;c<chs;++c) b.clear(c,0,b.getNumSamples());
    lastInputPeak.store(inPeak);
    lastOutputPeak.store(outPeak);
}
void SishhinDSPAudioProcessor::processBlock(juce::AudioBuffer<double>& b,juce::MidiBuffer& midi) {
    // JUCE can request double precision. Convert through a reusable float buffer.
    if(floatScratch.getNumSamples()<b.getNumSamples() || floatScratch.getNumChannels()<b.getNumChannels())
        floatScratch.setSize(juce::jmax(2,b.getNumChannels()),b.getNumSamples());
    juce::AudioBuffer<float>& temp=floatScratch;
    for(int c=0;c<b.getNumChannels();++c) for(int i=0;i<b.getNumSamples();++i)
        temp.setSample(c,i,(float)b.getSample(c,i));
    juce::AudioBuffer<float> view(temp.getArrayOfWritePointers(),b.getNumChannels(),b.getNumSamples());
    processBlock(view,midi);
    for(int c=0;c<b.getNumChannels();++c) for(int i=0;i<b.getNumSamples();++i)
        b.setSample(c,i,temp.getSample(c,i));
}
juce::AudioProcessorEditor* SishhinDSPAudioProcessor::createEditor(){return new SishhinDSPAudioProcessorEditor(*this);}

const juce::String SishhinDSPAudioProcessor::getProgramName(int index) {
    static const char* adjectives[]={"Asa","Yuki","Kaze","Tsuki","Hana","Mizu","Kumo","Ame","Hoshi","Kawa","Matsu","Kiri"};
    static const char* tone[][2]={{"Clean","Shimmer"},{"Chug","Lead"},{"Wall","Crush"}};
    const int a=juce::jlimit(0,71,index)/24, i=juce::jlimit(0,71,index)%24;
    return juce::String(adjectives[i%12])+" "+tone[a][i/12];
}
void SishhinDSPAudioProcessor::setCurrentProgram(int index){
    // Hosts sometimes call this off the message thread; defer actual APVTS edits.
    requestedProgram.store(juce::jlimit(0,71,index));
    triggerAsyncUpdate();
}
void SishhinDSPAudioProcessor::handleAsyncUpdate() {
    const int program=requestedProgram.exchange(-1);
    if(program>=0)selectFactoryPreset(program);
}
void SishhinDSPAudioProcessor::selectFactoryPreset(int ix) {
    ix=juce::jlimit(0,71,ix); const int a=ix/24,i=ix%24;
    currentProgram.store(ix);
    setParam(apvts,"amp",(float)a);setParam(apvts,"cab",(float)a);
    setParam(apvts,"power",1.f);setParam(apvts,"gateOn",1.f);setParam(apvts,"eqOn",1.f);
    setParam(apvts,"irOn",0.f);setParam(apvts,"ma",1.f);setParam(apvts,"mb",1.f);
    setParam(apvts,"raOn",0.f);setParam(apvts,"rbOn",0.f);
    setParam(apvts,"arm",-6.f);setParam(apvts,"brm",-6.f);
    setParam(apvts,"micaType",(float)((i/6)%4));setParam(apvts,"micbType",(float)((i/8+1)%4));
    setParam(apvts,"stereo",0.f);setParam(apvts,"doubler",i%5==4?1.f:0.f);
    setParam(apvts,"input",0.f);setParam(apvts,"out",0.f);setParam(apvts,"trans",0.f);
    // deterministic preset values, with audible differences
    const float q=(float)((i*17+a*7)%11)/10.f;
    setParam(apvts,"gain", (a==0?2.0f:a==1?7.3f:5.8f)+q*(a==0?2.4f:1.6f));
    setParam(apvts,"bass", a==2?7.5f:a==1?6.7f:5.f);
    setParam(apvts,"mid", a==1?3.8f:a==2?6.9f:5.2f);
    setParam(apvts,"treb", a==0?6.4f:a==1?6.0f:4.8f);
    setParam(apvts,"pres", a==1?6.7f:4.6f);
    setParam(apvts,"master",5.5f);setParam(apvts,"ao",5.f);
    setParam(apvts,"gate",a==1?-48.f:-64.f);
    setParam(apvts,"p0On",i%7==0?1.f:0.f);
    setParam(apvts,"p1On",a==1?1.f:0.f);
    setParam(apvts,"p2On",a==2?1.f:0.f);
    setParam(apvts,"p3On",i>=12?1.f:0.f);
    setParam(apvts,"p4On",(i%6)>=4?1.f:0.f);
    const float shape=((i*13+a*7)%17-8)*.34f;
    for(int band=0;band<9;++band)
        setParam(apvts,"e"+juce::String(band),((band+i)%3-1)*shape*.45f);
    for(int pedal=0;pedal<5;++pedal) for(int dial=0;dial<3;++dial)
        setParam(apvts,"p"+juce::String(pedal)+"_"+juce::String(dial),
                 juce::jlimit(0.f,10.f,4.5f+(float)((i+pedal*3+dial*7)%9)*.46f));
    setParam(apvts,"apos",.12f+.07f*(float)(i%5));
    setParam(apvts,"bpos",.45f+.06f*(float)(i%5));
    setParam(apvts,"adist",.04f+.04f*(float)(i%7));
    setParam(apvts,"bdist",.15f+.05f*(float)(i%6));
    setParam(apvts,"alv",0.f);setParam(apvts,"blv",-3.f);
    setParam(apvts,"apan",-.3f);setParam(apvts,"bpan",.3f);
}
void SishhinDSPAudioProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto state=apvts.copyState();
    {
        const juce::ScopedLock lock(irLock);
        state.setProperty("irName",irName,nullptr);
        state.setProperty("factoryProgram",currentProgram.load(),nullptr);
        state.setProperty("factoryIRIndex",currentFactoryIR.load(),nullptr);
        juce::MemoryBlock raw(pendingImpulse.data(),(size_t)pendingLength*sizeof(float));
        state.setProperty("irTaps",raw.toBase64Encoding(),nullptr);
    }
    if(auto xml=state.createXml()) copyXmlToBinary(*xml,dest);
}
void SishhinDSPAudioProcessor::setStateInformation(const void* data,int size) {
    if(auto xml=getXmlFromBinary(data,size)) {
        if(xml->hasTagName(apvts.state.getType())) {
            auto state=juce::ValueTree::fromXml(*xml);
            {
                const juce::ScopedLock lock(irLock);
                irName=state.getProperty("irName").toString();
                juce::MemoryBlock mb;
                mb.fromBase64Encoding(state.getProperty("irTaps").toString());
                pendingImpulse.fill(0);
                pendingLength=juce::jmin(sdsp::maxIR,(int)(mb.getSize()/sizeof(float)));
                if(pendingLength) std::memcpy(pendingImpulse.data(),mb.getData(),(size_t)pendingLength*sizeof(float));
                impulseUpdate=true;
            }
            apvts.replaceState(state);
            currentProgram.store(juce::jlimit(0,71,(int)state.getProperty("factoryProgram",0)));
            currentFactoryIR.store(juce::jlimit(-1,71,(int)state.getProperty("factoryIRIndex",-1)));
        }
    }
}
bool SishhinDSPAudioProcessor::loadFactoryIR(int index) {
    if(index<0 || index>=72) return false;
    constexpr size_t bytesPerIR=(size_t)sdsp::maxIR*sizeof(float);
    if((size_t)BinaryData::ir_bank_short_binSize < 72*bytesPerIR) return false;
    std::array<float,sdsp::maxIR> taps {};
    std::memcpy(taps.data(),
                BinaryData::ir_bank_short_bin + (size_t)index*bytesPerIR,
                bytesPerIR);
    static constexpr const char* cabs[]={"1x12_Open","4x12_Modern","2x12_Oversized"};
    static constexpr const char* mics[]={"Dynamic57","Ribbon121","Condenser87","Dynamic421"};
    static constexpr const char* positions[]={"Center","CapEdge","Cone","Mid","Edge","Room"};
    const int cab=index/24, mic=(index/6)%4, position=index%6;
    {
        const juce::ScopedLock lock(irLock);
        pendingImpulse=taps;
        pendingLength=sdsp::maxIR;
        impulseUpdate=true;
        irName=juce::String(cabs[cab])+"__"+mics[mic]+"__"+positions[position]+".wav";
        currentFactoryIR.store(index);
    }
    setParam(apvts,"irOn",1.f);
    return true;
}
bool SishhinDSPAudioProcessor::loadIRFile(const juce::File& file,juce::String& error) {
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if(!reader){error="Unsupported IR. Choose a WAV or AIFF file.";return false;}
    if(reader->lengthInSamples<=0){error="IR is empty.";return false;}
    // Limited FIR cabinet loader. Resample the front of the IR to host rate,
    // preserving transient shape. Long-room tails are intentionally truncated.
    const double hostRate=getSampleRate()>0?getSampleRate():reader->sampleRate;
    const double ratio=reader->sampleRate/hostRate;
    const int wanted=juce::jlimit(1,sdsp::maxIR*4,(int)std::ceil(sdsp::maxIR*ratio)+2);
    const int readCount=(int)juce::jmin<juce::int64>(wanted,reader->lengthInSamples);
    juce::AudioBuffer<float> temp((int)juce::jmin(2u,reader->numChannels),readCount);
    if(!reader->read(&temp,0,readCount,0,true,true)){error="Could not decode IR.";return false;}
    const int length=juce::jmin(sdsp::maxIR,(int)std::ceil(readCount/ratio));
    std::array<float,sdsp::maxIR> taps{};
    float max=0;
    for(int i=0;i<length;++i) {
        const double readPos=i*ratio;
        const int a=juce::jlimit(0,readCount-1,(int)readPos),b=juce::jmin(readCount-1,a+1);
        const float frac=(float)(readPos-a);
        taps[(size_t)i]=temp.getSample(0,a)*(1-frac)+temp.getSample(0,b)*frac;
        max=std::max(max,std::abs(taps[(size_t)i]));
    }
    if(max<1e-9f){error="IR contains silence.";return false;}
    for(int i=0;i<length;++i) taps[(size_t)i]*=.8f/max;
    {
        const juce::ScopedLock lock(irLock);
        pendingImpulse=taps;pendingLength=length;impulseUpdate=true;irName=file.getFileName();
        currentFactoryIR.store(-1);
    }
    setParam(apvts,"irOn",1);
    return true;
}
bool SishhinDSPAudioProcessor::loadIRData(const juce::MemoryBlock& data,
                                               const juce::String& filename,
                                               juce::String& error) {
    const auto extension=filename.fromLastOccurrenceOf(".",true,false).toLowerCase();
    if (extension!=".wav" && extension!=".aif" && extension!=".aiff") {
        error="Choose a WAV/AIFF impulse response.";
        return false;
    }
    const auto file=juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getNonexistentChildFile("SishhinDSP_IR",extension,false);
    if (!file.replaceWithData(data.getData(),data.getSize())) {
        error="Unable to write temporary cabinet IR.";
        return false;
    }
    const bool ok=loadIRFile(file,error);
    file.deleteFile();
    if (ok) {
        const juce::ScopedLock lock(irLock);
        irName=filename;
    }
    return ok;
}
void SishhinDSPAudioProcessor::clearIR(){
    const juce::ScopedLock lock(irLock);
    pendingLength=0;impulseUpdate=true;irName.clear();currentFactoryIR.store(-1);
}
juce::String SishhinDSPAudioProcessor::currentIRName() const {
    const juce::ScopedLock lock(irLock);
    return irName;
}
juce::File SishhinDSPAudioProcessor::presetFolder() {
    auto dir=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                  .getChildFile("SishhinDSP").getChildFile("Presets");
    dir.createDirectory();return dir;
}
bool SishhinDSPAudioProcessor::saveUserPreset(const juce::String& name) {
    const auto filename=juce::File::createLegalFileName(name)+".sdsp";
    juce::MemoryBlock state;
    getStateInformation(state);
    return state.getSize()>0 && presetFolder().getChildFile(filename).replaceWithData(state.getData(),state.getSize());
}
bool SishhinDSPAudioProcessor::deleteUserPreset(const juce::String& name) {
    const auto file=presetFolder().getChildFile(juce::File::createLegalFileName(name)+".sdsp");
    return file.existsAsFile() && file.deleteFile();
}
bool SishhinDSPAudioProcessor::loadUserPreset(const juce::String& name) {
    auto file=presetFolder().getChildFile(juce::File::createLegalFileName(name)+".sdsp");
    juce::MemoryBlock state;
    if(!file.existsAsFile() || !file.loadFileAsData(state))return false;
    setStateInformation(state.getData(),(int)state.getSize());return true;
}
juce::StringArray SishhinDSPAudioProcessor::userPresetNames() const {
    juce::StringArray result;
    for(auto f:presetFolder().findChildFiles(juce::File::findFiles,false,"*.sdsp")) result.add(f.getFileNameWithoutExtension());
    result.sort(true);return result;
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new SishhinDSPAudioProcessor();}
