//PluginEditor.h
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AudioDeconstructorEditor : public juce::AudioProcessorEditor,
    private juce::Timer,
    private juce::FileDragAndDropTarget,
    private juce::ComboBox::Listener,
    private juce::Button::Listener {
public:
    explicit AudioDeconstructorEditor(AudioDeconstructorProcessor&);
    ~AudioDeconstructorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    AudioDeconstructorProcessor& processor;

    juce::TextButton loadButton;
    juce::TextButton extractButton;
    juce::TextButton saveButton;
    juce::TextButton saveAllButton;
    juce::TextButton clearButton;

    juce::ComboBox featureSelector;
    juce::ComboBox outputSelector;
    juce::Label featureLabel;
    juce::Label outputLabel;

    juce::Slider windowSizeSlider;
    juce::Slider hopSizeSlider;
    juce::ToggleButton normalizeToggle;

    juce::Label infoLabel;
    juce::Label statusLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::Rectangle<int> graphBounds;
    std::vector<std::pair<float, float>> displayedBreakpoints;
    juce::String currentFeature;
    int currentOutput = 0;

    struct DraggedBreakpoint {
        int index = -1;
        juce::Point<float> dragStartPosition;
        float originalTime = 0.0f;
        float originalValue = 0.0f;
    };
    DraggedBreakpoint draggedBreakpoint;
    bool isDragging = false;

    void timerCallback() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray& files, int, int) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked(juce::Button* button) override;

    int findBreakpointAtPosition(juce::Point<float> position, float tolerance = 8.0f);
    juce::Point<float> timeValueToScreen(float time, float value);
    std::pair<float, float> screenToTimeValue(juce::Point<float> screenPos);
    void updateBreakpointFromDrag(juce::Point<float> currentPosition);
    void addBreakpointAtPosition(juce::Point<float> position);
    void removeBreakpointAtPosition(juce::Point<float> position);

    void loadAudioFile();
    void extractFeatures();
    void saveCurrentBreakpoints();
    void saveAllBreakpoints();
    void clearAll();
    void updateDisplay();
    void updateFeatureSelector();
    void updateOutputSelector();

    void drawGraphBackground(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawWaveform(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawAudioWaveform(juce::Graphics& g, const juce::Rectangle<int>& area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeconstructorEditor)
};