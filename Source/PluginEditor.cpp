#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace {
const auto ink=juce::Colour(0xff2b3040);
const auto grey=juce::Colour(0xff6a7086);
const auto red=juce::Colour(0xffc8312b);
const auto cream=juce::Colour(0xfff4f1e9);
const auto pink=juce::Colour(0xffe9beca);
const auto deep=juce::Colour(0xff10172e);
const auto green=juce::Colour(0xff2f5034);
float param(const SishhinDSPAudioProcessor& p,const char* id) {
    if(auto* v=p.apvts.getRawParameterValue(id))return v->load();
    return 0;
}
void caption(juce::Graphics& g,const juce::String& text,juce::Rectangle<float> r,float pt=11,juce::Colour col=grey) {
    g.setColour(col);g.setFont(juce::Font(juce::FontOptions(pt,juce::Font::bold)));
    g.drawFittedText(text,r.toNearestInt(),juce::Justification::centred,1);
}
void roundedPanel(juce::Graphics&g,juce::Rectangle<float>r,juce::Colour bg,float radius=12) {
    g.setColour(juce::Colours::black.withAlpha(.13f));g.fillRoundedRectangle(r.translated(0,8),radius);
    g.setColour(bg);g.fillRoundedRectangle(r,radius);
}
}

void SishhinDSPAudioProcessorEditor::Style::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,
    float position,float start,float end,juce::Slider& slider) {
    auto r=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(5);
    const float diameter=std::min(r.getWidth(),r.getHeight());
    r=r.withSizeKeepingCentre(diameter,diameter);
    const bool dark=slider.getProperties().getWithDefault("dark",false);
    g.setColour(juce::Colours::black.withAlpha(.2f));g.fillEllipse(r.translated(0,3));
    g.setGradientFill(juce::ColourGradient(dark?juce::Colour(0xff666977):juce::Colours::white,r.getX(),r.getY(),
                                           dark?juce::Colour(0xff171922):juce::Colour(0xffbbc1d0),r.getRight(),r.getBottom(),false));
    g.fillEllipse(r);
    g.setColour(dark?juce::Colours::white.withAlpha(.2f):ink.withAlpha(.12f));g.drawEllipse(r,1);
    const float a=start+position*(end-start);
    juce::Path mark;
    const float cx=r.getCentreX(),cy=r.getCentreY();
    const float rad=diameter*.32f;
    mark.startNewSubPath(cx+std::sin(a)*rad*.28f,cy-std::cos(a)*rad*.28f);
    mark.lineTo(cx+std::sin(a)*rad,cy-std::cos(a)*rad);
    g.setColour(dark?juce::Colours::white:red);
    g.strokePath(mark,juce::PathStrokeType(2.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
}
void SishhinDSPAudioProcessorEditor::Style::drawButtonBackground(juce::Graphics&g,juce::Button&b,
    const juce::Colour&,bool over,bool down) {
    auto r=b.getLocalBounds().toFloat().reduced(1);
    const bool active=b.getToggleState();
    g.setColour(active?red:(over?juce::Colours::white:juce::Colour(0xfffaf8f4)));
    if(down)g.setColour(red.withAlpha(.65f));
    g.fillRoundedRectangle(r,8);
    g.setColour(active?red:grey.withAlpha(.25f));g.drawRoundedRectangle(r,8,active?2:1);
}
SishhinDSPAudioProcessorEditor::KnobControl::KnobControl(APVTS& tree,const juce::String& id,
    const juce::String& title,juce::String onPage):page(std::move(onPage)) {
    dial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dial.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    dial.setRotaryParameters(juce::degreesToRadians(225.f),juce::degreesToRadians(495.f),true);
    dial.setPopupDisplayEnabled(true,true,this);
    dial.setName(title);
    addAndMakeVisible(dial);
    name.setText(title,juce::dontSendNotification);name.setJustificationType(juce::Justification::centred);
    name.setColour(juce::Label::textColourId,grey);
    name.setFont(juce::Font(juce::FontOptions(10.f,juce::Font::bold)));
    addAndMakeVisible(name);
    attachment=std::make_unique<APVTS::SliderAttachment>(tree,id,dial);
}
void SishhinDSPAudioProcessorEditor::KnobControl::resized() {
    auto r=getLocalBounds(); dial.setBounds(r.removeFromTop(r.getHeight()-16));name.setBounds(r);
}
SishhinDSPAudioProcessorEditor::ToggleControl::ToggleControl(APVTS&tree,const juce::String&id,
    const juce::String&title,juce::String onPage):juce::TextButton(title),page(std::move(onPage)) {
    setClickingTogglesState(true);
    attachment=std::make_unique<APVTS::ButtonAttachment>(tree,id,*this);
}

SishhinDSPAudioProcessorEditor::SishhinDSPAudioProcessorEditor(SishhinDSPAudioProcessor& p)
    :juce::AudioProcessorEditor(&p),processor(p) {
    setLookAndFeel(&style);
    logo=juce::ImageFileFormat::loadFrom(BinaryData::sishhin_logo_png,BinaryData::sishhin_logo_pngSize);
    const auto mk=[this](const juce::String& id,const juce::String& name,const juce::String& page){makeKnob(id,name,page);};
    // Original top-bar controls
    for(auto item:std::initializer_list<std::pair<const char*,const char*>>{{"input","INPUT"},{"gate","GATE"},{"trans","TRANSPOSE"},{"dbl","DOUBLER"},{"out","OUTPUT"}})
        mk(item.first,item.second,"all");
    for(auto item:std::initializer_list<std::pair<const char*,const char*>>{{"gain","GAIN"},{"bass","BASS"},{"mid","MIDDLE"},{"treb","TREBLE"},{"pres","PRESENCE"},{"master","MASTER"},{"ao","OUTPUT"}})
        mk(item.first,item.second,"amp");
    for(auto prefix:{"a","b"})for(auto item:std::initializer_list<std::pair<const char*,const char*>>{{"pos","POSITION"},{"dist","DISTANCE"},{"lv","MIC LEVEL"},{"pan","PAN"}})
        mk(juce::String(prefix)+item.first,item.second,juce::String("mic")+prefix);
    for(auto item:std::initializer_list<std::pair<const char*,const char*>>{{"irlv","IR LEVEL"},{"irlo","LOW CUT"},{"irhi","HIGH CUT"}})
        mk(item.first,item.second,"cab");
    static const char* bands[]={"65 Hz","125 Hz","250 Hz","500 Hz","1 kHz","2 kHz","4 kHz","8 kHz","16 kHz"};
    for(int i=0;i<9;++i)mk("e"+juce::String(i),bands[i],"eq");
    static const char* names[5][3]={{"SUSTAIN","MIX","LEVEL"},{"DRIVE","TONE","LEVEL"},
                                     {"BOOST","LOW","LEVEL"},{"TIME","FEEDBACK","MIX"},{"DECAY","TONE","MIX"}};
    for(int i=0;i<5;++i)for(int j=0;j<3;++j)mk("p"+juce::String(i)+"_"+juce::String(j),names[i][j],i<3?"pre":"post");
    for(auto item:std::initializer_list<std::pair<const char*,const char*>>{{"gateOn","GATE"},{"doubler","DOUBLE"},{"stereo","STEREO"}})
        makeToggle(item.first,item.second,"all");
    for(auto item:std::initializer_list<std::pair<const char*,const char*>>{{"power","POWER"}})makeToggle(item.first,item.second,"amp");
    for(auto item:std::initializer_list<std::pair<const char*,const char*>>{{"ma","MIC A"},{"mb","MIC B"},{"irOn","IR POWER"}})
        makeToggle(item.first,item.second,"cab");
    makeToggle("eqOn","EQ POWER","eq");
    for(int i=0;i<5;++i)makeToggle("p"+juce::String(i)+"On","ON / OFF",i<3?"pre":"post");

    for(int i=0;i<5;++i) {
        addAndMakeVisible(tabButtons[(size_t)i]);
        tabButtons[(size_t)i].onClick=[this,i]{displayTab(i);};
    }
    for(int i=0;i<3;++i) {
        addAndMakeVisible(ampButtons[(size_t)i]);
        ampButtons[(size_t)i].onClick=[this,i]{selectAmp(i);};
        addAndMakeVisible(cabButtons[(size_t)i]);
        cabButtons[(size_t)i].onClick=[this,i]{selectCab(i);};
    }
    addAndMakeVisible(presetCombo);
    presetCombo.onChange=[this]{
        const int id=presetCombo.getSelectedId();
        if(id>0&&id<=72)processor.selectFactoryPreset(id-1);
        else if(id>=1000&&id<1000+userPresets.size())processor.loadUserPreset(userPresets[id-1000]);
    };
    for(auto* b:{&previous,&next,&save,&saveAs,&loadIR,&clearIRButton,&resetEQ})addAndMakeVisible(*b);
    previous.onClick=[this]{processor.selectFactoryPreset((processor.getCurrentProgram()+71)%72);presetCombo.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);};
    next.onClick=[this]{processor.selectFactoryPreset((processor.getCurrentProgram()+1)%72);presetCombo.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);};
    save.onClick=[this]{
        const int id=presetCombo.getSelectedId();
        if(id>=1000&&id<1000+userPresets.size()){processor.saveUserPreset(userPresets[id-1000]);}
        else saveAs.triggerClick();
    };
    saveAs.onClick=[this]{
        auto root=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("SishhinDSP").getChildFile("Presets");
        root.createDirectory();
        chooser=std::make_unique<juce::FileChooser>("Save SishhinDSP preset",root.getChildFile("My preset.sdsp"),"*.sdsp");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles,
            [safe=juce::Component::SafePointer<SishhinDSPAudioProcessorEditor>(this)](const juce::FileChooser& fc){
                if(safe==nullptr)return;
                const auto file=fc.getResult();
                if(file!=juce::File()){
                    safe->processor.saveUserPreset(file.getFileNameWithoutExtension());
                    safe->refreshPresets();
                }
            });
    };
    loadIR.onClick=[this]{
        chooser=std::make_unique<juce::FileChooser>("Load a WAV or AIFF cabinet IR",juce::File(),"*.wav;*.aif;*.aiff");
        chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
            [safe=juce::Component::SafePointer<SishhinDSPAudioProcessorEditor>(this)](const juce::FileChooser&fc){
                if(safe==nullptr)return;
                const auto file=fc.getResult();
                if(file!=juce::File()) {
                    juce::String error;
                    if(!safe->processor.loadIRFile(file,error))safe->irInfo.setText(error,juce::dontSendNotification);
                    else safe->irInfo.setText(file.getFileName(),juce::dontSendNotification);
                }
            });
    };
    clearIRButton.onClick=[this]{processor.clearIR();setParameter("irOn",0);irInfo.setText("NO IR LOADED",juce::dontSendNotification);};
    resetEQ.onClick=[this]{for(int i=0;i<9;++i)setParameter("e"+juce::String(i),0.f);};
    addAndMakeVisible(irInfo);irInfo.setJustificationType(juce::Justification::centred);
    irInfo.setColour(juce::Label::textColourId,grey);
    irInfo.setText("NO IR LOADED",juce::dontSendNotification);
    setSize(1060,790);
    setResizable(true,true);
    setResizeLimits(950,720,1320,1000);
    refreshPresets();displayTab(1);
    startTimerHz(15);
}
SishhinDSPAudioProcessorEditor::~SishhinDSPAudioProcessorEditor(){setLookAndFeel(nullptr);}
void SishhinDSPAudioProcessorEditor::makeKnob(const juce::String&id,const juce::String& name,const juce::String& page) {
    auto k=std::make_unique<KnobControl>(processor.apvts,id,name,page);
    addAndMakeVisible(*k);knobs.push_back(std::move(k));
}
void SishhinDSPAudioProcessorEditor::makeToggle(const juce::String&id,const juce::String&name,const juce::String&page){
    auto t=std::make_unique<ToggleControl>(processor.apvts,id,name,page);
    addAndMakeVisible(*t);toggles.push_back(std::move(t));
}
void SishhinDSPAudioProcessorEditor::setParameter(const juce::String& id,float v) {
    if(auto* p=processor.apvts.getParameter(id)){
        p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(v));p->endChangeGesture();
    }
}
void SishhinDSPAudioProcessorEditor::selectAmp(int amp){setParameter("amp",(float)amp);repaint();}
void SishhinDSPAudioProcessorEditor::selectCab(int cab){setParameter("cab",(float)cab);repaint();}
void SishhinDSPAudioProcessorEditor::refreshPresets(){
    presetCombo.clear(juce::dontSendNotification);
    for(int i=0;i<72;++i)presetCombo.addItem(juce::String(i+1).paddedLeft('0',3)+"  ·  "+processor.getProgramName(i),i+1);
    userPresets=processor.userPresetNames();
    for(int i=0;i<userPresets.size();++i)presetCombo.addItem("USER  ·  "+userPresets[i],1000+i);
    presetCombo.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);
}
void SishhinDSPAudioProcessorEditor::timerCallback(){
    const int program=processor.getCurrentProgram();
    if(program!=lastFactoryProgram){presetCombo.setSelectedId(program+1,juce::dontSendNotification);lastFactoryProgram=program;}
    const int amp=(int)param(processor,"amp"),cab=(int)param(processor,"cab");
    for(int i=0;i<3;++i){ampButtons[(size_t)i].setToggleState(amp==i,juce::dontSendNotification);
                         cabButtons[(size_t)i].setToggleState(cab==i,juce::dontSendNotification);}
    repaint();
}
void SishhinDSPAudioProcessorEditor::displayTab(int tab){
    currentTab=juce::jlimit(0,4,tab);
    for(int i=0;i<5;++i)tabButtons[(size_t)i].setToggleState(i==tab,juce::dontSendNotification);
    resized();repaint();
}
void SishhinDSPAudioProcessorEditor::resized(){
    // Use one reference coordinate system. Uniform scale keeps the original proportions.
    const float scale=std::min(getWidth()/1060.f,getHeight()/790.f);
    const float left=(getWidth()-1060.f*scale)*.5f, top=(getHeight()-790.f*scale)*.5f;
    const auto put=[&](juce::Component& c,float x,float y,float w,float h){
        c.setBounds(juce::Rectangle<float>(left+x*scale,top+y*scale,w*scale,h*scale).toNearestInt());};
    for(int i=0;i<5;++i)put(tabButtons[(size_t)i],310.f+i*91.f,44,86,30);
    // Common bar: INPUT, GATE, TRANSPOSE, DOUBLER, OUTPUT (separated by preset browser).
    const float topPositions[]={80,165,250,795,875};
    int commonKnob=0;
    int ai=0,ci=0,eq=0,ir=0,pre=0,post=0;
    for(auto& kp:knobs){
        auto& k=*kp;
        const bool visible=k.page=="all"||(k.page=="amp"&&currentTab==1)||(k.page=="cab"&&currentTab==2)
            ||(k.page=="mica"&&currentTab==2)||(k.page=="micb"&&currentTab==2)
            ||(k.page=="eq"&&currentTab==3)||(k.page=="pre"&&currentTab==0)||(k.page=="post"&&currentTab==4);
        k.setVisible(visible);
        if(k.page=="all")put(k,topPositions[commonKnob++],100,63,68);
        else if(k.page=="amp")put(k,248.f+ai++*83.f,421,65,77);
        else if(k.page=="mica")put(k,54.f+(ci%2)*91.f,277.f+(ci/2)*106.f,73,85),++ci;
        else if(k.page=="micb")put(k,825.f+((ci-4)%2)*91.f,277.f+((ci-4)/2)*106.f,73,85),++ci;
        else if(k.page=="cab")put(k,385.f+ir++*102.f,666,78,76);
        else if(k.page=="eq")put(k,132.f+eq++*90.f,355,70,108);
        else if(k.page=="pre")put(k,108.f+(pre/3)*295.f+(pre%3)*87.f,340,71,86),++pre;
        else if(k.page=="post")put(k,253.f+(post/3)*314.f+(post%3)*87.f,340,71,86),++post;
    }
    int togglePre=0,togglePost=0;
    for(auto& tp:toggles){
        auto& t=*tp;const bool visible=t.page=="all"||(t.page=="amp"&&currentTab==1)
            ||(t.page=="cab"&&currentTab==2)||(t.page=="eq"&&currentTab==3)
            ||(t.page=="pre"&&currentTab==0)||(t.page=="post"&&currentTab==4);
        t.setVisible(visible);
        const auto id=t.getButtonText();
        if(t.page=="all"){
            if(id=="GATE")put(t,165,168,61,23);
            else if(id=="DOUBLE")put(t,790,168,75,23);
            else put(t,940,111,82,26);
        }else if(t.page=="amp")put(t,875,432,95,31);
        else if(t.page=="cab"){
            if(id=="MIC A")put(t,53,244,88,25);
            else if(id=="MIC B")put(t,818,244,88,25);
            else put(t,245,676,112,30);
        }else if(t.page=="eq")put(t,800,264,118,31);
        else if(t.page=="pre")put(t,171+togglePre++*295,495,105,34);
        else if(t.page=="post")put(t,319+togglePost++*314,495,105,34);
    }
    put(presetCombo,356,108,390,35);
    put(previous,356,150,30,25);put(next,392,150,30,25);
    put(save,430,150,75,25);put(saveAs,511,150,108,25);
    for(int i=0;i<3;++i) {
        ampButtons[(size_t)i].setVisible(currentTab==1);
        cabButtons[(size_t)i].setVisible(currentTab==2);
        put(ampButtons[(size_t)i],262+i*182.f,568,176,58);
        put(cabButtons[(size_t)i],335+i*132.f,595,125,49);
    }
    for(auto* comp:{&loadIR,&clearIRButton,&irInfo})comp->setVisible(currentTab==2);
    put(loadIR,490,614,102,30);put(clearIRButton,601,614,80,30);put(irInfo,287,650,445,24);
    resetEQ.setVisible(currentTab==3);put(resetEQ,685,264,105,31);
}

void SishhinDSPAudioProcessorEditor::drawFrame(juce::Graphics&g){
    g.fillAll(juce::Colour(0xffebe8e2));
    auto frame=juce::Rectangle<float>(8,8,1044,774);
    roundedPanel(g,frame,juce::Colour(0xfff6f3ed),13);
    g.setColour(juce::Colour(0xffece7df));
    g.fillRoundedRectangle(frame.withHeight(40),12);
    g.setColour(juce::Colour(0xffff5f57));g.fillEllipse(22,23,12,12);
    g.setColour(juce::Colour(0xfffebc2e));g.fillEllipse(40,23,12,12);
    g.setColour(juce::Colour(0xff28c840));g.fillEllipse(58,23,12,12);
    caption(g,"SishhinDSP",juce::Rectangle<float>(450,17,160,26),13,ink);
    g.setColour(juce::Colour(0xffd8d5cf));g.drawHorizontalLine(84,20,1040);
    g.drawHorizontalLine(192,20,1040);
    g.setColour(grey);g.setFont(juce::Font(juce::FontOptions(10,juce::Font::bold)));
    g.drawText("SISHHINDSP    ·    NATIVE JUCE VST3",35,754,460,26,juce::Justification::centredLeft);
    g.setColour(red);g.drawText("失神 · DSP",886,754,130,26,juce::Justification::centredRight);
}
void SishhinDSPAudioProcessorEditor::drawAmp(juce::Graphics&g,juce::Rectangle<float> face,int model,bool small){
    const auto base=model==0?juce::Colour(0xffe9e2d6):model==1?deep:green;
    roundedPanel(g,face,base,15);
    auto inner=face.reduced(11,11);
    const auto bg=model==0?juce::Colour(0xfff7f3eb):model==1?juce::Colour(0xff11172c):juce::Colour(0xff1a311e);
    g.setColour(bg);g.fillRoundedRectangle(inner,10);
    g.saveState();
    g.reduceClipRegion(inner.toNearestInt());
    if(model==0){
        g.setColour(juce::Colour(0xfff1d3cf));g.fillEllipse(inner.getRight()-180,inner.getY()+30,138,138);
        g.setColour(juce::Colour(0xff8a6c5d));
        for(int i=0;i<4;++i){g.drawLine(inner.getRight()-i*70,inner.getY()+5,inner.getRight()-180+i*15,inner.getBottom()-30,2.7f);}
        for(int i=0;i<27;++i){
            const float x=inner.getRight()-20.f-(i*37%310),y=inner.getY()+17.f+(i*47%130);
            g.setColour(juce::Colour(0xffe7a4b5).withAlpha(.7f));g.fillEllipse(x,y,9,6);
        }
    } else if(model==1) {
        g.setColour(juce::Colour(0xffeef1fe));g.fillEllipse(inner.getRight()-125,inner.getY()+17,39,39);
        g.setColour(juce::Colour(0xff1a2749));
        juce::Path mountain;mountain.startNewSubPath(inner.getX()+240,inner.getBottom());
        mountain.lineTo(inner.getX()+315,inner.getY()+70);mountain.lineTo(inner.getX()+385,inner.getBottom());mountain.closeSubPath();g.fillPath(mountain);
        for(int i=0;i<7;++i){
            const float x=inner.getX()+inner.getWidth()*.66f+i*12;
            g.setColour(juce::Colour(0xffc4e2ff).withAlpha(.3f+i*.08f));g.drawLine(x,inner.getY(),x,inner.getBottom(),2);
        }
    } else {
        g.setColour(juce::Colour(0xff789b67).withAlpha(.35f));
        for(int i=0;i<8;++i){
            const float x=inner.getX()+inner.getWidth()*.42f+i*38;
            g.fillRoundedRectangle(x,inner.getY(),9,inner.getHeight(),3);
            g.setColour(juce::Colour(0xffa8ca88).withAlpha(.8f));
            g.drawLine(x,inner.getY()+54,x+9,inner.getY()+54,2);
        }
    }
    g.restoreState();
    auto logoBox=juce::Rectangle<float>(inner.getX()+35,inner.getY()+24,small?110.f:165.f,small?50.f:75.f);
    // Re-color the existing embedded Japanese logo using its alpha mask.
    if(logo.isValid()) {
        g.saveState();
        const auto target=logoBox.toNearestInt();
        juce::Image tinted(juce::Image::ARGB,logo.getWidth(),logo.getHeight(),true);
        juce::Image::BitmapData dst(tinted,juce::Image::BitmapData::writeOnly);
        juce::Image::BitmapData src(logo,juce::Image::BitmapData::readOnly);
        const auto tint=model==0?ink:juce::Colour(0xfff4f7ff);
        for(int y=0;y<logo.getHeight();++y)for(int x=0;x<logo.getWidth();++x){
            const auto px=src.getPixelColour(x,y);
            dst.setPixelColour(x,y,tint.withAlpha(px.getAlpha()/255.f));
        }
        g.drawImageWithin(tinted,target.getX(),target.getY(),target.getWidth(),target.getHeight(),juce::RectanglePlacement::centred);
        g.restoreState();
    } else {
        caption(g,"失神",logoBox,small?28:40,model==0?ink:juce::Colours::white);
    }
    caption(g,"S I S S H I N   ·   D S P",logoBox.translated(0,small?42:61).withHeight(18),small?7:9,
            model==0?ink:juce::Colours::white);
    // Handle / jack / top shell controls
    g.setColour(model==0?red:model==1?juce::Colour(0xffa1b9ee):juce::Colour(0xff90b476));
    g.drawLine(face.getX()+42,face.getY()-5,face.getRight()-42,face.getY()-5,6);
}
void SishhinDSPAudioProcessorEditor::drawCabinet(juce::Graphics&g,juce::Rectangle<float>cab,int model) {
    roundedPanel(g,cab,model==0?juce::Colour(0xffe8e0d3):model==1?deep:green,14);
    const auto inner=cab.reduced(15);
    g.setColour(juce::Colour(0xff080b12).withAlpha(.17f));g.fillRoundedRectangle(inner,10);
    const int n=model==0?1:model==1?4:2;
    for(int i=0;i<n;++i){
        const float size=n==1?185.f:100.f;
        const float x=n==1?inner.getCentreX() :inner.getCentreX()+(i%2==0?-1:1)*70.f;
        const float y=n==1?inner.getCentreY() :inner.getCentreY()+(i/2==0?-1:1)*62.f;
        g.setColour(model==0?juce::Colour(0xffd7d0c3):juce::Colour(0xff292d36));
        g.fillEllipse(x-size*.5f,y-size*.5f,size,size);
        g.setColour(model==0?juce::Colour(0xffede9dc):juce::Colour(0xff111318));
        g.fillEllipse(x-size*.4f,y-size*.4f,size*.8f,size*.8f);
        g.setColour(model==0?juce::Colour(0xffc7c1b4):juce::Colour(0xff353943));
        g.fillEllipse(x-size*.13f,y-size*.13f,size*.26f,size*.26f);
    }
    g.setColour(red);g.fillRoundedRectangle(cab.getCentreX()+25,cab.getCentreY()+17,80,13,4);
}
void SishhinDSPAudioProcessorEditor::drawPedals(juce::Graphics&g,bool post){
    static const char* pedals[]={"BLOOM","EDGE","WEIGHT","ECHO","HALO"};
    static const char* subtitles[]={"OPTICAL COMP","DYNAMIC DRIVE","LOW BOOST","TAPE DELAY","REVERB"};
    static const juce::Colour colours[]={juce::Colour(0xfff5d6df),deep,juce::Colour(0xffd2c6a7),
                                         juce::Colour(0xff559061),juce::Colour(0xff6194bd)};
    const int count=post?2:3;
    for(int i=0;i<count;++i){
        const int idx=i+(post?3:0);
        float x=(post?200.f:53.f)+i*(post?314.f:295.f);
        const auto r=juce::Rectangle<float>(x,290,260,280);
        roundedPanel(g,r,colours[idx],14);
        caption(g,pedals[idx],r.withY(435).withHeight(34),19,idx==0||idx==2?ink:juce::Colours::white);
        caption(g,subtitles[idx],r.withY(465).withHeight(24),10,idx==0||idx==2?ink:juce::Colours::white);
        g.setColour(juce::Colours::black.withAlpha(.25f));g.fillEllipse(r.getCentreX()-21,530,42,22);
    }
}
void SishhinDSPAudioProcessorEditor::drawEQ(juce::Graphics&g){
    auto r=juce::Rectangle<float>(86,244,890,350);
    roundedPanel(g,r,juce::Colour(0xffdee1ea),14);
    caption(g,"9-BAND GRAPHIC EQ",r.withHeight(42),16,ink);
    g.setColour(grey.withAlpha(.2f));
    g.drawHorizontalLine(482,r.getX()+30,r.getRight()-30);
}
void SishhinDSPAudioProcessorEditor::paint(juce::Graphics&g){
    const float scale=std::min(getWidth()/1060.f,getHeight()/790.f);
    const float left=(getWidth()-1060.f*scale)*.5f,top=(getHeight()-790.f*scale)*.5f;
    juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::scale(scale).translated(left,top));
    drawFrame(g);
    const int amp=(int)param(processor,"amp");
    if(currentTab==1){
        drawAmp(g,{130,234,800,275},amp);
        g.setColour(juce::Colours::black.withAlpha(.08f));g.fillRoundedRectangle(158,409,744,98,6);
        caption(g,"SAKURA CLEAN     ·     TAKI METAL     ·     TAKE THICK",{245,639,590,25},10);
    } else if(currentTab==2) {
        drawAmp(g,{362,240,334,107},amp,true);
        drawCabinet(g,{383,349,295,236},(int)param(processor,"cab"));
        roundedPanel(g,{220,653,600,102},cream,12);
        caption(g,"IR LOADER",{230,660,160,32},13,ink);
        caption(g,"MIC A",{43,216,190,30},14,ink);
        caption(g,"MIC B",{816,216,190,30},14,ink);
    } else if(currentTab==3) drawEQ(g);
    else drawPedals(g,currentTab==4);
}
