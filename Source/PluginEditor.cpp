#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    const juce::Rectangle<int> masterPanel { 14, 140, 230, 382 };
    const juce::Rectangle<int> eqPanel     { 256, 140, 910, 382 };
    const juce::Rectangle<int> rowA        { 14, 532, 1152, 144 };
    const juce::Rectangle<int> rowB        { 14, 684, 1152, 136 };
    constexpr int bandH = 38;
}

CHETOEditor::CHETOEditor (CHETOProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), eqDisplay (p)
{
    setLookAndFeel (&lnf);
    logo = juce::ImageFileFormat::loadFrom (BinaryData::logo_png, (size_t) BinaryData::logo_pngSize);

    // ---- columna izquierda: MEJORAR TODO + entrada / salida ----
    setupKnob (masterKnob, {}, Theme::yellow);
    masterKnob.slider.getProperties().set ("big", true);
    masterKnob.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterKnob.att = std::make_unique<SliderAtt> (proc.apvts, "master", masterKnob.slider);

    setupKnob (inKnob, "ENTRADA", Theme::green);
    inKnob.att = std::make_unique<SliderAtt> (proc.apvts, "inGain", inKnob.slider);
    setupKnob (outKnob, "SALIDA", Theme::green);
    outKnob.att = std::make_unique<SliderAtt> (proc.apvts, "outGain", outKnob.slider);

    // ---- EQ ----
    addAndMakeVisible (eqDisplay);
    eqDisplay.onBandSelected = [this] (int b) { selectBand (b); };

    for (auto* b : { &eqPower, &hpBtn, &lpBtn })
    {
        b->setColour (juce::ToggleButton::tickColourId, Theme::ink);
        addAndMakeVisible (*b);
    }
    eqAtt = std::make_unique<ButtonAtt> (proc.apvts, "eqOn", eqPower);
    hpAtt = std::make_unique<ButtonAtt> (proc.apvts, "hpOn", hpBtn);
    lpAtt = std::make_unique<ButtonAtt> (proc.apvts, "lpOn", lpBtn);

    addAndMakeVisible (bandOn);
    setupKnob (bandFreq, "FRECUENCIA", Theme::yellow);
    setupKnob (bandGain, "GANANCIA", Theme::yellow);
    setupKnob (bandQ, "Q (ANCHO)", Theme::yellow);
    selectBand (0);

    // ---- modulos (el orden de creacion se usa en resized) ----
    auto& at = addModule ("AUTOTUNE", Theme::pink, "atOn");
    addCombo (at, "atKey", "NOTA");
    addCombo (at, "atScale", "ESCALA");
    addKnob (at, "atSpeed", "Velocidad");
    addKnob (at, "atAmount", "Cantidad");

    auto& comp = addModule ("COMPRESOR", Theme::teal, "compOn");
    addKnob (comp, "cThresh", "Umbral");
    addKnob (comp, "cRatio", "Ratio");
    addKnob (comp, "cAttack", "Ataque");
    addKnob (comp, "cRelease", "Release");
    addKnob (comp, "cMakeup", "Makeup");
    addKnob (comp, "cMix", "Mezcla");

    auto& sat = addModule ("SATURADOR", Theme::orange, "satOn");
    addKnob (sat, "satDrive", "Drive");
    addKnob (sat, "satMix", "Mezcla");

    auto& nr = addModule ("ANTI-RUIDO", Theme::sky, "nrOn");
    nr.minWeight = 2.6f;
    addChip (nr, "nrFreeze", "FIJAR");
    addKnob (nr, "nrAmount", "Reduccion");
    addKnob (nr, "nrSens", "Sensibilidad");

    auto& bc = addModule ("BITCRUSHER", Theme::mint, "bcOn");
    addKnob (bc, "bcBits", "Bits");
    addKnob (bc, "bcRate", "Reduccion");
    addKnob (bc, "bcMix", "Mezcla");

    auto& dly = addModule ("DELAY", Theme::violet, "dlyOn");
    addKnob (dly, "dlyTime", "Tiempo");
    addKnob (dly, "dlyFeedback", "Feedback");
    addKnob (dly, "dlyMix", "Mezcla");

    auto& rev = addModule ("REVERB", Theme::peach, "revOn");
    addKnob (rev, "revSize", "Tamano");
    addKnob (rev, "revDamp", "Damping");
    addKnob (rev, "revMix", "Mezcla");

    auto& lim = addModule ("LIMITADOR", Theme::yellow, "limOn");
    lim.minWeight = 1.5f;
    addKnob (lim, "limCeiling", "Techo");

    setSize (1180, 840);
}

//==============================================================================
void CHETOEditor::setupKnob (Knob& k, const juce::String& text, juce::Colour colour)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                  juce::MathConstants<float>::pi * 2.75f, true);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    k.slider.setColour (juce::Slider::rotarySliderFillColourId, colour);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (Theme::body (11.5f, true));
    k.label.setColour (juce::Label::textColourId, Theme::dim);

    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);
}

CHETOEditor::Module& CHETOEditor::addModule (const juce::String& title, juce::Colour colour,
                                             const juce::String& powerId)
{
    auto m = std::make_unique<Module>();
    m->title = title;
    m->colour = colour;
    m->power.setColour (juce::ToggleButton::tickColourId, Theme::ink);
    addAndMakeVisible (m->power);
    m->powerAtt = std::make_unique<ButtonAtt> (proc.apvts, powerId, m->power);

    modules.push_back (std::move (m));
    return *modules.back();
}

void CHETOEditor::addChip (Module& m, const juce::String& id, const juce::String& text)
{
    m.chip.setButtonText (text);
    m.chip.setColour (juce::ToggleButton::tickColourId, Theme::ink);
    addAndMakeVisible (m.chip);
    m.chipAtt = std::make_unique<ButtonAtt> (proc.apvts, id, m.chip);
    m.hasChip = true;
}

void CHETOEditor::addKnob (Module& m, const juce::String& id, const juce::String& text)
{
    auto k = std::make_unique<Knob>();
    setupKnob (*k, text, m.colour);
    k->att = std::make_unique<SliderAtt> (proc.apvts, id, k->slider);
    m.knobs.push_back (std::move (k));
}

void CHETOEditor::addCombo (Module& m, const juce::String& id, const juce::String& text)
{
    auto c = std::make_unique<Combo>();

    if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        c->box.addItemList (cp->choices, 1);

    c->label.setText (text, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centredLeft);
    c->label.setFont (Theme::body (11.5f, true));
    c->label.setColour (juce::Label::textColourId, Theme::dim);

    addAndMakeVisible (c->box);
    addAndMakeVisible (c->label);
    c->att = std::make_unique<ComboAtt> (proc.apvts, id, c->box);
    m.combos.push_back (std::move (c));
}

void CHETOEditor::selectBand (int band)
{
    currentBand = juce::jlimit (0, CHETOProcessor::numBands - 1, band);
    const juce::String n (currentBand + 1);
    const auto col = Theme::bandColour (currentBand);

    bandFreq.att.reset();
    bandGain.att.reset();
    bandQ.att.reset();
    bandOnAtt.reset();

    for (auto* k : { &bandFreq, &bandGain, &bandQ })
        k->slider.setColour (juce::Slider::rotarySliderFillColourId, col);
    bandOn.setColour (juce::ToggleButton::tickColourId, col);

    bandFreq.att = std::make_unique<SliderAtt> (proc.apvts, "b" + n + "Freq", bandFreq.slider);
    bandGain.att = std::make_unique<SliderAtt> (proc.apvts, "b" + n + "Gain", bandGain.slider);
    bandQ.att    = std::make_unique<SliderAtt> (proc.apvts, "b" + n + "Q", bandQ.slider);
    bandOnAtt    = std::make_unique<ButtonAtt> (proc.apvts, "b" + n + "On", bandOn);

    eqDisplay.setSelected (currentBand);
    repaint();
}

//==============================================================================
void CHETOEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour accent, int band)
{
    const auto f = r.toFloat();

    g.setColour (Theme::ink);                                  // sombra dura
    g.fillRoundedRectangle (f.translated (4.0f, 5.0f), 14.0f);

    g.setGradientFill (juce::ColourGradient (Theme::panelTop, 0.0f, f.getY(), Theme::panelBot, 0.0f, f.getBottom(), false));
    g.fillRoundedRectangle (f, 14.0f);

    if (band > 0)                                              // banda de color con el titulo
    {
        juce::Path clip;
        clip.addRoundedRectangle (f, 14.0f);
        g.saveState();
        g.reduceClipRegion (clip);
        g.setColour (accent);
        g.fillRect (f.withHeight ((float) band));
        g.setColour (Theme::ink);
        g.fillRect (f.getX(), f.getY() + (float) band - 1.5f, f.getWidth(), 3.0f);
        g.restoreState();
    }

    g.setColour (Theme::ink);
    g.drawRoundedRectangle (f.reduced (1.5f), 14.0f, 3.0f);
}

// Letras de CHETO dibujadas a mano con trazos: tipografia propia, no depende de ninguna fuente
void CHETOEditor::drawWordmark (juce::Graphics& g, float x, float y, float h)
{
    struct Letter { char c; float w; juce::Colour col; float rot; float dy; };
    const Letter letters[] = {
        { 'C', 1.00f, Theme::green,  -0.07f,  0.00f },
        { 'H', 0.80f, Theme::yellow,  0.05f, -0.04f },
        { 'E', 0.72f, Theme::orange, -0.04f,  0.03f },
        { 'T', 0.80f, Theme::teal,    0.06f, -0.03f },
        { 'O', 1.00f, Theme::peach,  -0.05f,  0.02f }
    };

    const float sw = h * 0.25f;
    float cx = x + sw;

    for (const auto& L : letters)
    {
        juce::Path p;

        switch (L.c)
        {
            case 'C': p.addCentredArc (0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 2.27f,
                                       juce::MathConstants<float>::twoPi + 0.87f, true); break;
            case 'H': p.startNewSubPath (0.06f, 0.0f); p.lineTo (0.06f, 1.0f);
                      p.startNewSubPath (0.74f, 0.0f); p.lineTo (0.74f, 1.0f);
                      p.startNewSubPath (0.06f, 0.5f); p.lineTo (0.74f, 0.5f); break;
            case 'E': p.startNewSubPath (0.70f, 0.0f); p.lineTo (0.06f, 0.0f); p.lineTo (0.06f, 1.0f); p.lineTo (0.70f, 1.0f);
                      p.startNewSubPath (0.06f, 0.5f); p.lineTo (0.58f, 0.5f); break;
            case 'T': p.startNewSubPath (0.04f, 0.0f); p.lineTo (0.76f, 0.0f);
                      p.startNewSubPath (0.40f, 0.0f); p.lineTo (0.40f, 1.0f); break;
            default:  p.addEllipse (0.0f, 0.0f, 1.0f, 1.0f); break;
        }

        p.applyTransform (juce::AffineTransform::translation (-L.w * 0.5f, -0.5f)
                              .rotated (L.rot)
                              .scaled (h)
                              .translated (cx + L.w * 0.5f * h, y + h * 0.5f + L.dy * h));

        const juce::PathStrokeType outline (sw + 10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        const juce::PathStrokeType fill (sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        const juce::PathStrokeType shine (sw * 0.22f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        juce::Path shadow (p);
        shadow.applyTransform (juce::AffineTransform::translation (5.0f, 6.0f));
        g.setColour (Theme::ink);
        g.strokePath (shadow, outline);
        g.strokePath (p, outline);

        g.setColour (L.col);
        g.strokePath (p, fill);

        juce::Path hl (p);
        hl.applyTransform (juce::AffineTransform::translation (-sw * 0.16f, -sw * 0.16f));
        g.setColour (Theme::cream.withAlpha (0.55f));
        g.strokePath (hl, shine);

        cx += (L.w + 0.6f) * h;
    }
}

void CHETOEditor::paint (juce::Graphics& g)
{
    // fondo arena
    g.setGradientFill (juce::ColourGradient (Theme::bgTop, 0.0f, 0.0f, Theme::bgBottom, 0.0f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (Theme::yellow.withAlpha (0.16f));
    g.fillEllipse ((float) getWidth() - 300.0f, -170.0f, 480.0f, 480.0f);
    g.setColour (Theme::green.withAlpha (0.07f));
    g.fillEllipse (-150.0f, 460.0f, 480.0f, 480.0f);

    g.setColour (Theme::ink.withAlpha (0.035f));
    for (int x = -getHeight(); x < getWidth(); x += 24)
        g.drawLine ((float) x, 0.0f, (float) (x + getHeight()), (float) getHeight(), 1.0f);

    // ---- cabecera: foto grande + CHETO ----
    const juce::Rectangle<float> badge (20.0f, 12.0f, 116.0f, 116.0f);
    g.setColour (Theme::ink);
    g.fillEllipse (badge.translated (4.0f, 5.0f));
    g.setColour (Theme::cream);
    g.fillEllipse (badge);

    if (logo.isValid())
    {
        juce::Path clip;
        clip.addEllipse (badge);
        g.saveState();
        g.reduceClipRegion (clip);
        g.drawImageWithin (logo, (int) badge.getX(), (int) badge.getY(), (int) badge.getWidth(), (int) badge.getHeight(),
                           juce::RectanglePlacement::centred);
        g.restoreState();
    }

    g.setColour (Theme::ink);
    g.drawEllipse (badge, 4.0f);

    drawWordmark (g, 156.0f, 30.0f, 72.0f);

    // etiquetas de funciones (arriba a la derecha)
    {
        struct Pill { const char* t; juce::Colour c; };
        const Pill pills[] = { { "EQ PRO", Theme::yellow }, { "AUTOTUNE", Theme::pink }, { "ANTI-RUIDO", Theme::sky },
                               { "BITCRUSHER", Theme::mint }, { "COMPRESOR", Theme::teal } };

        float px = 700.0f;
        g.setFont (Theme::body (11.5f, true));

        for (const auto& pl : pills)
        {
            const float w = 18.0f + 7.2f * (float) juce::String (pl.t).length();
            const juce::Rectangle<float> r (px, 36.0f, w, 24.0f);
            g.setColour (Theme::ink);
            g.fillRoundedRectangle (r.translated (2.0f, 3.0f), 12.0f);
            g.setColour (pl.c);
            g.fillRoundedRectangle (r, 12.0f);
            g.setColour (Theme::ink);
            g.drawRoundedRectangle (r, 12.0f, 2.0f);
            g.drawText (pl.t, r.toNearestInt(), juce::Justification::centred);
            px += w + 9.0f;
        }
    }

    g.setColour (Theme::ink.withAlpha (0.7f));
    g.setFont (Theme::body (13.0f, true));
    g.drawText ("el mejorador total  ·  by Santino", 700, 78, getWidth() - 716, 22, juce::Justification::centredRight);

    // ---- columna MEJORAR TODO ----
    drawPanel (g, masterPanel, Theme::yellow, bandH);
    g.setColour (Theme::ink);
    g.setFont (Theme::title (25.0f));
    g.drawText ("MEJORAR TODO", masterPanel.getX(), masterPanel.getY(), masterPanel.getWidth(), bandH, juce::Justification::centred);
    g.setColour (Theme::dim);
    g.setFont (Theme::body (12.0f, true));
    g.drawText ("una perilla, todo suena mejor", masterPanel.getX(), masterPanel.getY() + bandH + 6, masterPanel.getWidth(), 16, juce::Justification::centred);

    // ---- EQ ----
    drawPanel (g, eqPanel, Theme::yellow, bandH);
    g.setColour (Theme::ink);
    g.setFont (Theme::title (23.0f));
    g.drawText ("EQ PRO", eqPanel.getX() + 18, eqPanel.getY(), 100, bandH, juce::Justification::centredLeft);
    g.setColour (Theme::ink.withAlpha (0.75f));
    g.setFont (Theme::body (12.0f, true));
    g.drawText ("arrastra los puntos  ·  rueda = ancho (Q)  ·  doble click = prender / apagar",
                eqPanel.getX() + 118, eqPanel.getY(), 560, bandH, juce::Justification::centredLeft);

    const juce::Rectangle<float> tag ((float) eqPanel.getX() + 16.0f, (float) eqPanel.getBottom() - 78.0f, 104.0f, 28.0f);
    g.setColour (Theme::ink);
    g.fillRoundedRectangle (tag.translated (2.0f, 3.0f), 14.0f);
    g.setColour (Theme::bandColour (currentBand));
    g.fillRoundedRectangle (tag, 14.0f);
    g.setColour (Theme::ink);
    g.drawRoundedRectangle (tag, 14.0f, 2.0f);
    g.setFont (Theme::title (17.0f));
    g.drawText ("BANDA " + juce::String (currentBand + 1), tag.toNearestInt(), juce::Justification::centred);

    // ---- modulos ----
    for (auto& m : modules)
    {
        drawPanel (g, m->bounds, m->colour, bandH);
        g.setColour (Theme::ink);
        g.setFont (Theme::title (20.0f));
        g.drawText (m->title, m->header.withTrimmedLeft (44), juce::Justification::centredLeft);
    }
}

//==============================================================================
void CHETOEditor::placeKnob (Knob& k, juce::Rectangle<int> cell)
{
    k.label.setBounds (cell.removeFromTop (15));
    k.slider.setBounds (cell);
}

void CHETOEditor::layoutRow (juce::Rectangle<int> row, std::vector<Module*> mods)
{
    const int gap = 10;
    float totalWeight = 0.0f;
    std::vector<float> weights;

    for (auto* m : mods)
    {
        float w = (m->combos.empty() ? 0.0f : 1.7f) + (float) m->knobs.size();
        w = juce::jmax (w, m->minWeight);
        weights.push_back (w);
        totalWeight += w;
    }

    const float unit = (float) (row.getWidth() - gap * ((int) mods.size() - 1)) / totalWeight;

    for (size_t i = 0; i < mods.size(); ++i)
    {
        auto& m = *mods[i];
        m.bounds = row.removeFromLeft ((int) std::round (unit * weights[i]));
        row.removeFromLeft (gap);

        auto inner = m.bounds.reduced (12, 7);
        m.header = inner.removeFromTop (bandH - 7 - 4);

        auto hd = m.header;
        m.power.setBounds (hd.removeFromLeft (38));
        if (m.hasChip)
            m.chip.setBounds (hd.removeFromRight (60).reduced (0, 1));

        inner.removeFromTop (10);

        const float u = (float) inner.getWidth() / weights[i];

        if (! m.combos.empty())
        {
            auto col = inner.removeFromLeft ((int) (u * 1.7f)).reduced (0, 2).withTrimmedRight (10);
            const int each = col.getHeight() / (int) m.combos.size();

            for (auto& c : m.combos)
            {
                auto cell = col.removeFromTop (each);
                c->label.setBounds (cell.removeFromTop (16));
                c->box.setBounds (cell.removeFromTop (28));
            }
        }
        else if ((float) m.knobs.size() < weights[i] - 0.01f)
        {
            inner.removeFromLeft ((int) ((weights[i] - (float) m.knobs.size()) * u * 0.5f));
        }

        for (auto& k : m.knobs)
            placeKnob (*k, inner.removeFromLeft ((int) u));
    }
}

void CHETOEditor::resized()
{
    // MEJORAR TODO
    masterKnob.slider.setBounds (masterPanel.getX() + 12, masterPanel.getY() + 68, masterPanel.getWidth() - 24, 206);

    placeKnob (inKnob,  { masterPanel.getX() + 12,  masterPanel.getBottom() - 96, 100, 88 });
    placeKnob (outKnob, { masterPanel.getX() + 118, masterPanel.getBottom() - 96, 100, 88 });

    // EQ
    eqPower.setBounds (eqPanel.getRight() - 176, eqPanel.getY() + 8, 52, 22);
    hpBtn.setBounds   (eqPanel.getRight() - 118, eqPanel.getY() + 8, 52, 22);
    lpBtn.setBounds   (eqPanel.getRight() - 60,  eqPanel.getY() + 8, 48, 22);

    eqDisplay.setBounds (eqPanel.getX() + 10, eqPanel.getY() + bandH + 8, eqPanel.getWidth() - 20, 250);

    const int sy = eqPanel.getBottom() - 84;
    bandOn.setBounds (eqPanel.getX() + 130, sy + 4, 40, 22);
    placeKnob (bandFreq, { eqPanel.getX() + 196, sy, 120, 80 });
    placeKnob (bandGain, { eqPanel.getX() + 326, sy, 120, 80 });
    placeKnob (bandQ,    { eqPanel.getX() + 456, sy, 120, 80 });

    // modulos: 0 autotune, 1 compresor, 2 saturador, 3 anti-ruido, 4 bitcrusher, 5 delay, 6 reverb, 7 limitador
    auto get = [this] (size_t i) { return modules[i].get(); };
    layoutRow (rowA, { get (0), get (1), get (2) });
    layoutRow (rowB, { get (3), get (4), get (5), get (6), get (7) });
}
