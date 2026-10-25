#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <vector>

namespace
{
    std::optional<juce::WebBrowserComponent::Resource> makeResource(const char* data,
                                                                    int length,
                                                                    const juce::String& mime)
    {
        if (data == nullptr || length <= 0)
            return std::nullopt;

        std::vector<std::byte> bytes(static_cast<size_t>(length));
        std::memcpy(bytes.data(), data, static_cast<size_t>(length));
        return juce::WebBrowserComponent::Resource { std::move(bytes), mime };
    }
}

std::optional<juce::WebBrowserComponent::Resource>
SishhinDSPAudioProcessorEditor::getResource(const juce::String& path)
{
    // The original HTML and its JS bridge are compiled into the plugin bundle.
    // There is no localhost server, no filesystem dependency, and no network upload.
    if (path == "/" || path == "/index.html" || path == "/interface.html")
        return makeResource(BinaryData::interface_html,
                            BinaryData::interface_htmlSize, "text/html; charset=utf-8");

    if (path == "/bridge.js")
        return makeResource(BinaryData::bridge_js,
                            BinaryData::bridge_jsSize, "text/javascript; charset=utf-8");

    return std::nullopt;
}

juce::WebBrowserComponent::Options SishhinDSPAudioProcessorEditor::makeOptions()
{
    auto options = juce::WebBrowserComponent::Options{}.withNativeIntegrationEnabled();

#if JUCE_WINDOWS
    // The Edge WebView2 user-data folder must be writable inside third-party DAWs.
    const auto dataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("SishhinDSP").getChildFile("WebView2");
    dataDir.createDirectory();
    options = options
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
                                   .withUserDataFolder(dataDir));
#endif

    return options
        .withEventListener("sdspReady", [this](juce::var)
        {
            browserReady = true;
            lastSnapshot.clear();
            // The timer sends initial data after the document has finished loading.
        })
        .withEventListener("sdspParameter", [this](juce::var v) { handleParameter(std::move(v)); })
        .withEventListener("sdspBatch", [this](juce::var v) { handleBatch(std::move(v)); })
        .withEventListener("sdspProgram", [this](juce::var v) { handleProgram(std::move(v)); })
        .withEventListener("sdspSavePreset", [this](juce::var v)
        {
            if (const auto* object = v.getDynamicObject())
            {
                const auto name = object->getProperty("name").toString().trim();
                if (name.isNotEmpty() && !processor.saveUserPreset(name))
                    sendError("Unable to save preset " + name);
            }
        })
        .withEventListener("sdspDeletePreset", [this](juce::var v)
        {
            if (const auto* object = v.getDynamicObject())
            {
                const auto name = object->getProperty("name").toString().trim();
                if (name.isNotEmpty() && !processor.deleteUserPreset(name))
                    sendError("Unable to delete preset " + name);
            }
        })
        .withEventListener("sdspIR", [this](juce::var v) { handleIR(std::move(v)); })
        .withEventListener("sdspClearIR", [this](juce::var)
        {
            processor.clearIR();
            setParameter("irOn", 0.0f);
            lastSnapshot.clear();
        })
        .withResourceProvider([](const juce::String& path) { return getResource(path); });
}

SishhinDSPAudioProcessorEditor::SishhinDSPAudioProcessorEditor(SishhinDSPAudioProcessor& p)
    : juce::AudioProcessorEditor(&p),
      processor(p),
      browser(makeOptions())
{
    addAndMakeVisible(browser);
    setSize(1100, 925);
    setResizable(true, true);
    setResizeLimits(880, 675, 1650, 1380);
    browser.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    startTimerHz(8);
}

void SishhinDSPAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xffefece6));
}

void SishhinDSPAudioProcessorEditor::resized()
{
    browser.setBounds(getLocalBounds());
}

void SishhinDSPAudioProcessorEditor::setParameter(const juce::String& id, float value)
{
    if (!std::isfinite(value))
        return;

    if (auto* parameter = processor.apvts.getParameter(id))
    {
        const auto normalized = juce::jlimit(0.0f, 1.0f, parameter->convertTo0to1(value));
        if (std::abs(parameter->getValue() - normalized) < 0.000001f)
            return;

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(normalized);
        parameter->endChangeGesture();
    }
}

void SishhinDSPAudioProcessorEditor::handleParameter(juce::var request)
{
    if (const auto* object = request.getDynamicObject())
        setParameter(object->getProperty("id").toString(),
                     static_cast<float>(object->getProperty("value")));
}

void SishhinDSPAudioProcessorEditor::handleBatch(juce::var request)
{
    if (const auto* object = request.getDynamicObject())
    {
        const auto data = object->getProperty("values");
        if (const auto* values = data.getDynamicObject())
        {
            const auto& properties = values->getProperties();
            for (int i = 0; i < properties.size(); ++i)
                setParameter(properties.getName(i).toString(),
                             static_cast<float>(properties.getValueAt(i)));
        }
    }
}

void SishhinDSPAudioProcessorEditor::handleProgram(juce::var request)
{
    if (const auto* object = request.getDynamicObject())
    {
        const int index = static_cast<int>(object->getProperty("index"));
        if (index >= 0 && index < processor.getNumPrograms())
        {
            processor.selectFactoryPreset(index);
            lastSnapshot.clear();
        }
    }
}

void SishhinDSPAudioProcessorEditor::sendError(const juce::String& message)
{
    if (!browserReady)
        return;

    juce::var payload(new juce::DynamicObject());
    payload.getDynamicObject()->setProperty("message", message);
    browser.emitEventIfBrowserIsVisible("sdspError", payload);
}

void SishhinDSPAudioProcessorEditor::handleIR(juce::var request)
{
    const auto* object = request.getDynamicObject();
    if (object == nullptr)
        return;

    const auto filename = object->getProperty("filename").toString();
    const auto encoded = object->getProperty("base64").toString();
    if (encoded.length() > 12 * 1024 * 1024 || encoded.isEmpty())
    {
        sendError("IR is empty or exceeds the 8 MB limit");
        return;
    }

    const auto extension = filename.fromLastOccurrenceOf(".", true, false).toLowerCase();
    if (extension != ".wav" && extension != ".aif" && extension != ".aiff")
    {
        sendError("Load a WAV or AIFF impulse response");
        return;
    }

    juce::MemoryBlock bytes;
    if (!bytes.fromBase64Encoding(encoded) || bytes.getSize() > 8 * 1024 * 1024)
    {
        sendError("Unable to decode IR data");
        return;
    }

    juce::String error;
    if (!processor.loadIRData(bytes, filename, error))
    {
        sendError(error);
        return;
    }

    lastSnapshot.clear();
}

void SishhinDSPAudioProcessorEditor::sendSnapshot()
{
    if (!browserReady)
        return;

    juce::var root(new juce::DynamicObject());
    juce::var values(new juce::DynamicObject());
    auto* props = values.getDynamicObject();

    constexpr const char* names[] = {
        "amp","cab","micaType","micbType",
        "input","gate","trans","dbl","out","gain","bass","mid","treb","pres",
        "master","ao","irlv","irlo","irhi",
        "apos","adist","alv","apan","arm",
        "bpos","bdist","blv","bpan","brm",
        "power","gateOn","eqOn","irOn","ma","mb","raOn","rbOn","doubler","stereo",
        "p0On","p1On","p2On","p3On","p4On"
    };
    for (auto* name : names)
        if (const auto* value = processor.apvts.getRawParameterValue(name))
            props->setProperty(juce::Identifier(name), value->load());

    for (int i = 0; i < 9; ++i)
    {
        const auto id = "e" + juce::String(i);
        if (const auto* value = processor.apvts.getRawParameterValue(id))
            props->setProperty(juce::Identifier(id), value->load());
    }
    for (int effect = 0; effect < 5; ++effect)
        for (int knob = 0; knob < 3; ++knob)
        {
            const auto id = "p" + juce::String(effect) + "_" + juce::String(knob);
            if (const auto* value = processor.apvts.getRawParameterValue(id))
                props->setProperty(juce::Identifier(id), value->load());
        }

    auto* obj = root.getDynamicObject();
    obj->setProperty("values", values);
    obj->setProperty("program", processor.getCurrentProgram());
    obj->setProperty("irName", processor.currentIRName());
    obj->setProperty("inputPeak", processor.lastInputPeak.load());
    obj->setProperty("outputPeak", processor.lastOutputPeak.load());

    const auto serialized = juce::JSON::toString(root, true);
    if (serialized == lastSnapshot)
        return;

    lastSnapshot = serialized;
    browser.emitEventIfBrowserIsVisible("sdspState", root);
}

void SishhinDSPAudioProcessorEditor::timerCallback()
{
    sendSnapshot();
}
