#include "PluginEditor.h"
#include <cmath>
#include <cstring>

// ============================ metal analog knob =============================
void RayKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                           float pos, float startAngle, float endAngle,
                                           juce::Slider&)
{
    const auto area = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const auto c = area.getCentre();
    const float r = area.getWidth() * 0.5f;
    const float ang   = startAngle + pos * (endAngle - startAngle);
    const float theta = ang - juce::MathConstants<float>::halfPi;   // 0 = up
    const juce::Point<float> dir  (std::cos (theta), std::sin (theta));
    const juce::Point<float> perp (-dir.y, dir.x);

    // cast shadow
    g.setColour (juce::Colour (0xaa000000));
    g.fillEllipse (area.expanded (1.5f).translated (0.0f, 2.0f));

    // dark machined bezel
    g.setColour (juce::Colour (0xff15171a));
    g.fillEllipse (area);

    // polished-chrome face: mirror reflection (bright sky top, dark horizon,
    // light ground) — matches the shiny liquid-chrome lettering.
    auto face = area.reduced (r * 0.12f);
    juce::ColourGradient fg (juce::Colours::white, face.getCentreX(), face.getY(),
                             juce::Colour (0xffececea), face.getCentreX(), face.getBottom(), false);
    fg.addColour (0.30, juce::Colour (0xffc9c9c6));    // neutral silver (no blue)
    fg.addColour (0.48, juce::Colour (0xff3f3f3d));
    fg.addColour (0.54, juce::Colour (0xff222221));   // hard horizon = chrome
    fg.addColour (0.70, juce::Colour (0xff97968f));
    g.setGradientFill (fg);
    g.fillEllipse (face);

    // faint machined rings
    g.setColour (juce::Colour (0x18000000));
    for (float rr = 0.86f; rr > 0.18f; rr -= 0.18f)
        g.drawEllipse (face.reduced (r * (1.0f - rr)), 0.7f);

    // rotating specular streak across the metal (reads as turning)
    {
        const float fr = face.getWidth() * 0.5f;
        juce::Path streak;
        streak.startNewSubPath (c - perp * (fr * 0.90f) + dir * (fr * 0.10f));
        streak.lineTo          (c + perp * (fr * 0.90f) + dir * (fr * 0.10f));
        g.setColour (juce::Colour (0x66ffffff));
        g.strokePath (streak, juce::PathStrokeType (fr * 0.18f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xd0ffffff));   // hard bright core
        g.strokePath (streak, juce::PathStrokeType (fr * 0.05f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // dark inner cap
    auto cap = face.reduced (face.getWidth() * 0.30f);
    juce::ColourGradient cg (juce::Colour (0xff3b3e41), cap.getCentreX(), cap.getY(),
                             juce::Colour (0xff0e0f10), cap.getCentreX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillEllipse (cap);
    g.setColour (juce::Colour (0x30ffffff));
    g.drawEllipse (cap, 1.0f);

    // pointer: engraved groove + bright metal indicator to the bezel
    const juce::Point<float> a = c + dir * (face.getWidth() * 0.30f);
    const juce::Point<float> b = c + dir * (r * 0.92f);
    g.setColour (juce::Colour (0xff0a0a0a));
    g.drawLine (juce::Line<float> (a, b), r * 0.16f);
    g.setColour (juce::Colour (0xfff3f6f8));
    g.drawLine (juce::Line<float> (a, b), r * 0.08f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (b.x - r * 0.07f, b.y - r * 0.07f, r * 0.14f, r * 0.14f);
}

// ============================== editor =====================================
RayAudioProcessorEditor::RayAudioProcessorEditor (RayAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    background = juce::ImageCache::getFromMemory (BinaryData::ray_ui_png, BinaryData::ray_ui_pngSize);

    // static fine grain for a tactile surface (built once)
    grain = juce::Image (juce::Image::ARGB, 180, 180, true);
    {
        juce::Random rng (0x9e3779b9);
        juce::Image::BitmapData bd (grain, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < grain.getHeight(); ++y)
            for (int x = 0; x < grain.getWidth(); ++x)
            {
                const int v = rng.nextInt (256);
                bd.setPixelColour (x, y, juce::Colour::fromRGBA ((juce::uint8) v, (juce::uint8) v,
                                                                 (juce::uint8) v, (juce::uint8) (rng.nextInt (26))));
            }
    }

    setupKnob (inputKnob);
    setupKnob (thresholdKnob);
    setupKnob (posSatKnob);
    setupKnob (negSatKnob);

    inputAtt     = std::make_unique<SliderAttachment>(proc.apvts, "input",     inputKnob);
    thresholdAtt = std::make_unique<SliderAttachment>(proc.apvts, "threshold", thresholdKnob);
    posSatAtt    = std::make_unique<SliderAttachment>(proc.apvts, "posSat",    posSatKnob);
    negSatAtt    = std::make_unique<SliderAttachment>(proc.apvts, "negSat",    negSatKnob);

    setResizable (true, true);
    if (auto* cst = getConstrainer())
        cst->setFixedAspectRatio (600.0 / 400.0);
    setResizeLimits (360, 240, 1200, 800);
    setSize (600, 400);

    startTimerHz (30);
}

RayAudioProcessorEditor::~RayAudioProcessorEditor()
{
    stopTimer();
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

// ----- parameter-driven visual mappings -----
float RayAudioProcessorEditor::satMul() const
{
    return juce::jlimit (0.1f, 2.0f, proc.apvts.getRawParameterValue ("posSat")->load());
}
float RayAudioProcessorEditor::hueTurns() const
{
    return juce::jlimit (0.0f, 1.0f, (proc.apvts.getRawParameterValue ("negSat")->load() - 0.1f) / 1.9f);
}
float RayAudioProcessorEditor::inputNorm() const
{
    return juce::jlimit (0.0f, 1.0f, (proc.apvts.getRawParameterValue ("input")->load() + 9.0f) / 18.0f);
}
float RayAudioProcessorEditor::threshNorm() const
{
    // threshold drives total sun: left (-10 dB) = none, right (0 dB) = full blast
    return juce::jlimit (0.0f, 1.0f, (proc.apvts.getRawParameterValue ("threshold")->load() + 10.0f) / 10.0f);
}

// ----- hue/sat grade of the photo background, rebuilt only when it moves -----
void RayAudioProcessorEditor::regradeIfNeeded()
{
    if (! background.isValid()) return;
    const int gw = juce::jlimit (300, 1200, getWidth());
    const float s = satMul(), hh = hueTurns();
    if (gradedBg.isValid() && gw == gradedW
        && std::abs (s - gradedSat) < 0.01f && std::abs (hh - gradedHue) < 0.004f)
        return;

    const int gh = juce::roundToInt ((float) gw * 800.0f / 1200.0f);
    juce::Image img (juce::Image::RGB, gw, gh, false);
    {
        juce::Graphics g (img);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (background, img.getBounds().toFloat());
    }
    {
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::readWrite);
        for (int yy = 0; yy < gh; ++yy)
            for (int xx = 0; xx < gw; ++xx)
            {
                auto col = bd.getPixelColour (xx, yy).withRotatedHue (hh).withMultipliedSaturation (s);
                bd.setPixelColour (xx, yy, col);
            }
    }
    gradedBg = img; gradedW = gw; gradedSat = s; gradedHue = hh;
}

// ----- waveform scope: time-domain envelope drawn with a colour gradient -----
void RayAudioProcessorEditor::drawWaveform (juce::Graphics& g, juce::Rectangle<float> box)
{
    const int W = juce::jmax (2, (int) box.getWidth());
    const int N = 1536;                                      // samples in view (< ring)
    const int wr = proc.scopeWrite.load (std::memory_order_acquire);
    const float midY = box.getCentreY();
    const float amp  = box.getHeight() * 0.46f;

    std::vector<float> yTop ((size_t) W), yBot ((size_t) W);
    for (int x = 0; x < W; ++x)
    {
        const int i0 = (int) ((float) x       / (float) W * (float) N);
        const int i1 = (int) ((float) (x + 1) / (float) W * (float) N);
        float mn = 1.0e9f, mx = -1.0e9f;
        for (int i = i0; i <= i1; ++i)
        {
            const float v = proc.scopeBuf[(size_t) ((wr - N + i) & RayAudioProcessor::scopeMask)];
            mn = juce::jmin (mn, v); mx = juce::jmax (mx, v);
        }
        yTop[(size_t) x] = midY - mx * amp;
        yBot[(size_t) x] = midY - mn * amp;
    }

    // green->yellow gradient across the waveform; the hue knob slides it.
    const float sat = juce::jlimit (0.4f, 1.0f,
        (proc.apvts.getRawParameterValue ("posSat")->load() - 0.1f) / 1.4f);
    const float hueL = 0.33f + hueTurns();   // green
    const float hueR = 0.17f + hueTurns();   // yellow
    auto hsv = [sat] (float h, float v, float a) {
        h -= std::floor (h);
        return juce::Colour::fromHSV (h, sat, v, a);
    };

    juce::Path fill;
    fill.startNewSubPath (box.getX(), yTop[0]);
    for (int x = 1; x < W; ++x) fill.lineTo (box.getX() + (float) x, yTop[(size_t) x]);
    for (int x = W - 1; x >= 0; --x) fill.lineTo (box.getX() + (float) x, yBot[(size_t) x]);
    fill.closeSubPath();

    juce::ColourGradient grad (hsv (hueL, 0.95f, 0.55f), box.getX(), midY,
                               hsv (hueR, 0.95f, 0.55f), box.getRight(), midY, false);
    g.setGradientFill (grad);
    g.fillPath (fill);

    // bright gradient edge on top of the envelope
    juce::Path top;
    top.startNewSubPath (box.getX(), yTop[0]);
    for (int x = 1; x < W; ++x) top.lineTo (box.getX() + (float) x, yTop[(size_t) x]);
    juce::ColourGradient edge (hsv (hueL, 1.0f, 1.0f), box.getX(), midY,
                               hsv (hueR, 1.0f, 1.0f), box.getRight(), midY, false);
    g.setGradientFill (edge);
    g.strokePath (top, juce::PathStrokeType (1.4f));
}

// grain + corner screws + faceplate bevel: makes the black space feel like a
// real panel you could touch.
void RayAudioProcessorEditor::drawPanelDecor (juce::Graphics& g, float s)
{
    auto b = getLocalBounds().toFloat();

    // fine surface grain
    if (grain.isValid())
    {
        g.setTiledImageFill (grain, 0, 0, 0.55f);
        g.fillRect (b);
    }

    // faceplate bevel: top highlight, bottom shade
    g.setColour (juce::Colour (0x22ffffff));
    g.fillRect (b.withHeight (1.5f * s));
    g.setColour (juce::Colour (0x33000000));
    g.fillRect (b.removeFromBottom (2.0f * s));

    // chrome screws bolting the plate down
    auto screw = [&] (float cx, float cy)
    {
        const float rr = 6.0f * s;
        juce::Rectangle<float> sc (cx * s - rr, cy * s - rr, rr * 2.0f, rr * 2.0f);
        g.setColour (juce::Colour (0xaa000000));
        g.fillEllipse (sc.translated (0.0f, 1.0f * s));
        juce::ColourGradient mg (juce::Colour (0xffd8d8d6), sc.getX(), sc.getY(),
                                 juce::Colour (0xff4a4a48), sc.getRight(), sc.getBottom(), false);
        g.setGradientFill (mg);
        g.fillEllipse (sc);
        g.setColour (juce::Colour (0xff202020));
        const float a = juce::degreesToRadians (cx + cy);   // varied slot angle
        const juce::Point<float> d (std::cos (a), std::sin (a));
        const juce::Point<float> c = sc.getCentre();
        g.drawLine (c.x - d.x * rr * 0.7f, c.y - d.y * rr * 0.7f,
                    c.x + d.x * rr * 0.7f, c.y + d.y * rr * 0.7f, 1.4f * s);
        g.setColour (juce::Colour (0x40ffffff));
        g.drawEllipse (sc.reduced (0.5f), 0.8f);
    };
    screw (16.0f, 16.0f);  screw (584.0f, 16.0f);
    screw (16.0f, 384.0f); screw (584.0f, 384.0f);
}

// ----- sunlight: broad soft parallel shafts + bloom, not a starburst -----
static void drawSky (juce::Graphics& g, juce::Rectangle<float> b, float /*s*/,
                     float in, float th, float hueT, float satM, float phase)
{
    // threshold is the master sun dimmer: amt 0 (knob left, -10 dB) = nothing,
    // amt 1 (knob right, 0 dB) = full blast. Input adds ray detail/strength.
    const float amt = juce::jlimit (0.0f, 1.0f, th);
    if (amt <= 0.001f) return;                             // left = no sun at all
    const juce::Colour sun = juce::Colour (0xfff6e2a0).withRotatedHue (hueT)
                                                      .withMultipliedSaturation (satM);
    const juce::Point<float> src (b.getWidth() * 0.10f, -b.getHeight() * 0.18f);

    // 1) diffuse corner bloom, scaled by the master amount
    juce::ColourGradient bloom (sun.withAlpha ((0.06f + 0.12f * in) * amt), src.x, src.y,
                                sun.withAlpha (0.0f), b.getWidth() * 1.0f, b.getHeight() * 1.1f, true);
    bloom.addColour (0.4, sun.withAlpha ((0.03f + 0.06f * in) * amt));
    g.setGradientFill (bloom);
    g.fillRect (b);

    // 2) wide, diffused, near-parallel shafts spread across the whole face; each
    //    breathes slowly as if clouds drift across it.
    const float baseAngle = juce::degreesToRadians (60.0f);     // down-right
    const float L = b.getWidth() * 1.9f;
    const float wUnit = b.getWidth() * 0.13f;
    const int   nShafts = 5;
    for (int k = 0; k < nShafts; ++k)
    {
        const float f = (float) k / (float) (nShafts - 1);        // 0..1
        const float offset = (f - 0.32f) * b.getWidth() * 1.25f;  // span the plugin
        const float a = baseAngle + 0.11f * std::sin (phase * 0.5f + (float) k * 1.3f);
        const juce::Point<float> dir (std::cos (a), std::sin (a));
        const juce::Point<float> pp (-dir.y, dir.x);
        const juce::Point<float> c = src + pp * offset + dir * (b.getHeight() * 0.1f);
        const float breathe = 0.55f + 0.45f * std::sin (phase * (0.4f + 0.05f * (float) k) + (float) k * 2.1f);
        const float widthPx = wUnit * (0.8f + 0.6f * std::sin ((float) k));
        auto shaft = [&] (float halfW, float alpha)
        {
            const juce::Point<float> e0 = c - pp * halfW, e1 = c + pp * halfW;
            juce::Path p;
            p.startNewSubPath (e0 - dir * L); p.lineTo (e1 - dir * L);
            p.lineTo (e1 + dir * L);          p.lineTo (e0 + dir * L);
            p.closeSubPath();
            juce::ColourGradient cg (sun.withAlpha (0.0f), e0.x, e0.y,
                                     sun.withAlpha (0.0f), e1.x, e1.y, false);
            cg.addColour (0.5, sun.withAlpha (alpha));
            g.setGradientFill (cg);
            g.fillPath (p);
        };
        shaft (std::abs (widthPx) * 2.2f, (0.03f + 0.05f * in) * amt * breathe);  // soft bloom
        shaft (std::abs (widthPx) * 0.7f, (0.07f + 0.14f * in) * amt * breathe);  // hard core
    }
}

void RayAudioProcessorEditor::paint (juce::Graphics& g)
{
    regradeIfNeeded();

    if (gradedBg.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (gradedBg, getLocalBounds().toFloat());
    }
    else
    {
        g.fillAll (juce::Colour (0xff101010));
    }

    const float s = (float) getWidth() / 600.0f;

    // recessed screen: inset shadow well, waveform, glass sheen (no hard outline)
    const juce::Rectangle<float> box (boxDesign.getX() * s, boxDesign.getY() * s,
                                      boxDesign.getWidth() * s, boxDesign.getHeight() * s);
    {
        juce::ColourGradient well (juce::Colour (0xff000000), box.getCentreX(), box.getY(),
                                   juce::Colour (0xff141414), box.getCentreX(), box.getBottom(), false);
        g.setGradientFill (well);
        g.fillRoundedRectangle (box, 4.0f * s);
    }
    drawWaveform (g, box);
    // top inner shadow + bottom glass sheen = recessed glass feel
    juce::ColourGradient innerTop (juce::Colour (0x80000000), box.getCentreX(), box.getY(),
                                   juce::Colour (0x00000000), box.getCentreX(), box.getY() + 14.0f * s, false);
    g.setGradientFill (innerTop);
    g.fillRect (box.withHeight (16.0f * s));
    g.setColour (juce::Colour (0x18ffffff));
    g.drawRoundedRectangle (box.reduced (0.5f), 4.0f * s, 1.0f);   // soft bevel, not white

    // sun + clouds over the whole scene
    drawSky (g, getLocalBounds().toFloat(), s, inputNorm(), threshNorm(),
             hueTurns(), satMul(), phase);

    drawPanelDecor (g, s);
}

void RayAudioProcessorEditor::timerCallback()
{
    phase += 0.05f;
    repaint();
}

void RayAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / 600.0f;
    auto place = [&] (juce::Slider& k, float cx, float cy, float d)
    {
        k.setBounds (juce::roundToInt ((cx - d * 0.5f) * s), juce::roundToInt ((cy - d * 0.5f) * s),
                     juce::roundToInt (d * s), juce::roundToInt (d * s));
    };
    // centres measured from the reference mockup (600x400 space)
    place (inputKnob,     574.0f,  58.0f, 34.0f);
    place (thresholdKnob, 574.0f,  95.0f, 34.0f);
    place (posSatKnob,    404.0f, 353.0f, 32.0f);
    place (negSatKnob,    508.0f, 352.0f, 32.0f);
}
