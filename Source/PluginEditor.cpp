#include "PluginEditor.h"
#include <cmath>
#include <cstring>

void RayKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                           float pos, float startAngle, float endAngle,
                                           juce::Slider&)
{

    const auto area = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced ((float) w * 0.09f);
    const auto c = area.getCentre();
    const float r = area.getWidth() * 0.5f;
    const float theta = startAngle + pos * (endAngle - startAngle) - juce::MathConstants<float>::halfPi;
    const juce::Point<float> dir (std::cos (theta), std::sin (theta));

    g.setColour (juce::Colour (0x99000000));
    g.fillEllipse (area.expanded (1.0f).translated (0.0f, 2.0f));

    g.setColour (juce::Colour (0xff0b0c0e));
    g.fillEllipse (area);

    auto face = area.reduced (r * 0.12f);
    juce::ColourGradient fg (juce::Colours::white, face.getCentreX(), face.getY(),
                             juce::Colour (0xffeef1f4), face.getCentreX(), face.getBottom(), false);
    fg.addColour (0.30, juce::Colour (0xffc2c7cd));
    fg.addColour (0.48, juce::Colour (0xff3d4045));
    fg.addColour (0.54, juce::Colour (0xff24262a));
    fg.addColour (0.70, juce::Colour (0xff9aa0a7));
    g.setGradientFill (fg);
    g.fillEllipse (face);

    {
        juce::ColourGradient hot (juce::Colours::white.withAlpha (0.95f),
                                  c.x - r * 0.34f, c.y - r * 0.44f,
                                  juce::Colours::transparentWhite,
                                  c.x - r * 0.02f, c.y + r * 0.10f, true);
        g.setGradientFill (hot);
        g.fillEllipse (face.reduced (r * 0.06f));
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawEllipse (face.reduced (0.8f), 1.2f);
    }

    if (sunAmt > 0.02f)
    {
        g.setColour (sunCol.withAlpha (0.5f * sunAmt));
        g.fillEllipse (face);
        g.setColour (sunCol.brighter (0.5f).withAlpha (0.85f * sunAmt));
        g.drawEllipse (face.reduced (1.0f), 2.4f);
        const float ga = sunPhase * 0.8f;
        const juce::Point<float> gp = c + juce::Point<float> (std::cos (ga), std::sin (ga)) * (r * 0.46f);
        const float gr = r * 0.72f;
        juce::ColourGradient glint (juce::Colours::white.withAlpha (sunAmt), gp.x, gp.y,
                                    sunCol.withAlpha (0.0f), gp.x + gr, gp.y, true);
        glint.addColour (0.28, sunCol.brighter (0.9f).withAlpha (0.8f * sunAmt));
        g.setGradientFill (glint);
        g.fillEllipse (gp.x - gr, gp.y - gr, gr * 2.0f, gr * 2.0f);
    }

    auto cap = face.reduced (face.getWidth() * 0.32f);
    juce::ColourGradient cg (juce::Colour (0xff3a3d42), cap.getCentreX(), cap.getY(),
                             juce::Colour (0xff0d0e10), cap.getCentreX(), cap.getBottom(), false);
    g.setGradientFill (cg);
    g.fillEllipse (cap);
    g.setColour (juce::Colour (0x30ffffff));
    g.drawEllipse (cap, 1.0f);

    const juce::Point<float> a = c + dir * (cap.getWidth() * 0.45f);
    const juce::Point<float> b = c + dir * (r * 0.9f);
    g.setColour (juce::Colour (0xff0a0b0d));
    g.drawLine (juce::Line<float> (a, b), r * 0.18f);
    g.setColour (juce::Colours::white);
    g.drawLine (juce::Line<float> (a, b), r * 0.09f);
    g.fillEllipse (b.x - r * 0.08f, b.y - r * 0.08f, r * 0.16f, r * 0.16f);
}

RayAudioProcessorEditor::RayAudioProcessorEditor (RayAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    background = juce::ImageCache::getFromMemory (BinaryData::ray_ui_png, BinaryData::ray_ui_pngSize);

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
        cst->setFixedAspectRatio (700.0 / 400.0);
    setResizeLimits (420, 240, 1400, 800);
    setSize (700, 400);

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

    return juce::jlimit (-1.0f, 1.0f, proc.apvts.getRawParameterValue ("input")->load() / 9.0f);
}
float RayAudioProcessorEditor::threshNorm() const
{

    return juce::jlimit (0.0f, 1.0f, (proc.apvts.getRawParameterValue ("threshold")->load() + 10.0f) / 10.0f);
}

void RayAudioProcessorEditor::regradeIfNeeded()
{
    if (! background.isValid() || getWidth() <= 0 || getHeight() <= 0) return;
    const int gw = juce::jlimit (300, 1400, getWidth());
    const float s = satMul(), hh = hueTurns();
    if (gradedBg.isValid() && gw == gradedW
        && std::abs (s - gradedSat) < 0.01f && std::abs (hh - gradedHue) < 0.004f)
        return;

    const int gh = juce::roundToInt ((float) gw * (float) getHeight() / (float) getWidth());
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

void RayAudioProcessorEditor::drawWaveform (juce::Graphics& g, juce::Rectangle<float> box)
{
    const int W = juce::jmax (2, (int) box.getWidth());
    const int N = 1536;
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

    juce::Path fill, top, bot;
    fill.startNewSubPath (box.getX(), yTop[0]);
    top .startNewSubPath (box.getX(), yTop[0]);
    bot .startNewSubPath (box.getX(), yBot[0]);
    for (int x = 1; x < W; ++x) { fill.lineTo (box.getX() + (float) x, yTop[(size_t) x]);
                                  top .lineTo (box.getX() + (float) x, yTop[(size_t) x]); }
    for (int x = W - 1; x >= 0; --x) fill.lineTo (box.getX() + (float) x, yBot[(size_t) x]);
    fill.closeSubPath();
    for (int x = 1; x < W; ++x) bot.lineTo (box.getX() + (float) x, yBot[(size_t) x]);

    using PS = juce::PathStrokeType;
    const float hw = box.getHeight() * 0.12f;
    const float mw = box.getHeight() * 0.05f;
    const float cw = box.getHeight() * 0.022f;

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillPath (fill);
    g.setColour (juce::Colours::white.withAlpha (0.11f));
    g.strokePath (top, PS (hw, PS::curved, PS::rounded));  g.strokePath (bot, PS (hw, PS::curved, PS::rounded));
    g.setColour (juce::Colours::white.withAlpha (0.26f));
    g.strokePath (top, PS (mw, PS::curved, PS::rounded));  g.strokePath (bot, PS (mw, PS::curved, PS::rounded));
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.strokePath (top, PS (cw, PS::curved, PS::rounded));  g.strokePath (bot, PS (cw, PS::curved, PS::rounded));
}

void RayAudioProcessorEditor::drawPanelDecor (juce::Graphics& g, float s)
{
    auto b = getLocalBounds().toFloat();

    if (grain.isValid())
    {
        g.setTiledImageFill (grain, 0, 0, 0.5f);
        g.fillRect (0.0f, 0.0f, 345.0f * s, b.getHeight());
    }
}

static void drawSky (juce::Graphics& g, juce::Rectangle<float> b, float ,
                     float in, float th, float hueT, float satM, float phase)
{

    const float sky  = in;
    const float line = std::abs (sky);
    if (line <= 0.001f) return;
    const float bloomF = juce::jlimit (0.0f, 1.0f, th);
    const bool  shadow = sky < 0.0f;
    const juce::Colour warm = juce::Colour (0xfff6e2a0).withRotatedHue (hueT)
                                                       .withMultipliedSaturation (satM);
    const juce::Colour col  = shadow ? juce::Colour (0xff04060a) : warm;
    const juce::Point<float> src (b.getWidth() * 0.10f, -b.getHeight() * 0.18f);

    const float bloomA = (shadow ? 0.14f : 0.11f) * line * bloomF;
    if (bloomA > 0.002f)
    {
        juce::ColourGradient bloom (col.withAlpha (bloomA), src.x, src.y,
                                    col.withAlpha (0.0f), b.getWidth() * 1.0f, b.getHeight() * 1.1f, true);
        bloom.addColour (0.4, col.withAlpha (bloomA * 0.5f));
        g.setGradientFill (bloom);
        g.fillRect (b);
    }

    if (! shadow)
    {
        const float sunA = 0.55f * line;
        juce::ColourGradient disc (warm.brighter (0.7f).withAlpha (sunA), src.x, src.y,
                                   warm.withAlpha (0.0f), src.x, src.y + b.getHeight() * 0.55f, true);
        disc.addColour (0.12, warm.brighter (0.4f).withAlpha (sunA * 0.75f));
        disc.addColour (0.45, warm.withAlpha (sunA * 0.22f));
        g.setGradientFill (disc);
        g.fillRect (b);
    }

    auto hash = [] (int i) { float x = std::sin ((float) i * 12.9898f) * 43758.5453f; return x - std::floor (x); };

    const float baseAngle = juce::degreesToRadians (60.0f);
    const float L = b.getWidth() * 1.9f;
    const float wUnit = b.getWidth() * 0.20f;
    const int   nShafts = 7;
    for (int k = 0; k < nShafts; ++k)
    {
        const float f = (float) k / (float) (nShafts - 1);
        const float offset = (f - 0.30f) * b.getWidth() * 1.3f;
        const float a = baseAngle + 0.03f * std::sin (phase * 0.2f + (float) k * 1.3f);
        const juce::Point<float> dir (std::cos (a), std::sin (a));
        const juce::Point<float> pp (-dir.y, dir.x);
        const juce::Point<float> c = src + pp * offset + dir * (b.getHeight() * 0.1f);
        const float breathe = 0.9f + 0.1f * std::sin (phase * 0.2f + (float) k * 2.1f);

        const float wSeed  = hash (k * 3 + 1);
        const float wPulse = 0.5f + 0.5f * std::sin (phase * 0.35f + wSeed * 6.2831f);
        const float widthPx = wUnit * (0.22f + 1.7f * wSeed * wSeed) * (0.55f + 0.9f * wPulse);
        auto shaft = [&] (float halfW, float alpha, bool crisp)
        {
            const juce::Point<float> e0 = c - pp * halfW, e1 = c + pp * halfW;
            juce::Path p;
            p.startNewSubPath (e0 - dir * L); p.lineTo (e1 - dir * L);
            p.lineTo (e1 + dir * L);          p.lineTo (e0 + dir * L);
            p.closeSubPath();
            juce::ColourGradient cg (col.withAlpha (0.0f), e0.x, e0.y,
                                     col.withAlpha (0.0f), e1.x, e1.y, false);
            if (crisp) { cg.addColour (0.34, col.withAlpha (alpha)); cg.addColour (0.66, col.withAlpha (alpha)); }
            cg.addColour (0.5, col.withAlpha (alpha));
            g.setGradientFill (cg);
            g.fillPath (p);
        };
        shaft (widthPx * 2.4f, (shadow ? 0.10f : 0.08f) * line * bloomF * breathe, false);
        shaft (widthPx * 0.62f, (shadow ? 0.48f : 0.42f) * line * breathe,          true);
    }

    if (! shadow)
    {
        const juce::Point<float> dir (std::cos (baseAngle), std::sin (baseAngle));
        for (int i = 0; i < 60; ++i)
        {
            const float sx = hash (i * 2 + 1), sy = hash (i * 3 + 7), sp = 0.4f + hash (i * 5 + 2);
            const float t  = std::fmod (phase * 0.03f * sp + sx + sy, 1.2f) - 0.1f;
            const juce::Point<float> p = juce::Point<float> (b.getWidth() * sx, b.getHeight() * sy)
                                         + dir * (t * b.getWidth() * 0.5f);
            const float tw = 0.3f + 0.7f * std::abs (std::sin (phase * 1.6f + (float) i));
            const float a  = juce::jlimit (0.0f, 1.0f, 0.55f * line * tw);
            const float r  = 0.7f + 1.5f * hash (i * 11 + 4);
            g.setColour (warm.brighter (0.85f).withAlpha (a));
            g.fillEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f);
        }
    }

    for (int i = 0; i < 7; ++i)
    {
        const float lx  = hash (i * 5 + 2), ly = hash (i * 7 + 1);
        const float cx  = b.getWidth()  * lx + 7.0f * std::sin (phase * 0.15f + (float) i);
        const float cy  = b.getHeight() * ly + 5.0f * std::cos (phase * 0.13f + (float) i * 1.3f);
        const float rad = b.getWidth() * (0.07f + 0.06f * hash (i * 9 + 3));
        const float da  = 0.10f * line;
        juce::ColourGradient leaf (juce::Colour (0xff000000).withAlpha (da), cx, cy,
                                   juce::Colour (0x00000000), cx + rad, cy, true);
        g.setGradientFill (leaf);
        g.fillEllipse (cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
    }
}

void RayAudioProcessorEditor::drawWindow (juce::Graphics& g, float s)
{
    const float H = (float) getHeight();
    const float sceneR = 340.0f * s;
    const juce::Rectangle<float> win (0.0f, 0.0f, sceneR, H);

    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (win.toNearestInt());
        const juce::Point<float> d (0.5f, 0.86f);
        const juce::Point<float> p (-d.y, d.x);
        const float L = H * 1.8f;
        auto streak = [&] (float off, float halfW, float alpha)
        {
            const juce::Point<float> ctr = win.getCentre() + p * (off * s);
            const juce::Point<float> e0 = ctr - p * (halfW * s), e1 = ctr + p * (halfW * s);
            juce::Path q;
            q.startNewSubPath (e0 - d * L); q.lineTo (e1 - d * L);
            q.lineTo (e1 + d * L);          q.lineTo (e0 + d * L); q.closeSubPath();
            g.setColour (juce::Colours::white.withAlpha (alpha));
            g.fillPath (q);
        };
        streak (-90.0f, 34.0f, 0.09f);
        streak (-48.0f, 14.0f, 0.15f);
        streak ( 80.0f, 22.0f, 0.06f);
    }

    {
        const float f0 = 90.0f * s, f1 = 352.0f * s;
        juce::ColourGradient fade (juce::Colour (0x00000000), f0, 0.0f,
                                   juce::Colour (0xff000000), f1, 0.0f, false);
        fade.addColour (0.35, juce::Colour (0x22000000));
        fade.addColour (0.62, juce::Colour (0x77000000));
        fade.addColour (0.82, juce::Colour (0xcc000000));
        g.setGradientFill (fade);
        g.fillRect (f0, 0.0f, f1 - f0 + 1.0f, H);
    }

    const float vx = 46.0f * s, vy = 54.0f * s;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff000000), 0.0f, 0.0f,
                                             juce::Colour (0x00000000), vx, 0.0f, false));
    g.fillRect (0.0f, 0.0f, vx, H);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff000000), 0.0f, 0.0f,
                                             juce::Colour (0x00000000), 0.0f, vy, false));
    g.fillRect (0.0f, 0.0f, sceneR, vy);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff000000), 0.0f, H,
                                             juce::Colour (0x00000000), 0.0f, H - vy, false));
    g.fillRect (0.0f, H - vy, sceneR, vy);
}

static void drawGlassFX (juce::Graphics& g, juce::Rectangle<float> b,
                         float sun, float phase, float hueT, float satM)
{
    if (sun <= 0.02f) return;
    const juce::Point<float> src (b.getWidth() * 0.13f, b.getHeight() * 0.10f);
    const juce::Point<float> axis = b.getCentre() - src;
    const juce::Colour warm = juce::Colour (0xfff4e3b0).withRotatedHue (hueT).withMultipliedSaturation (satM);

    {
        const float R = b.getWidth() * 0.34f * (0.9f + 0.1f * std::sin (phase * 0.6f));
        juce::ColourGradient cg (juce::Colours::white.withAlpha (0.75f * sun), src.x, src.y,
                                 warm.withAlpha (0.0f), src.x + R, src.y, true);
        cg.addColour (0.22, warm.withAlpha (0.45f * sun));
        g.setGradientFill (cg);
        g.fillEllipse (src.x - R, src.y - R, R * 2.0f, R * 2.0f);
    }

    const juce::Colour ghost[] = { warm, juce::Colour (0xff66ffcc), juce::Colour (0xffff66aa),
                                   juce::Colour (0xff88aaff), warm, juce::Colour (0xffffc866) };
    const float ts[] = { 1.3f, 1.65f, 2.0f, 2.45f, 2.9f, 3.3f };
    const float sz[] = { 0.055f, 0.022f, 0.075f, 0.03f, 0.05f, 0.018f };
    for (int i = 0; i < 6; ++i)
    {
        const juce::Point<float> pos = src + axis * ts[i];
        const float rr = b.getWidth() * sz[i];
        const juce::Colour cc = ghost[i].withAlpha ((0.09f + 0.05f * std::sin (phase * 0.7f + (float) i)) * sun);
        juce::ColourGradient gg (cc, pos.x, pos.y, cc.withAlpha (0.0f), pos.x + rr, pos.y, true);
        gg.addColour (0.68, cc);
        g.setGradientFill (gg);
        g.fillEllipse (pos.x - rr, pos.y - rr, rr * 2.0f, rr * 2.0f);
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

    const float s = (float) getWidth() / 700.0f;

    drawWindow (g, s);

    g.setColour (juce::Colour (0xff000000));
    g.fillRect (345.0f * s, 0.0f, (float) getWidth() - 345.0f * s, (float) getHeight());

    const juce::Rectangle<float> box (boxDesign.getX() * s, boxDesign.getY() * s,
                                      boxDesign.getWidth() * s, boxDesign.getHeight() * s);
    drawWaveform (g, box);

    drawLabels (g);

    drawSky (g, getLocalBounds().toFloat(), s, inputNorm(), threshNorm(),
             hueTurns(), satMul(), phase);

    drawGlassFX (g, getLocalBounds().toFloat(), juce::jmax (0.0f, inputNorm()),
                 phase, hueTurns(), satMul());

    drawPanelDecor (g, s);
}

void RayAudioProcessorEditor::drawLabels (juce::Graphics& g)
{
    const float S = (float) getWidth() / 700.0f;

    auto label = [&] (const juce::String& text, float dx, float dy, float dw, float dh,
                      float htDesign, juce::Justification just)
    {
        const juce::Rectangle<float> box (dx * S, dy * S, dw * S, dh * S);
        juce::Font font (juce::FontOptions().withName ("SF Pro Display").withHeight (htDesign * S));
        font.setBold (true);
        g.setFont (font);

        g.setColour (juce::Colour (0x88000000));
        g.drawText (text, box.translated (1.0f * S, 1.5f * S), just, false);

        g.setColour (juce::Colour (0xffffffff));
        g.drawText (text, box, just, false);
    };

    using J = juce::Justification;
    label ("input",      365.0f,  36.0f, 300.0f, 40.0f, 35.0f, J::centred);
    label ("threshold",  365.0f, 123.0f, 300.0f, 40.0f, 35.0f, J::centred);
    label ("saturation", 365.0f, 211.0f, 300.0f, 38.0f, 33.0f, J::centred);

    auto sign = [&] (float cxD, float cyD, float szD, bool plus)
    {
        const float cx = cxD * S, cy = cyD * S, sz = szD * S, th = szD * 0.26f * S;
        g.setColour (juce::Colour (0xffffffff));
        g.fillRoundedRectangle (cx - sz * 0.5f, cy - th * 0.5f, sz, th, th * 0.4f);
        if (plus) g.fillRoundedRectangle (cx - th * 0.5f, cy - sz * 0.5f, th, sz, th * 0.4f);
    };
    sign (446.0f, 272.0f, 15.0f, true);
    sign (586.0f, 272.0f, 15.0f, false);
}

void RayAudioProcessorEditor::timerCallback()
{
    phase += 0.05f;

    knobLnf.sunAmt   = juce::jmax (0.0f, inputNorm());
    knobLnf.sunPhase = phase;
    knobLnf.sunCol   = juce::Colour (0xfff4e3b0).withRotatedHue (hueTurns())
                                                .withMultipliedSaturation (satMul());
    repaint();
    inputKnob.repaint();  thresholdKnob.repaint();
    posSatKnob.repaint(); negSatKnob.repaint();
}

void RayAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / 700.0f;
    auto place = [&] (juce::Slider& k, float cx, float cy, float d)
    {
        k.setBounds (juce::roundToInt ((cx - d * 0.5f) * s), juce::roundToInt ((cy - d * 0.5f) * s),
                     juce::roundToInt (d * s), juce::roundToInt (d * s));
    };

    place (inputKnob,     515.0f,  96.0f, 48.0f);
    place (thresholdKnob, 515.0f, 184.0f, 48.0f);
    place (posSatKnob,    479.0f, 272.0f, 42.0f);
    place (negSatKnob,    553.0f, 272.0f, 42.0f);
}
