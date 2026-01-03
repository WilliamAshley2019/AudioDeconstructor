// ============================================================================
// PluginProcessor.h
// ============================================================================
#pragma once
#include <JuceHeader.h>
#include "FeatureExtractors.h"

class AudioDeconstructorProcessor : public juce::AudioProcessor {
public:
    AudioDeconstructorProcessor();
    ~AudioDeconstructorProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Audio Deconstructor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // Audio file loading
    bool loadAudioFile(const juce::File& file);
    void clearLoadedAudio();
    bool hasLoadedAudio() const { return loadedAudio.getNumSamples() > 0; }
    const juce::AudioBuffer<float>& getLoadedAudio() const { return loadedAudio; }
    double getLoadedSampleRate() const { return loadedSampleRate; }
    juce::String getLoadedFileName() const { return loadedFileName; }

    // Feature extraction
    void extractFeature(const juce::String& featureName, int channel = 0);
    void extractAllFeatures();
    bool isFeatureExtracted(const juce::String& featureName) const;
    juce::StringArray getExtractedFeatures() const;

    // Feature information
    juce::StringArray getAvailableFeatures() const;
    juce::Colour getFeatureColour(const juce::String& featureName) const;
    int getNumOutputsForFeature(const juce::String& featureName) const;
    juce::String getOutputName(const juce::String& featureName, int outputIndex) const;

    // Breakpoint access 
    std::vector<std::pair<double, double>> getBreakpointsForDisplay(
        const juce::String& featureName, int outputIndex = 0) const;

    // Breakpoint editing 
    void addBreakpoint(const juce::String& featureName, int outputIndex,
        double time, double value);
    void updateBreakpoint(const juce::String& featureName, int outputIndex,
        size_t pointIndex, double time, double value);
    void removeBreakpoint(const juce::String& featureName, int outputIndex,
        size_t pointIndex);

    // File I/O
    void saveBreakpoints(const juce::String& featureName, const juce::File& file);
    void saveAllBreakpoints(const juce::File& directory);
    void loadBreakpoints(const juce::String& featureName, int outputIndex,
        const juce::File& file);

    juce::AudioProcessorValueTreeState params;

private:
    juce::AudioBuffer<float> loadedAudio;
    double loadedSampleRate = 44100.0;
    juce::String loadedFileName;

    std::map<juce::String, std::unique_ptr<FeatureExtractor>> extractors;
    std::map<juce::String, std::vector<std::vector<std::pair<double, double>>>> featureBreakpoints;

    std::atomic<bool> isAnalyzing{ false };
    std::atomic<float> analysisProgress{ 0.0f };

    void initializeExtractors();
    void sortBreakpoints(const juce::String& featureName, int outputIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeconstructorProcessor)
};