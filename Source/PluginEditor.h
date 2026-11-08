#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class RayKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override;

    float sunAmt = 0.0f, sunPhase = 0.0f;
    juce::Colour sunCol { 0xfff4e3b0 };
};

class RayAudioProcessorEditor : public juce::AudioProcessorEditor,
                                private juce::Timer
{
public:
    explicit RayAudioProcessorEditor (RayAudioProcessor&);
    ~RayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void timerCallback() override;
    void setupKnob (juce::Slider&);

    void regradeIfNeeded();
    float satMul() const;
    float hueTurns() const;
    float inputNorm() const;
    float threshNorm() const;

    void drawWaveform (juce::Graphics&, juce::Rectangle<float> box);
    void drawPanelDecor (juce::Graphics&, float s);
    void drawLabels (juce::Graphics&);
    void drawWindow (juce::Graphics&, float s);

    RayAudioProcessor& proc;
    juce::Image background;
    juce::Image gradedBg;
    juce::Image grain;

    RayKnobLookAndFeel knobLnf;
    juce::Slider inputKnob, thresholdKnob, posSatKnob, negSatKnob;
    std::unique_ptr<SliderAttachment> inputAtt, thresholdAtt, posSatAtt, negSatAtt;

    float gradedSat = -1.0f, gradedHue = -1.0f;
    int   gradedW = 0;
    float phase = 0.0f;

    juce::Rectangle<float> boxDesign { 355.0f, 305.0f, 330.0f, 73.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RayAudioProcessorEditor)
};
