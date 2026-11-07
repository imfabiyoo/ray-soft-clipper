#pragma once
#include <JuceHeader.h>

// ray — rebuild of Initial Clipper (Initial Audio). DSP reverse-engineered from
// the shipped VST3 (arm64) with REA/Ghidra. Signal chain, per the binary:
//   8x oversample (half-band FIR equiripple) -> per-sample saturate+softclip ->
//   downsample -> hard clip to [-1, 1].
class RayAudioProcessor : public juce::AudioProcessor
{
public:
    RayAudioProcessor();
    ~RayAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ray"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    int oversampleFactor = 3;   // 2^3 = 8x, matches the binary

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RayAudioProcessor)
};
