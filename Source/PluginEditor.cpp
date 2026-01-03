// ============================================================================
// PluginEditor.cpp 
// ============================================================================
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "FeatureExtractors.h"

AudioDeconstructorEditor::AudioDeconstructorEditor(AudioDeconstructorProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {

    loadButton.setButtonText("Load Audio");
    loadButton.addListener(this);
    addAndMakeVisible(loadButton);

    extractButton.setButtonText("Extract");
    extractButton.addListener(this);
    addAndMakeVisible(extractButton);

    saveButton.setButtonText("Save");
    saveButton.addListener(this);
    addAndMakeVisible(saveButton);

    saveAllButton.setButtonText("Save All");
    saveAllButton.addListener(this);
    addAndMakeVisible(saveAllButton);

    clearButton.setButtonText("Clear");
    clearButton.addListener(this);
    addAndMakeVisible(clearButton);

    featureLabel.setText("Feature:", juce::dontSendNotification);
    addAndMakeVisible(featureLabel);

    featureSelector.addListener(this);
    addAndMakeVisible(featureSelector);

    outputLabel.setText("Output:", juce::dontSendNotification);
    addAndMakeVisible(outputLabel);

    outputSelector.addListener(this);
    addAndMakeVisible(outputSelector);

    windowSizeSlider.setRange(1.0, 100.0, 0.1);
    windowSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 24);
    windowSizeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    addAndMakeVisible(windowSizeSlider);

    hopSizeSlider.setRange(10.0, 90.0, 1.0);
    hopSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 24);
    hopSizeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    addAndMakeVisible(hopSizeSlider);

    normalizeToggle.setButtonText("Normalize");
    addAndMakeVisible(normalizeToggle);

    infoLabel.setText("Load an audio file or drag & drop here", juce::dontSendNotification);
    infoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(infoLabel);

    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    updateFeatureSelector();

    setSize(800, 700);
    startTimerHz(30);
}

AudioDeconstructorEditor::~AudioDeconstructorEditor() {
    stopTimer();
}

void AudioDeconstructorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1e1e1e));

    g.setColour(juce::Colours::white);
    g.setFont(22.0f);
    g.drawText("Audio Deconstructor", getLocalBounds().removeFromTop(40),
        juce::Justification::centred);

    graphBounds = getLocalBounds().withTrimmedTop(160).withHeight(300).reduced(10, 10);
    drawGraphBackground(g, graphBounds);

    if (processor.hasLoadedAudio()) {
        drawAudioWaveform(g, graphBounds);
    }

    drawWaveform(g, graphBounds);
}

void AudioDeconstructorEditor::drawGraphBackground(juce::Graphics& g,
    const juce::Rectangle<int>& area) {

    g.setColour(juce::Colour(0xff2d2d2d));
    g.fillRect(area);

    g.setColour(juce::Colour(0xff444444));
    g.drawRect(area, 1);

    g.setColour(juce::Colour(0xff333333));
    for (int i = 0; i <= 4; ++i) {
        float y = area.getY() + (area.getHeight() * i / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y),
            static_cast<float>(area.getX()),
            static_cast<float>(area.getRight()));
    }

    for (int i = 0; i <= 10; ++i) {
        float x = area.getX() + (area.getWidth() * i / 10.0f);
        g.drawVerticalLine(static_cast<int>(x),
            static_cast<float>(area.getY()),
            static_cast<float>(area.getBottom()));
    }

    g.setColour(juce::Colour(0xff666666));
    g.drawHorizontalLine(area.getCentreY(),
        static_cast<float>(area.getX()),
        static_cast<float>(area.getRight()));
}

void AudioDeconstructorEditor::drawAudioWaveform(juce::Graphics& g,
    const juce::Rectangle<int>& area) {

    const auto& buffer = processor.getLoadedAudio();
    if (buffer.getNumSamples() == 0) return;

    g.setColour(juce::Colours::grey.withAlpha(0.3f));

    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();

    juce::Path path;
    path.startNewSubPath(area.getX(), area.getCentreY());

    int step = juce::jmax(1, numSamples / area.getWidth());

    for (int i = 0; i < numSamples; i += step) {
        float x = area.getX() + (i * area.getWidth() / (float)numSamples);
        float y = area.getCentreY() - (data[i] * area.getHeight() * 0.4f);
        path.lineTo(x, y);
    }

    g.strokePath(path, juce::PathStrokeType(1.0f));
}

void AudioDeconstructorEditor::drawWaveform(juce::Graphics& g,
    const juce::Rectangle<int>& area) {

    if (displayedBreakpoints.empty()) return;

    juce::Colour featureColour = processor.getFeatureColour(currentFeature);
    g.setColour(featureColour.withAlpha(0.9f));

    juce::Path path;
    bool first = true;

    float maxTime = 0.0f;
    float minValue = 1e10f;
    float maxValue = -1e10f;

    for (const auto& [time, value] : displayedBreakpoints) {
        maxTime = juce::jmax(maxTime, time);
        minValue = juce::jmin(minValue, value);
        maxValue = juce::jmax(maxValue, value);
    }

    if (maxTime <= 0.0f) maxTime = 1.0f;
    float valueRange = maxValue - minValue;
    if (valueRange < 0.001f) valueRange = 1.0f;

    for (size_t i = 0; i < displayedBreakpoints.size(); ++i) {
        const auto& [time, value] = displayedBreakpoints[i];
        float x = area.getX() + (time / maxTime) * area.getWidth();
        float normalizedValue = (value - minValue) / valueRange;
        float y = area.getY() + area.getHeight() * (1.0f - normalizedValue);

        if (i == draggedBreakpoint.index && isDragging) {
            g.setColour(juce::Colours::red);
        }
        else {
            g.setColour(juce::Colours::yellow);
        }

        g.fillEllipse(x - 6, y - 6, 12, 12);
        g.setColour(juce::Colours::black);
        g.drawEllipse(x - 6, y - 6, 12, 12, 1.5f);

        g.setColour(juce::Colours::white);
        g.setFont(9.0f);
        g.drawText(juce::String(i),
            static_cast<int>(x - 10), static_cast<int>(y - 22),
            20, 15, juce::Justification::centred);
    }
}

void AudioDeconstructorEditor::resized() {
    auto area = getLocalBounds();
    auto header = area.removeFromTop(40);

    graphBounds = area.removeFromTop(300).reduced(10, 10);

    auto controlRow1 = area.removeFromTop(40).reduced(10, 5);
    loadButton.setBounds(controlRow1.removeFromLeft(90));
    controlRow1.removeFromLeft(10);
    extractButton.setBounds(controlRow1.removeFromLeft(80));
    controlRow1.removeFromLeft(10);
    saveButton.setBounds(controlRow1.removeFromLeft(70));
    controlRow1.removeFromLeft(5);
    saveAllButton.setBounds(controlRow1.removeFromLeft(80));
    controlRow1.removeFromLeft(5);
    clearButton.setBounds(controlRow1.removeFromLeft(70));

    auto controlRow2 = area.removeFromTop(40).reduced(10, 5);
    featureLabel.setBounds(controlRow2.removeFromLeft(60));
    controlRow2.removeFromLeft(5);
    featureSelector.setBounds(controlRow2.removeFromLeft(140));
    controlRow2.removeFromLeft(15);
    outputLabel.setBounds(controlRow2.removeFromLeft(60));
    controlRow2.removeFromLeft(5);
    outputSelector.setBounds(controlRow2.removeFromLeft(140));

    auto controlRow3 = area.removeFromTop(40).reduced(10, 5);
    windowSizeSlider.setBounds(controlRow3.removeFromLeft(200));
    controlRow3.removeFromLeft(15);
    hopSizeSlider.setBounds(controlRow3.removeFromLeft(200));
    controlRow3.removeFromLeft(10);
    normalizeToggle.setBounds(controlRow3.removeFromLeft(100));

    auto statusRow = area.removeFromTop(30).reduced(10, 5);
    infoLabel.setBounds(statusRow.removeFromLeft(400));
    statusLabel.setBounds(statusRow);
}

void AudioDeconstructorEditor::timerCallback() {
    updateDisplay();
    repaint();
}

bool AudioDeconstructorEditor::isInterestedInFileDrag(const juce::StringArray& files) {
    for (const auto& file : files) {
        if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aif") ||
            file.endsWithIgnoreCase(".aiff") || file.endsWithIgnoreCase(".mp3") ||
            file.endsWithIgnoreCase(".flac")) {
            return true;
        }
    }
    return false;
}

void AudioDeconstructorEditor::filesDropped(const juce::StringArray& files, int, int) {
    for (const auto& file : files) {
        if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aif") ||
            file.endsWithIgnoreCase(".aiff") || file.endsWithIgnoreCase(".mp3") ||
            file.endsWithIgnoreCase(".flac")) {
            if (processor.loadAudioFile(juce::File(file))) {
                infoLabel.setText("Loaded: " + juce::File(file).getFileName(),
                    juce::dontSendNotification);
                statusLabel.setText("Ready to extract features", juce::dontSendNotification);
                updateFeatureSelector();
                repaint();
            }
            break;
        }
    }
}

void AudioDeconstructorEditor::comboBoxChanged(juce::ComboBox* combo) {
    if (combo == &featureSelector) {
        currentFeature = featureSelector.getText();
        updateOutputSelector();
        updateDisplay();
    }
    else if (combo == &outputSelector) {
        currentOutput = outputSelector.getSelectedId() - 1;
        updateDisplay();
    }
}

void AudioDeconstructorEditor::buttonClicked(juce::Button* button) {
    if (button == &loadButton) {
        loadAudioFile();
    }
    else if (button == &extractButton) {
        extractFeatures();
    }
    else if (button == &saveButton) {
        saveCurrentBreakpoints();
    }
    else if (button == &saveAllButton) {
        saveAllBreakpoints();
    }
    else if (button == &clearButton) {
        clearAll();
    }
}

void AudioDeconstructorEditor::loadAudioFile() {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Audio File",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.wav;*.aif;*.aiff;*.mp3;*.flac"
    );

    auto browserFlags = juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(browserFlags, [this](const juce::FileChooser& chooser) {
        auto result = chooser.getResult();
        if (result.existsAsFile()) {
            if (processor.loadAudioFile(result)) {
                infoLabel.setText("Loaded: " + result.getFileName(),
                    juce::dontSendNotification);
                statusLabel.setText("Ready to extract", juce::dontSendNotification);
                updateFeatureSelector();
                repaint();
            }
        }
        });
}

void AudioDeconstructorEditor::extractFeatures() {
    if (!processor.hasLoadedAudio()) {
        statusLabel.setText("Please load audio first", juce::dontSendNotification);
        return;
    }

    if (currentFeature.isNotEmpty()) {
        processor.extractFeature(currentFeature, 0);
        statusLabel.setText("Extracted: " + currentFeature, juce::dontSendNotification);
    }
    else {
        processor.extractAllFeatures();
        statusLabel.setText("Extracted all features", juce::dontSendNotification);
    }

    updateDisplay();
}

void AudioDeconstructorEditor::saveCurrentBreakpoints() {
    if (currentFeature.isEmpty()) {
        statusLabel.setText("Select a feature first", juce::dontSendNotification);
        return;
    }

    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Breakpoint File",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile(processor.getLoadedFileName() + "_" + currentFeature + ".txt"),
        "*.txt"
    );

    auto browserFlags = juce::FileBrowserComponent::saveMode |
        juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(browserFlags, [this](const juce::FileChooser& chooser) {
        auto result = chooser.getResult();
        if (result.getFullPathName().isNotEmpty()) {
            processor.saveBreakpoints(currentFeature, result);
            statusLabel.setText("Saved: " + result.getFileName(), juce::dontSendNotification);
        }
        });
}

void AudioDeconstructorEditor::saveAllBreakpoints() {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Directory to Save All Breakpoints",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        ""
    );

    auto browserFlags = juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(browserFlags, [this](const juce::FileChooser& chooser) {
        auto result = chooser.getResult();
        if (result.exists()) {
            processor.saveAllBreakpoints(result);
            statusLabel.setText("Saved all breakpoints", juce::dontSendNotification);
        }
        });
}

void AudioDeconstructorEditor::clearAll() {
    processor.clearLoadedAudio();
    displayedBreakpoints.clear();
    updateFeatureSelector();
    updateOutputSelector();
    infoLabel.setText("Load an audio file or drag & drop here",
        juce::dontSendNotification);
    statusLabel.setText("Ready", juce::dontSendNotification);
    repaint();
}

void AudioDeconstructorEditor::updateDisplay() {
    if (currentFeature.isNotEmpty()) {
        auto points = processor.getBreakpointsForDisplay(currentFeature, currentOutput);
        displayedBreakpoints.clear();
        for (const auto& p : points) {
            displayedBreakpoints.push_back({ static_cast<float>(p.first),
                                           static_cast<float>(p.second) });
        }
    }
}

void AudioDeconstructorEditor::updateFeatureSelector() {
    featureSelector.clear();
    auto features = processor.getAvailableFeatures();

    for (int i = 0; i < features.size(); ++i) {
        featureSelector.addItem(features[i], i + 1);
    }

    if (!currentFeature.isEmpty() && features.contains(currentFeature)) {
        featureSelector.setText(currentFeature, juce::dontSendNotification);
    }
    else if (features.size() > 0) {
        currentFeature = features[0];
        featureSelector.setSelectedId(1, juce::dontSendNotification);
    }

    updateOutputSelector();
}

void AudioDeconstructorEditor::updateOutputSelector() {
    outputSelector.clear();

    if (currentFeature.isNotEmpty()) {
        int numOutputs = processor.getNumOutputsForFeature(currentFeature);
        for (int i = 0; i < numOutputs; ++i) {
            juce::String outputName = processor.getOutputName(currentFeature, i);
            outputSelector.addItem(outputName, i + 1);
        }

        if (numOutputs > 0) {
            currentOutput = 0;
            outputSelector.setSelectedId(1, juce::dontSendNotification);
        }
    }
}

int AudioDeconstructorEditor::findBreakpointAtPosition(juce::Point<float> position,
    float tolerance) {

    if (displayedBreakpoints.empty()) return -1;

    float maxTime = 0.0f;
    float minValue = 1e10f;
    float maxValue = -1e10f;

    for (const auto& [time, value] : displayedBreakpoints) {
        maxTime = juce::jmax(maxTime, time);
        minValue = juce::jmin(minValue, value);
        maxValue = juce::jmax(maxValue, value);
    }

    if (maxTime <= 0.0f) maxTime = 1.0f;
    float valueRange = maxValue - minValue;
    if (valueRange < 0.001f) valueRange = 1.0f;

    for (size_t i = 0; i < displayedBreakpoints.size(); ++i) {
        const auto& [time, value] = displayedBreakpoints[i];
        float x = graphBounds.getX() + (time / maxTime) * graphBounds.getWidth();
        float normalizedValue = (value - minValue) / valueRange;
        float y = graphBounds.getY() + graphBounds.getHeight() * (1.0f - normalizedValue);

        if (std::abs(x - position.x) <= tolerance &&
            std::abs(y - position.y) <= tolerance) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

juce::Point<float> AudioDeconstructorEditor::timeValueToScreen(float time, float value) {
    float maxTime = 1.0f;
    float minValue = 1e10f;
    float maxValue = -1e10f;

    for (const auto& [t, v] : displayedBreakpoints) {
        maxTime = juce::jmax(maxTime, t);
        minValue = juce::jmin(minValue, v);
        maxValue = juce::jmax(maxValue, v);
    }

    if (maxTime <= 0.0f) maxTime = 1.0f;
    float valueRange = maxValue - minValue;
    if (valueRange < 0.001f) valueRange = 1.0f;

    float x = graphBounds.getX() + (time / maxTime) * graphBounds.getWidth();
    float normalizedValue = (value - minValue) / valueRange;
    float y = graphBounds.getY() + graphBounds.getHeight() * (1.0f - normalizedValue);

    return { x, y };
}

std::pair<float, float> AudioDeconstructorEditor::screenToTimeValue(
    juce::Point<float> screenPos) {

    float maxTime = 1.0f;
    float minValue = 1e10f;
    float maxValue = -1e10f;

    for (const auto& [time, value] : displayedBreakpoints) {
        maxTime = juce::jmax(maxTime, time);
        minValue = juce::jmin(minValue, value);
        maxValue = juce::jmax(maxValue, value);
    }

    if (maxTime <= 0.0f) maxTime = 1.0f;
    float valueRange = maxValue - minValue;
    if (valueRange < 0.001f) valueRange = 1.0f;

    float time = ((screenPos.x - graphBounds.getX()) / graphBounds.getWidth()) * maxTime;
    float normalizedValue = 1.0f - ((screenPos.y - graphBounds.getY()) /
        graphBounds.getHeight());
    float value = minValue + normalizedValue * valueRange;

    return { juce::jmax(0.0f, time), value };
}

void AudioDeconstructorEditor::updateBreakpointFromDrag(juce::Point<float> currentPosition) {
    if (draggedBreakpoint.index >= 0 && currentFeature.isNotEmpty()) {
        auto [newTime, newValue] = screenToTimeValue(currentPosition);
        processor.updateBreakpoint(currentFeature, currentOutput,
            draggedBreakpoint.index, newTime, newValue);
        updateDisplay();
    }
}

void AudioDeconstructorEditor::addBreakpointAtPosition(juce::Point<float> position) {
    if (graphBounds.contains(position.toInt()) && currentFeature.isNotEmpty()) {
        auto [time, value] = screenToTimeValue(position);
        processor.addBreakpoint(currentFeature, currentOutput, time, value);
        updateDisplay();
        statusLabel.setText("Added breakpoint at " + juce::String(time, 2) + "s",
            juce::dontSendNotification);
    }
}

void AudioDeconstructorEditor::removeBreakpointAtPosition(juce::Point<float> position) {
    int index = findBreakpointAtPosition(position);
    if (index >= 0 && currentFeature.isNotEmpty()) {
        processor.removeBreakpoint(currentFeature, currentOutput, index);
        updateDisplay();
        statusLabel.setText("Removed breakpoint " + juce::String(index),
            juce::dontSendNotification);
    }
}

void AudioDeconstructorEditor::mouseDown(const juce::MouseEvent& event) {
    if (graphBounds.contains(event.getPosition())) {
        if (event.mods.isLeftButtonDown()) {
            int index = findBreakpointAtPosition(event.position);
            if (index >= 0) {
                draggedBreakpoint.index = index;
                draggedBreakpoint.dragStartPosition = event.position;
                isDragging = true;
            }
        }
        else if (event.mods.isRightButtonDown()) {
            removeBreakpointAtPosition(event.position);
        }
    }
}

void AudioDeconstructorEditor::mouseDrag(const juce::MouseEvent& event) {
    if (isDragging && event.mods.isLeftButtonDown()) {
        updateBreakpointFromDrag(event.position);
    }
}

void AudioDeconstructorEditor::mouseUp(const juce::MouseEvent&) {
    if (isDragging) {
        isDragging = false;
        statusLabel.setText("Breakpoint updated", juce::dontSendNotification);
    }
}

void AudioDeconstructorEditor::mouseDoubleClick(const juce::MouseEvent& event) {
    if (graphBounds.contains(event.getPosition()) && event.mods.isLeftButtonDown()) {
        addBreakpointAtPosition(event.position);
    }
}