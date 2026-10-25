#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <memory>
#include <vector>

class SishhinDSPAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer {
public:
    explicit SishhinDSPAudioProcessorEditor(SishhinDSPAudioProcessor&);
    ~SishhinDSPAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    using APVTS=juce::AudioProcessorValueTreeState;
    struct Style final : juce::LookAndFeel_V4 {
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
        void drawButtonBackground(juce::Graphics&, juce::Button&,const juce::Colour&,bool,bool) override;
    } style;
    struct KnobControl final : juce::Component {
        juce::Slider dial;
        juce::Label name;
        std::unique_ptr<APVTS::SliderAttachment> attachment;
        juce::String page;
        KnobControl(APVTS&,const juce::String& id,const juce::String& title,juce::String onPage);
        void resized() override;
    };
    struct ToggleControl final : juce::TextButton {
        std::unique_ptr<APVTS::ButtonAttachment> attachment;
        juce::String page;
        ToggleControl(APVTS&,const juce::String& id,const juce::String& title,juce::String onPage);
    };
    SishhinDSPAudioProcessor& processor;
    int currentTab=1;
    juce::Image logo, logoDark, logoLight;
    std::vector<std::unique_ptr<KnobControl>> knobs;
    std::vector<std::unique_ptr<ToggleControl>> toggles;
    std::array<juce::TextButton,5> tabButtons{{juce::TextButton{"PRE FX"},juce::TextButton{"AMP"},juce::TextButton{"CAB"},juce::TextButton{"EQ"},juce::TextButton{"POST FX"}}};
    std::array<juce::TextButton,3> ampButtons{{juce::TextButton{"Sakura  CLEAN"},juce::TextButton{"Taki  METAL"},juce::TextButton{"Take  THICK"}}};
    std::array<juce::TextButton,3> cabButtons{{juce::TextButton{"1 x 12  OPEN"},juce::TextButton{"4 x 12  MODERN"},juce::TextButton{"2 x 12  OVERSIZED"}}};
    juce::ComboBox presetCombo;
    juce::TextButton previous{"←"},next{"→"},save{"SAVE"},saveAs{"SAVE AS..."},deletePreset{"DELETE"};
    juce::TextButton loadIR{"LOAD IR"},clearIRButton{"CLEAR"},resetEQ{"RESET"},loadLogo{"LOAD LOGO"};
    std::array<juce::ComboBox,2> micSelectors;
    std::array<std::unique_ptr<APVTS::ComboBoxAttachment>,2> micAttachments;
    juce::Label irInfo, statusInfo;
    int lastPaintAmp=-1, lastPaintCab=-1;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::StringArray userPresets;
    int lastFactoryProgram=0;

    void timerCallback() override;
    void makeKnob(const juce::String& id,const juce::String& text,const juce::String& page);
    void makeToggle(const juce::String& id,const juce::String& text,const juce::String& page);
    void displayTab(int tab);
    void refreshPresets();
    void setParameter(const juce::String& id,float value);
    void selectAmp(int amp);
    void selectCab(int cab);
    void drawAmp(juce::Graphics&,juce::Rectangle<float> face, int model, bool small=false);
    void drawCabinet(juce::Graphics&,juce::Rectangle<float>,int cab);
    void drawPedals(juce::Graphics&,bool post);
    void drawEQ(juce::Graphics&);
    void drawFrame(juce::Graphics&);
    void selectUserPreset(const juce::String& name);
    void updateIRLabel();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SishhinDSPAudioProcessorEditor)
};
