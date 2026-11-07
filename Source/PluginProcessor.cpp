#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <cstring>

namespace ids {
    constexpr auto input     = "input";
    constexpr auto threshold = "threshold";
    constexpr auto posSat    = "posSat";
    constexpr auto negSat    = "negSat";
    constexpr auto saturate  = "saturate";
}

// Small-signal saturation gain G(P), P = saturation amount (0.1..2.0).
// Cubic fit to the original (max error 6e-4). out = tanh(P*g*s) * (G(P)/P).
static inline float satGain (float P)
{
    return ((-0.051512f * P + 0.380001f) * P - 0.019802f) * P + 1.000860f;
}

// Ceiling clamp constant from the binary: bit pattern 0x3f7fffef (~0.99999797f).
static float ceilingClamp()
{
    uint32_t bits = 0x3f7fffefu;
    float f; std::memcpy (&f, &bits, sizeof (f));
    return f;
}

RayAudioProcessor::RayAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "state", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout RayAudioProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Ranges match the original Initial Clipper exactly (read via pedalboard).
    layout.add (std::make_unique<P>(juce::ParameterID { ids::input, 1 }, "Input",
        juce::NormalisableRange<float>(-9.0f, 9.0f, 0.001f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<P>(juce::ParameterID { ids::threshold, 1 }, "Threshold",
        juce::NormalisableRange<float>(-10.0f, 0.0f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<P>(juce::ParameterID { ids::posSat, 1 }, "Positive Saturation",
        juce::NormalisableRange<float>(0.1f, 2.0f, 0.01f), 0.1f));

    layout.add (std::make_unique<P>(juce::ParameterID { ids::negSat, 1 }, "Negative Saturation",
        juce::NormalisableRange<float>(0.1f, 2.0f, 0.01f), 0.1f));

    layout.add (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ids::saturate, 1 }, "Saturate", true));

    return layout;
}

void RayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate);
    // 8x oversampling, half-band FIR equiripple, maximum quality — matches the binary.
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        getTotalNumOutputChannels(), oversampleFactor,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, false);
    oversampler->initProcessing ((size_t) samplesPerBlock);
    setLatencySamples ((int) oversampler->getLatencyInSamples());
}

bool RayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void RayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    const float gain  = juce::Decibels::decibelsToGain (apvts.getRawParameterValue (ids::input)->load());
    const float T     = juce::Decibels::decibelsToGain (apvts.getRawParameterValue (ids::threshold)->load());
    const float pSat  = apvts.getRawParameterValue (ids::posSat)->load();
    const float nSat  = apvts.getRawParameterValue (ids::negSat)->load();
    const bool  satOn = apvts.getRawParameterValue (ids::saturate)->load() > 0.5f;
    const float lo = -0.999999f, hi = ceilingClamp();

    juce::dsp::AudioBlock<float> block (buffer);
    auto up = oversampler->processSamplesUp (block);   // 8x

    for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
    {
        float* d = up.getChannelPointer (ch);
        const size_t n = up.getNumSamples();
        for (size_t i = 0; i < n; ++i)
        {
            const float s = d[i];
            float x = gain * s;                        // input gain

            if (satOn)
            {
                const float P = (s < 0.0f) ? nSat : pSat;
                // tanh drive is P*s (no input gain); input gain scales the output.
                x = std::tanh (P * s) * (satGain (P) / P) * gain;
            }

            // Threshold soft-clip (exact transfer from the binary):
            if (x > T)
                x = T + (1.0f - T) * (1.0f - T / x);
            else if (x < -T)
                x = -T - (T / x + 1.0f) * (1.0f - T);

            d[i] = juce::jlimit (lo, hi, x);
        }
    }

    oversampler->processSamplesDown (block);           // back to 1x

    // Final safety hard-clip to [-1, 1] after downsampling.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            d[i] = juce::jlimit (-1.0f, 1.0f, d[i]);
    }
}

void RayAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void RayAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* RayAudioProcessor::createEditor() { return new RayAudioProcessorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RayAudioProcessor(); }
