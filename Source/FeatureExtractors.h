// FeatureExtractors.h
#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

class FeatureExtractor {
public:
    virtual ~FeatureExtractor() = default;

    virtual juce::String getName() const = 0;
    virtual juce::Colour getColor() const = 0;
    virtual bool supportsMultiChannel() const { return false; }
    virtual int getNumOutputs() const { return 1; }
    virtual juce::String getOutputName(int index) const { return getName(); }

    struct Settings {
        float windowSizeMs = 15.0f;
        float hopSizePct = 50.0f;
        bool normalizeOutput = true;
        float minValue = -1.0f;
        float maxValue = 1.0f;
        bool smoothOutput = false;
        float smoothTimeMs = 10.0f;
    };

    Settings settings;

    virtual std::vector<std::vector<std::pair<double, double>>> extract(const juce::AudioBuffer<float>& buffer,
        double sampleRate,
        int channel = 0) = 0;
};

class AmplitudeExtractor : public FeatureExtractor {
public:
    juce::String getName() const override { return "Amplitude"; }
    juce::Colour getColor() const override { return juce::Colours::green; }
    bool supportsMultiChannel() const override { return true; }
    int getNumOutputs() const override { return 2; }
    juce::String getOutputName(int index) const override { return index == 0 ? "RMS" : "Peak"; }

    std::vector<std::vector<std::pair<double, double>>> extract(const juce::AudioBuffer<float>& buffer,
        double sampleRate,
        int channel = 0) override;
};

class PanningExtractor : public FeatureExtractor {
public:
    juce::String getName() const override { return "Panning"; }
    juce::Colour getColor() const override { return juce::Colours::blue; }
    bool supportsMultiChannel() const override { return true; }
    int getNumOutputs() const override { return 3; }
    juce::String getOutputName(int index) const override {
        switch (index) {
        case 0: return "Pan Position";
        case 1: return "Stereo Width";
        case 2: return "Balance";
        default: return "Output";
        }
    }

    std::vector<std::vector<std::pair<double, double>>> extract(const juce::AudioBuffer<float>& buffer,
        double sampleRate,
        int channel = 0) override;
};

class SpectralExtractor : public FeatureExtractor {
public:
    SpectralExtractor();

    juce::String getName() const override { return "Spectral"; }
    juce::Colour getColor() const override { return juce::Colours::purple; }
    int getNumOutputs() const override { return 4; }
    juce::String getOutputName(int index) const override {
        switch (index) {
        case 0: return "Centroid";
        case 1: return "Flux";
        case 2: return "Flatness";
        case 3: return "Rolloff";
        default: return "Output";
        }
    }

    std::vector<std::vector<std::pair<double, double>>> extract(const juce::AudioBuffer<float>& buffer,
        double sampleRate,
        int channel = 0) override;

private:
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> fftData;
    int fftSize;

    float calculateCentroid(const std::vector<float>& magnitudes, double sampleRate);
    float calculateFlux(const std::vector<float>& current, const std::vector<float>& previous);
    float calculateFlatness(const std::vector<float>& magnitudes);
    float calculateRolloff(const std::vector<float>& magnitudes, double sampleRate);
};

class PitchExtractor : public FeatureExtractor {
public:
    juce::String getName() const override { return "Pitch"; }
    juce::Colour getColor() const override { return juce::Colours::orange; }
    int getNumOutputs() const override { return 2; }
    juce::String getOutputName(int index) const override { return index == 0 ? "Frequency" : "Confidence"; }

    std::vector<std::vector<std::pair<double, double>>> extract(const juce::AudioBuffer<float>& buffer,
        double sampleRate,
        int channel = 0) override;

private:
    std::pair<float, float> detectPitch(const float* data, int numSamples, double sampleRate);
};

class TransientExtractor : public FeatureExtractor {
public:
    juce::String getName() const override { return "Transients"; }
    juce::Colour getColor() const override { return juce::Colours::red; }
    int getNumOutputs() const override { return 1; }
    juce::String getOutputName(int index) const override { return "Onset Strength"; }

    std::vector<std::vector<std::pair<double, double>>> extract(const juce::AudioBuffer<float>& buffer,
        double sampleRate,
        int channel = 0) override;
};

class FeatureExtractorFactory {
public:
    static std::unique_ptr<FeatureExtractor> createExtractor(const juce::String& name);
    static juce::StringArray getAvailableFeatures();
    static juce::Colour getFeatureColour(const juce::String& name);
};