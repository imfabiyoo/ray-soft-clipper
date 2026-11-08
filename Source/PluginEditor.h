#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

// Brushed-metal analog knob: a fixed machined body with a rotating pointer +
// specular streak. No 3D sphere — modeled after a hardware control.
class RayKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override;
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

    // --- colour grading driven by the saturation knobs -----------------------
    void regradeIfNeeded();                 // rebuild gradedBg when sat/hue moves
    float satMul() const;                   // positive-sat knob -> colour saturation
    float hueTurns() const;                 // negative-sat knob -> hue rotation (0..1)
    float inputNorm() const;                // 0..1 for sun-ray intensity
    float threshNorm() const;               // 0..1 for cloud amount

    // --- waveform scope ------------------------------------------------------
    void drawWaveform (juce::Graphics&, juce::Rectangle<float> box);
    void drawPanelDecor (juce::Graphics&, float s);   // grain + screws = tangible

    RayAudioProcessor& proc;
    juce::Image background;      // master 1200x800 (forest + panel)
    juce::Image gradedBg;        // hue/sat-graded working copy (component-ish res)
    juce::Image grain;           // static noise overlay for material texture

    RayKnobLookAndFeel knobLnf;
    juce::Slider inputKnob, thresholdKnob, posSatKnob, negSatKnob;
    std::unique_ptr<SliderAttachment> inputAtt, thresholdAtt, posSatAtt, negSatAtt;

    // grade cache keys
    float gradedSat = -1.0f, gradedHue = -1.0f;
    int   gradedW = 0;
    float phase = 0.0f;          // animation clock for rays/clouds

    // spectrogram box in 600x400 design space (measured from the mockup)
    juce::Rectangle<float> boxDesign { 326.0f, 125.0f, 265.0f, 160.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RayAudioProcessorEditor)
};
