#pragma once
#include <JuceHeader.h>

class MultiFeatureDisplay : public juce::Component {
public:
    MultiFeatureDisplay();
    ~MultiFeatureDisplay() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setFeatures(const std::map<juce::String,
        std::vector<std::vector<std::pair<double, double>>>>&features);
    void setActiveFeature(const juce::String& feature, int outputIndex = 0);
    void setShowAllFeatures(bool showAll);
    void setTimeRange(double startTime, double endTime);

private:
    struct FeatureInfo {
        juce::Colour color;
        std::vector<std::pair<double, double>> points;
        juce::String name;
    };

    std::map<juce::String, FeatureInfo> features;
    juce::String activeFeature;
    int activeOutput = 0;
    bool showAllFeatures = false;
    double viewStartTime = 0.0;
    double viewEndTime = 5.0; // 5 second default view

    juce::Rectangle<int> getGraphArea() const;
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawFeature(juce::Graphics& g, const juce::Rectangle<int>& area,
        const FeatureInfo& feature);
    void drawAllFeatures(juce::Graphics& g, const juce::Rectangle<int>& area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiFeatureDisplay)
};