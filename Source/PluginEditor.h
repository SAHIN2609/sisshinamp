#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <optional>

// The original HTML is embedded as BinaryData and presented by JUCE 8's WebView.
// Audio is always processed by PluginProcessor/AmpEngine, never by JavaScript.
class SishhinDSPAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    explicit SishhinDSPAudioProcessorEditor(SishhinDSPAudioProcessor&);
    ~SishhinDSPAudioProcessorEditor() override = default;

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    class Page final : public juce::WebBrowserComponent
    {
    public:
        explicit Page(const juce::WebBrowserComponent::Options& options)
            : juce::WebBrowserComponent(options) {}

        bool pageAboutToLoad(const juce::String& url) override
        {
            // Do not expose the native event bridge to external websites.
            return url.startsWith(juce::WebBrowserComponent::getResourceProviderRoot());
        }
    };

    static std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String&);
    juce::WebBrowserComponent::Options makeOptions();
    void handleParameter(juce::var);
    void handleBatch(juce::var);
    void handleProgram(juce::var);
    void handleIR(juce::var);
    void setParameter(const juce::String&, float);
    void sendError(const juce::String&);
    void sendSnapshot();
    void timerCallback() override;

    SishhinDSPAudioProcessor& processor;
    Page browser;
    bool browserReady = false;
    bool tunerActive = false;
    juce::String lastSnapshot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SishhinDSPAudioProcessorEditor)
};
