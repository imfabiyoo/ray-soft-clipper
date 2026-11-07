#include "PluginEditor.h"

void RayKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                           float pos, float startAngle, float endAngle,
                                           juce::Slider&)
{
    const auto area = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced (1.0f);
    const auto c = area.getCentre();
    const float r = area.getWidth() * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Black glossy sphere with a green-tinted gloss, echoing the reference.
    const auto gloss = juce::Colour (0xff6fe0b0);

    // 1. Dark cast shadow under the sphere (depth).
    g.setColour (juce::Colour (0xcc000000));
    g.fillEllipse (area.expanded (1.5f).translated (0.0f, 2.5f));

    // 2. Black body — radial gradient lit from the top.
    juce::ColourGradient body (juce::Colour (0xff343a38), c.x, c.y - r * 0.55f,
                               juce::Colour (0xff000000), c.x, c.y + r * 0.95f, true);
    body.addColour (0.45, juce::Colour (0xff15110f));
    body.addColour (0.80, juce::Colour (0xff050505));
    g.setGradientFill (body);
    g.fillEllipse (area);

    // 3. Green glossy rim light on the lower edge (roundness, like the reference).
    juce::ColourGradient rimLit (juce::Colours::transparentBlack, c.x, c.y - r * 0.2f,
                                 gloss.withAlpha (0.45f), c.x, area.getBottom(), true);
    rimLit.addColour (0.82, juce::Colours::transparentBlack);
    g.setGradientFill (rimLit);
    g.fillEllipse (area);

    // 4. Hard specular highlight (shiny), upper area.
    juce::ColourGradient hot (juce::Colours::white.withAlpha (0.95f), c.x - r * 0.22f, c.y - r * 0.42f,
                              juce::Colours::transparentWhite, c.x - r * 0.05f, c.y, true);
    g.setGradientFill (hot);
    g.fillEllipse (c.x - r * 0.5f, c.y - r * 0.62f, r * 0.62f, r * 0.5f);

    // 5. Physical indicator dot — cast shadow, white body, bright top glint.
    const float dr = r * 0.64f;
    const float dotR = r * 0.14f;
    const juce::Point<float> dot (c.x + dr * std::cos (angle - juce::MathConstants<float>::halfPi),
                                  c.y + dr * std::sin (angle - juce::MathConstants<float>::halfPi));
    g.setColour (juce::Colour (0x99000000));                       // shadow
    g.fillEllipse (dot.x - dotR + 0.5f, dot.y - dotR + 1.5f, dotR * 2.0f, dotR * 2.0f);
    juce::ColourGradient dotg (juce::Colours::white, dot.x - dotR * 0.4f, dot.y - dotR * 0.5f,
                               juce::Colour (0xffb8c4bc), dot.x + dotR * 0.5f, dot.y + dotR * 0.7f, true);
    g.setGradientFill (dotg);
    g.fillEllipse (dot.x - dotR, dot.y - dotR, dotR * 2.0f, dotR * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));           // hard top glint
    g.fillEllipse (dot.x - dotR * 0.45f, dot.y - dotR * 0.6f, dotR * 0.6f, dotR * 0.5f);
}

RayAudioProcessorEditor::RayAudioProcessorEditor (RayAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    background = juce::ImageCache::getFromMemory (BinaryData::ray_ui_png, BinaryData::ray_ui_pngSize);

    setupKnob (inputKnob);
    setupKnob (thresholdKnob);
    setupKnob (posSatKnob);
    setupKnob (negSatKnob);

    inputAtt     = std::make_unique<SliderAttachment>(proc.apvts, "input",     inputKnob);
    thresholdAtt = std::make_unique<SliderAttachment>(proc.apvts, "threshold", thresholdKnob);
    posSatAtt    = std::make_unique<SliderAttachment>(proc.apvts, "posSat",    posSatKnob);
    negSatAtt    = std::make_unique<SliderAttachment>(proc.apvts, "negSat",    negSatKnob);

    // Resizable, locked to the mockup's 3:2 aspect ratio; everything scales.
    setResizable (true, true);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio (600.0 / 400.0);
    setResizeLimits (360, 240, 1200, 800);
    setSize (600, 400);
}

RayAudioProcessorEditor::~RayAudioProcessorEditor()
{
    inputKnob.setLookAndFeel (nullptr);
    thresholdKnob.setLookAndFeel (nullptr);
    posSatKnob.setLookAndFeel (nullptr);
    negSatKnob.setLookAndFeel (nullptr);
}

void RayAudioProcessorEditor::setupKnob (juce::Slider& k)
{
    k.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    k.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.setLookAndFeel (&knobLnf);
    k.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                           juce::MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (k);
}

void RayAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (background, getLocalBounds().toFloat());
    }
    else
    {
        g.fillAll (juce::Colour (0xff101010));
    }

    // Erase the mockup's baked green spheres so only our drawn knobs show — a soft
    // dark radial that fades into the panel (centres are the knob centres in 600-space).
    const float s = (float) getWidth() / 600.0f;
    const juce::Point<float> centres[] { { 418, 150 }, { 513, 150 }, { 418, 236 }, { 513, 236 } };
    for (auto c : centres)
    {
        const float cx = c.x * s, cy = c.y * s, rad = 52.0f * s;
        juce::ColourGradient er (juce::Colour (0xff16120c), cx, cy,
                                 juce::Colour (0x0016120c), cx, cy + rad, true);
        g.setGradientFill (er);
        g.fillEllipse (cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
    }
}

void RayAudioProcessorEditor::resized()
{
    // Centres/sizes measured on the 600x400 mockup; scaled to the current window
    // so the UI stays pixel-proportional at any size. Background image fills in paint().
    const float s = (float) getWidth() / 600.0f;
    auto place = [&] (juce::Slider& k, float cx, float cy, float d)
    {
        k.setBounds (juce::roundToInt ((cx - d * 0.5f) * s), juce::roundToInt ((cy - d * 0.5f) * s),
                     juce::roundToInt (d * s), juce::roundToInt (d * s));
    };

    place (inputKnob,     418.0f, 150.0f, 90.0f);
    place (thresholdKnob, 513.0f, 150.0f, 90.0f);
    place (posSatKnob,    418.0f, 236.0f, 90.0f);
    place (negSatKnob,    513.0f, 236.0f, 90.0f);
}
