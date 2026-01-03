//FeatureExtractors.cpp

#include "FeatureExtractors.h"

std::vector<std::vector<std::pair<double, double>>> AmplitudeExtractor::extract(const juce::AudioBuffer<float>& buffer,
    double sampleRate,
    int channel) {

    std::vector<std::vector<std::pair<double, double>>> results(2);

    int windowSamples = static_cast<int>(settings.windowSizeMs * sampleRate / 1000.0f);
    int hopSamples = std::max(1, static_cast<int>(windowSamples * settings.hopSizePct / 100.0f));

    if (windowSamples == 0) windowSamples = 1;

    const float* data = buffer.getReadPointer(channel);
    int numSamples = buffer.getNumSamples();

    std::vector<float> rmsValues;
    std::vector<float> peakValues;
    std::vector<double> times;

    for (int start = 0; start < numSamples; start += hopSamples) {
        int end = std::min(start + windowSamples, numSamples);
        int length = end - start;

        if (length == 0) continue;

        float sumSquares = 0.0f;
        float peak = 0.0f;

        for (int i = start; i < end; ++i) {
            float sample = data[i];
            sumSquares += sample * sample;
            float absSample = std::abs(sample);
            if (absSample > peak) peak = absSample;
        }

        float rms = std::sqrt(sumSquares / length);

        times.push_back(start / sampleRate);
        rmsValues.push_back(rms);
        peakValues.push_back(peak);
    }

    if (settings.normalizeOutput) {
        float maxRms = *std::max_element(rmsValues.begin(), rmsValues.end());
        float maxPeak = *std::max_element(peakValues.begin(), peakValues.end());

        if (maxRms > 0.0f)
            for (auto& v : rmsValues)
                v /= maxRms;

        if (maxPeak > 0.0f)
            for (auto& v : peakValues)
                v /= maxPeak;
    }

    for (size_t i = 0; i < times.size(); ++i) {
        results[0].push_back({ times[i], rmsValues[i] });
        results[1].push_back({ times[i], peakValues[i] });
    }

    return results;
}

std::vector<std::vector<std::pair<double, double>>> PanningExtractor::extract(const juce::AudioBuffer<float>& buffer,
    double sampleRate,
    int channel) {

    std::vector<std::vector<std::pair<double, double>>> results(3);

    if (buffer.getNumChannels() < 2) {
        results[0].push_back({ 0.0, 0.0 });
        results[1].push_back({ 0.0, 0.0 });
        results[2].push_back({ 0.0, 0.0 });
        return results;
    }

    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    int numSamples = buffer.getNumSamples();

    int windowSamples = static_cast<int>(settings.windowSizeMs * sampleRate / 1000.0f);
    int hopSamples = std::max(1, static_cast<int>(windowSamples * settings.hopSizePct / 100.0f));

    for (int start = 0; start < numSamples; start += hopSamples) {
        int end = std::min(start + windowSamples, numSamples);
        int length = end - start;

        if (length == 0) continue;

        double time = start / sampleRate;

        float leftSum = 0.0f, rightSum = 0.0f;
        float leftSq = 0.0f, rightSq = 0.0f;
        float correlation = 0.0f;

        for (int i = start; i < end; ++i) {
            float l = left[i];
            float r = right[i];
            leftSum += std::abs(l);
            rightSum += std::abs(r);
            leftSq += l * l;
            rightSq += r * r;
            correlation += l * r;
        }

        float totalSum = leftSum + rightSum;
        float pan = totalSum > 0.0f ? (rightSum - leftSum) / totalSum : 0.0f;

        float leftRMS = std::sqrt(leftSq / length);
        float rightRMS = std::sqrt(rightSq / length);
        float denom = leftRMS * rightRMS;
        float corr = denom > 0.0f ? correlation / (length * denom) : 0.0f;
        float width = 1.0f - (corr * 0.5f + 0.5f);

        float totalRMS = leftRMS + rightRMS;
        float balance = totalRMS > 0.0f ? (rightRMS - leftRMS) / totalRMS : 0.0f;

        results[0].push_back({ time, pan });
        results[1].push_back({ time, width });
        results[2].push_back({ time, balance });
    }

    return results;
}

SpectralExtractor::SpectralExtractor() {
    fft = std::make_unique<juce::dsp::FFT>(11);
    fftSize = 1 << 11;
    fftData.resize(fftSize * 2, 0.0f);
}

std::vector<std::vector<std::pair<double, double>>> SpectralExtractor::extract(const juce::AudioBuffer<float>& buffer,
    double sampleRate,
    int channel) {

    std::vector<std::vector<std::pair<double, double>>> results(4);

    const float* data = buffer.getReadPointer(channel);
    int numSamples = buffer.getNumSamples();

    int hopSamples = fftSize / 2;
    std::vector<float> previousMagnitudes;

    for (int start = 0; start <= numSamples - fftSize; start += hopSamples) {
        double time = start / sampleRate;

        for (int i = 0; i < fftSize; ++i) {
            float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / fftSize));
            fftData[i] = data[start + i] * window;
        }

        fft->performFrequencyOnlyForwardTransform(fftData.data());

        std::vector<float> magnitudes(fftSize / 2);
        for (int i = 0; i < fftSize / 2; ++i) {
            float real = fftData[i * 2];
            float imag = fftData[i * 2 + 1];
            magnitudes[i] = std::sqrt(real * real + imag * imag);
        }

        float centroid = calculateCentroid(magnitudes, sampleRate);
        float flux = previousMagnitudes.empty() ? 0.0f : calculateFlux(magnitudes, previousMagnitudes);
        float flatness = calculateFlatness(magnitudes);
        float rolloff = calculateRolloff(magnitudes, sampleRate);

        results[0].push_back({ time, centroid });
        results[1].push_back({ time, flux });
        results[2].push_back({ time, flatness });
        results[3].push_back({ time, rolloff });

        previousMagnitudes = magnitudes;
    }

    return results;
}

float SpectralExtractor::calculateCentroid(const std::vector<float>& magnitudes, double sampleRate) {
    float weightedSum = 0.0f;
    float totalSum = 0.0f;

    for (size_t i = 0; i < magnitudes.size(); ++i) {
        float freq = i * sampleRate / (fftSize * 2.0f);
        weightedSum += freq * magnitudes[i];
        totalSum += magnitudes[i];
    }

    return totalSum > 0.0f ? weightedSum / totalSum : 0.0f;
}

float SpectralExtractor::calculateFlux(const std::vector<float>& current, const std::vector<float>& previous) {
    float sum = 0.0f;
    for (size_t i = 0; i < current.size(); ++i) {
        float diff = current[i] - previous[i];
        sum += diff * diff;
    }
    return std::sqrt(sum / current.size());
}

float SpectralExtractor::calculateFlatness(const std::vector<float>& magnitudes) {
    float geometricMean = 0.0f;
    float arithmeticMean = 0.0f;
    int count = 0;

    for (auto mag : magnitudes) {
        if (mag > 0.0f) {
            geometricMean += std::log(mag);
            arithmeticMean += mag;
            count++;
        }
    }

    if (count == 0) return 0.0f;

    geometricMean = std::exp(geometricMean / count);
    arithmeticMean /= count;

    return arithmeticMean > 0.0f ? geometricMean / arithmeticMean : 0.0f;
}

float SpectralExtractor::calculateRolloff(const std::vector<float>& magnitudes, double sampleRate) {
    float totalEnergy = 0.0f;
    for (auto mag : magnitudes)
        totalEnergy += mag;

    float threshold = totalEnergy * 0.85f;
    float cumulativeEnergy = 0.0f;

    for (size_t i = 0; i < magnitudes.size(); ++i) {
        cumulativeEnergy += magnitudes[i];
        if (cumulativeEnergy >= threshold)
            return i * sampleRate / (fftSize * 2.0f);
    }

    return sampleRate / 2.0f;
}

std::vector<std::vector<std::pair<double, double>>> PitchExtractor::extract(const juce::AudioBuffer<float>& buffer,
    double sampleRate,
    int channel) {

    std::vector<std::vector<std::pair<double, double>>> results(2);

    const float* data = buffer.getReadPointer(channel);
    int numSamples = buffer.getNumSamples();

    int windowSamples = static_cast<int>(0.05 * sampleRate);
    int hopSamples = windowSamples / 2;

    for (int start = 0; start < numSamples - windowSamples; start += hopSamples) {
        double time = start / sampleRate;

        auto [freq, confidence] = detectPitch(data + start, windowSamples, sampleRate);

        results[0].push_back({ time, freq });
        results[1].push_back({ time, confidence });
    }

    return results;
}

std::pair<float, float> PitchExtractor::detectPitch(const float* data, int numSamples, double sampleRate) {
    int minLag = static_cast<int>(sampleRate / 1000.0);
    int maxLag = static_cast<int>(sampleRate / 50.0);

    std::vector<float> correlation(maxLag - minLag);

    for (int lag = minLag; lag < maxLag; ++lag) {
        float sum = 0.0f;
        for (int i = 0; i < numSamples - lag; ++i)
            sum += data[i] * data[i + lag];
        correlation[lag - minLag] = sum;
    }

    auto maxIt = std::max_element(correlation.begin(), correlation.end());
    int peakLag = static_cast<int>(std::distance(correlation.begin(), maxIt)) + minLag;

    float freq = static_cast<float>(sampleRate / peakLag);
    float confidence = *maxIt / correlation[0];

    return { freq, std::max(0.0f, std::min(1.0f, confidence)) };
}

std::vector<std::vector<std::pair<double, double>>> TransientExtractor::extract(const juce::AudioBuffer<float>& buffer,
    double sampleRate,
    int channel) {

    std::vector<std::vector<std::pair<double, double>>> results(1);

    const float* data = buffer.getReadPointer(channel);
    int numSamples = buffer.getNumSamples();

    int windowSamples = 1024;
    int hopSamples = 512;

    float previousEnergy = 0.0f;

    for (int start = 0; start < numSamples - windowSamples; start += hopSamples) {
        double time = start / sampleRate;

        float energy = 0.0f;
        for (int i = 0; i < windowSamples; ++i) {
            float sample = data[start + i];
            energy += sample * sample;
        }
        energy = std::sqrt(energy / windowSamples);

        float onsetStrength = std::max(0.0f, energy - previousEnergy);

        results[0].push_back({ time, onsetStrength });

        previousEnergy = energy;
    }

    return results;
}

std::unique_ptr<FeatureExtractor> FeatureExtractorFactory::createExtractor(const juce::String& name) {
    if (name == "Amplitude") return std::make_unique<AmplitudeExtractor>();
    if (name == "Panning") return std::make_unique<PanningExtractor>();
    if (name == "Spectral") return std::make_unique<SpectralExtractor>();
    if (name == "Pitch") return std::make_unique<PitchExtractor>();
    if (name == "Transients") return std::make_unique<TransientExtractor>();
    return nullptr;
}

juce::StringArray FeatureExtractorFactory::getAvailableFeatures() {
    return juce::StringArray{ "Amplitude", "Panning", "Spectral", "Pitch", "Transients" };
}

juce::Colour FeatureExtractorFactory::getFeatureColour(const juce::String& name) {
    static std::map<juce::String, juce::Colour> colours = {
        {"Amplitude", juce::Colours::green},
        {"Panning", juce::Colours::blue},
        {"Spectral", juce::Colours::purple},
        {"Pitch", juce::Colours::orange},
        {"Transients", juce::Colours::red}
    };
    auto it = colours.find(name);
    return it != colours.end() ? it->second : juce::Colours::white;
}