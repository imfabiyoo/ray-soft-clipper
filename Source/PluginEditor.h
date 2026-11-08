#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// 3D polished-chrome knob with a white indicator; reacts to the on-screen sun.
class RayKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override;

    // live sunlight the chrome reacts to (set by the editor each frame)
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

    // --- colour grading driven by the saturation knobs -----------------------
    void regradeIfNeeded();                 // rebuild gradedBg when sat/hue moves
    float satMul() const;                   // positive-sat knob -> colour saturation
    float hueTurns() const;                 // negative-sat knob -> hue rotation (0..1)
    float inputNorm() const;                // 0..1 for sun-ray intensity
    float threshNorm() const;               // 0..1 for cloud amount

    // --- waveform scope ------------------------------------------------------
    void drawWaveform (juce::Graphics&, juce::Rectangle<float> box);
    void drawPanelDecor (juce::Graphics&, float s);   // surface grain over the left scene
    void drawLabels (juce::Graphics&);                // white SF Pro labels + / - signs
    void drawWindow (juce::Graphics&, float s);       // glass sheen + clean fade into black

    RayAudioProcessor& proc;
    juce::Image background;      // master 2800x1600 (scene + black panel)
    juce::Image gradedBg;        // hue/sat-graded working copy (component-ish res)
    juce::Image grain;           // static noise overlay for material texture

    RayKnobLookAndFeel knobLnf;
    juce::Slider inputKnob, thresholdKnob, posSatKnob, negSatKnob;
    std::unique_ptr<SliderAttachment> inputAtt, thresholdAtt, posSatAtt, negSatAtt;

    // grade cache keys
    float gradedSat = -1.0f, gradedHue = -1.0f;
    int   gradedW = 0;
    float phase = 0.0f;          // animation clock for rays/clouds

    // waveform area in 700x400 design space
    juce::Rectangle<float> boxDesign { 355.0f, 305.0f, 330.0f, 73.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RayAudioProcessorEditor)
};
