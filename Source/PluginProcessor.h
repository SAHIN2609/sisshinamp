#pragma once
#include <JuceHeader.h>
#include "AmpEngine.h"
#include <atomic>

class SishhinDSPAudioProcessor final : public juce::AudioProcessor, private juce::AsyncUpdater {
public:
    SishhinDSPAudioProcessor();
    ~SishhinDSPAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock(juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SishhinDSP"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 72; }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;
    void selectFactoryPreset(int index);
    bool loadIRFile(const juce::File& file, juce::String& error);
    void clearIR();
    juce::String currentIRName() const;
    bool saveUserPreset(const juce::String& name);
    bool deleteUserPreset(const juce::String& name);
    bool loadUserPreset(const juce::String& name);
    juce::StringArray userPresetNames() const;
    std::atomic<int> requestedProgram {-1};

private:
    sdsp::AmpEngine engine;
    juce::AudioBuffer<float> floatScratch;
    std::array<float, sdsp::maxIR> pendingImpulse{};
    int pendingLength = 0;
    bool impulseUpdate = false;
    mutable juce::CriticalSection irLock;
    juce::String irName;
    std::atomic<int> currentProgram {0};
    sdsp::Parameters collectParameters() const noexcept;
    float get(const char* id) const noexcept;
    void pushIRToEngine() noexcept;
    static juce::File presetFolder();
    void handleAsyncUpdate() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SishhinDSPAudioProcessor)
};
