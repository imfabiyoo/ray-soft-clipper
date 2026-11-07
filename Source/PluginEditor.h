#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Draws the glossy green knob from the mockup, with a rotating indicator dot.
class RayKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override;
};

class RayAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit RayAudioProcessorEditor (RayAudioProcessor&);
    ~RayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    RayAudioProcessor& proc;
    juce::Image background;
    RayKnobLookAndFeel knobLnf;

    juce::Slider inputKnob, thresholdKnob, posSatKnob, negSatKnob;
    std::unique_ptr<SliderAttachment> inputAtt, thresholdAtt, posSatAtt, negSatAtt;

    void setupKnob (juce::Slider&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RayAudioProcessorEditor)
};
