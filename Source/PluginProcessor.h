#pragma once
#include <JuceHeader.h>
#include "AmpEngine.h"
#include <atomic>
#include <array>
#include <vector>

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
    bool loadFactoryIR(int index);
    int selectedFactoryIR() const noexcept { return currentFactoryIR.load(); }
    bool loadIRData(const juce::MemoryBlock& data, const juce::String& filename, juce::String& error);
    void clearIR();
    juce::String currentIRName() const;
    bool saveUserPreset(const juce::String& name);
    bool deleteUserPreset(const juce::String& name);
    bool loadUserPreset(const juce::String& name);
    juce::StringArray userPresetNames() const;
    std::atomic<int> requestedProgram {-1};
    std::atomic<float> lastInputPeak {0.0f}, lastOutputPeak {0.0f};

    struct MidiMapping {
        juce::String target;
        int type=0;     // 0=CC Absolute, 1=Program Change, 2=Note
        int channel=0;  // 0=omni; 1..16 specific channel
        int number=10;  // CC, program-change or note number
        int preset=0;   // Program selected by Program Change mapping
    };
    void startMidiLearn(const juce::String& target);
    void cancelMidiLearn();
    juce::String currentMidiLearnTarget() const;
    juce::var getMidiMappings() const;
    void setMidiMappings(const juce::var& mappings);
    float detectTunerPitchHz() const;

private:
    sdsp::AmpEngine engine;
    juce::AudioBuffer<float> floatScratch;
    std::array<float, sdsp::maxIR> pendingImpulse{};
    int pendingLength = 0;
    std::atomic<bool> impulseUpdate {false};
    mutable juce::CriticalSection irLock;
    juce::String irName;
    std::atomic<int> currentProgram {0};
    std::atomic<int> currentFactoryIR {-1};
    // Lock-free tuner capture: the audio thread only publishes decimated DI.
    static constexpr unsigned tunerRingSize=2048;
    std::array<std::atomic<float>,tunerRingSize> tunerRing;
    std::atomic<unsigned> tunerWrite {0};
    int tunerCounter=0, tunerStride=4;
    std::atomic<float> tunerCaptureRate {12000.f};
    // MIDI is decoded on the real-time thread and applied on JUCE message
    // thread via AsyncUpdater; no APVTS edit or allocation inside processBlock.
    std::atomic<int> pendingMidiEvent {-1};
    mutable juce::CriticalSection midiMapLock;
    std::vector<MidiMapping> midiMappings;
    juce::String midiLearnTarget;
    void handleIncomingMidiEvent(int encoded);
    static juce::var midiMappingToVar(const MidiMapping&);
    sdsp::Parameters collectParameters() const noexcept;
    float get(const char* id) const noexcept;
    void pushIRToEngine() noexcept;
    static juce::File presetFolder();
    void handleAsyncUpdate() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SishhinDSPAudioProcessor)
};
