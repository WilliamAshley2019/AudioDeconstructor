// ============================================================================
// PluginProcessor.cpp - Implementation
// ============================================================================

#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioDeconstructorProcessor::AudioDeconstructorProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withInput("Input", juce::AudioChannelSet::mono(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , params(*this, nullptr, "PARAMS", {
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"windowSize", 1},
            "Window Size (ms)",
            juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f),
            15.0f
        ),
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"hopSize", 1},
            "Hop Size (%)",
            juce::NormalisableRange<float>(10.0f, 90.0f, 1.0f),
            50.0f
        ),
        std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"normalize", 1},
            "Normalize Output",
            true
        ),
        std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"smooth", 1},
            "Smooth Output",
            false
        ),
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"smoothTime", 1},
            "Smooth Time (ms)",
            juce::NormalisableRange<float>(1.0f, 50.0f, 1.0f),
            10.0f
        )
        })
{
    initializeExtractors();
}

AudioDeconstructorProcessor::~AudioDeconstructorProcessor() {}

void AudioDeconstructorProcessor::initializeExtractors() {
    extractors["Amplitude"] = FeatureExtractorFactory::createExtractor("Amplitude");
    extractors["Panning"] = FeatureExtractorFactory::createExtractor("Panning");
    extractors["Spectral"] = FeatureExtractorFactory::createExtractor("Spectral");
    extractors["Pitch"] = FeatureExtractorFactory::createExtractor("Pitch");
    extractors["Transients"] = FeatureExtractorFactory::createExtractor("Transients");
}

bool AudioDeconstructorProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo();
}

void AudioDeconstructorProcessor::prepareToPlay(double, int) {}
void AudioDeconstructorProcessor::releaseResources() {}

void AudioDeconstructorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    // Pass-through - this is an analysis plugin
}

bool AudioDeconstructorProcessor::loadAudioFile(const juce::File& file) {
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader != nullptr) {
        loadedSampleRate = reader->sampleRate;
        int numChannels = static_cast<int>(reader->numChannels);
        int numSamples = static_cast<int>(reader->lengthInSamples);

        loadedAudio.setSize(numChannels, numSamples);
        reader->read(&loadedAudio, 0, numSamples, 0, true, true);
        loadedFileName = file.getFileNameWithoutExtension();

        featureBreakpoints.clear();
        return true;
    }
    return false;
}

void AudioDeconstructorProcessor::clearLoadedAudio() {
    loadedAudio.setSize(0, 0);
    loadedFileName = "";
    featureBreakpoints.clear();
}

void AudioDeconstructorProcessor::extractFeature(const juce::String& featureName, int channel) {
    auto it = extractors.find(featureName);
    if (it == extractors.end() || !hasLoadedAudio()) return;

    isAnalyzing = true;
    auto& extractor = it->second;

    // Update settings from parameters
    extractor->settings.windowSizeMs = params.getRawParameterValue("windowSize")->load();
    extractor->settings.hopSizePct = params.getRawParameterValue("hopSize")->load();
    extractor->settings.normalizeOutput = params.getRawParameterValue("normalize")->load() > 0.5f;
    extractor->settings.smoothOutput = params.getRawParameterValue("smooth")->load() > 0.5f;
    extractor->settings.smoothTimeMs = params.getRawParameterValue("smoothTime")->load();

    int channelToUse = juce::jlimit(0, loadedAudio.getNumChannels() - 1,
        channel < 0 ? 0 : channel);

    auto results = extractor->extract(loadedAudio, loadedSampleRate, channelToUse);
    featureBreakpoints[featureName] = results;

    isAnalyzing = false;
}

void AudioDeconstructorProcessor::extractAllFeatures() {
    for (const auto& [featureName, extractor] : extractors) {
        extractFeature(featureName, 0);
    }
}

bool AudioDeconstructorProcessor::isFeatureExtracted(const juce::String& featureName) const {
    return featureBreakpoints.find(featureName) != featureBreakpoints.end();
}

juce::StringArray AudioDeconstructorProcessor::getExtractedFeatures() const {
    juce::StringArray features;
    for (const auto& [name, _] : featureBreakpoints) {
        features.add(name);
    }
    return features;
}

juce::StringArray AudioDeconstructorProcessor::getAvailableFeatures() const {
    juce::StringArray features;
    for (const auto& [name, _] : extractors) {
        features.add(name);
    }
    return features;
}

juce::Colour AudioDeconstructorProcessor::getFeatureColour(const juce::String& featureName) const {
    auto it = extractors.find(featureName);
    return it != extractors.end() ? it->second->getColor() : juce::Colours::white;
}

int AudioDeconstructorProcessor::getNumOutputsForFeature(const juce::String& featureName) const {
    auto it = extractors.find(featureName);
    return it != extractors.end() ? it->second->getNumOutputs() : 0;
}

juce::String AudioDeconstructorProcessor::getOutputName(const juce::String& featureName,
    int outputIndex) const {
    auto it = extractors.find(featureName);
    return it != extractors.end() ? it->second->getOutputName(outputIndex) : "";
}

std::vector<std::pair<double, double>> AudioDeconstructorProcessor::getBreakpointsForDisplay(
    const juce::String& featureName, int outputIndex) const {

    auto it = featureBreakpoints.find(featureName);
    if (it != featureBreakpoints.end() && outputIndex < it->second.size()) {
        return it->second[outputIndex];
    }
    return {};
}

void AudioDeconstructorProcessor::addBreakpoint(const juce::String& featureName,
    int outputIndex, double time, double value) {

    auto it = featureBreakpoints.find(featureName);
    if (it != featureBreakpoints.end() && outputIndex < it->second.size()) {
        it->second[outputIndex].emplace_back(time, value);
        sortBreakpoints(featureName, outputIndex);
    }
}

void AudioDeconstructorProcessor::updateBreakpoint(const juce::String& featureName,
    int outputIndex, size_t pointIndex, double time, double value) {

    auto it = featureBreakpoints.find(featureName);
    if (it != featureBreakpoints.end() && outputIndex < it->second.size()) {
        auto& points = it->second[outputIndex];
        if (pointIndex < points.size()) {
            points[pointIndex] = { juce::jmax(0.0, time), value };
            sortBreakpoints(featureName, outputIndex);
        }
    }
}

void AudioDeconstructorProcessor::removeBreakpoint(const juce::String& featureName,
    int outputIndex, size_t pointIndex) {

    auto it = featureBreakpoints.find(featureName);
    if (it != featureBreakpoints.end() && outputIndex < it->second.size()) {
        auto& points = it->second[outputIndex];
        if (pointIndex < points.size()) {
            points.erase(points.begin() + pointIndex);
        }
    }
}

void AudioDeconstructorProcessor::sortBreakpoints(const juce::String& featureName,
    int outputIndex) {
    auto it = featureBreakpoints.find(featureName);
    if (it != featureBreakpoints.end() && outputIndex < it->second.size()) {
        std::sort(it->second[outputIndex].begin(), it->second[outputIndex].end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
    }
}

void AudioDeconstructorProcessor::saveBreakpoints(const juce::String& featureName,
    const juce::File& file) {

    auto it = featureBreakpoints.find(featureName);
    if (it == featureBreakpoints.end()) return;

    juce::FileOutputStream stream(file);
    if (stream.openedOk()) {
        stream.writeText("# Audio Deconstructor Breakpoint File\n", false, false, "\n");
        stream.writeText("# Feature: " + featureName + "\n", false, false, "\n");
        stream.writeText("# Source: " + loadedFileName + "\n", false, false, "\n");
        stream.writeText("# Sample Rate: " + juce::String(loadedSampleRate) + " Hz\n",
            false, false, "\n");
        stream.writeText("# Generated: " + juce::Time::getCurrentTime().toString(true, true) +
            "\n", false, false, "\n");
        stream.writeText("# Format: time(seconds) value\n\n", false, false, "\n");

        const auto& outputs = it->second;
        for (size_t i = 0; i < outputs.size(); ++i) {
            auto extractorIt = extractors.find(featureName);
            juce::String outputName = extractorIt != extractors.end() ?
                extractorIt->second->getOutputName(static_cast<int>(i)) :
                "Output " + juce::String(i + 1);

            stream.writeText("# " + outputName + "\n", false, false, "\n");

            for (const auto& [time, value] : outputs[i]) {
                stream.writeText(juce::String(time, 6) + "\t" +
                    juce::String(value, 6) + "\n", false, false, "\n");
            }
            stream.writeText("\n", false, false, "\n");
        }
    }
}

void AudioDeconstructorProcessor::saveAllBreakpoints(const juce::File& directory) {
    for (const auto& [featureName, _] : featureBreakpoints) {
        juce::File file = directory.getChildFile(loadedFileName + "_" +
            featureName + ".txt");
        saveBreakpoints(featureName, file);
    }
}

void AudioDeconstructorProcessor::loadBreakpoints(const juce::String& featureName,
    int outputIndex, const juce::File& file) {

    juce::FileInputStream stream(file);
    if (!stream.openedOk()) return;

    juce::String content = stream.readEntireStreamAsString();
    auto lines = juce::StringArray::fromLines(content);

    std::vector<std::pair<double, double>> points;

    for (const auto& line : lines) {
        juce::String trimmed = line.upToFirstOccurrenceOf("#", false, false).trim();
        if (trimmed.isEmpty()) continue;

        auto tokens = juce::StringArray::fromTokens(trimmed, "\t ", "");
        if (tokens.size() >= 2) {
            double time = tokens[0].getDoubleValue();
            double value = tokens[1].getDoubleValue();
            points.emplace_back(time, value);
        }
    }

    if (featureBreakpoints.find(featureName) == featureBreakpoints.end()) {
        auto extractorIt = extractors.find(featureName);
        if (extractorIt != extractors.end()) {
            int numOutputs = extractorIt->second->getNumOutputs();
            featureBreakpoints[featureName] =
                std::vector<std::vector<std::pair<double, double>>>(numOutputs);
        }
        else {
            return;
        }
    }

    auto& outputs = featureBreakpoints[featureName];
    while (outputs.size() <= outputIndex) {
        outputs.emplace_back();
    }

    outputs[outputIndex] = points;
    sortBreakpoints(featureName, outputIndex);
}

void AudioDeconstructorProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = params.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioDeconstructorProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName(params.state.getType())) {
        params.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorEditor* AudioDeconstructorProcessor::createEditor() {
    return new AudioDeconstructorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AudioDeconstructorProcessor();
}