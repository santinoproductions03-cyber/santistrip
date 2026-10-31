#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    const juce::Rectangle<int> masterPanel { 14, 92, 230, 382 };
    const juce::Rectangle<int> eqPanel     { 256, 92, 910, 382 };
    const juce::Rectangle<int> rowA        { 14, 484, 1152, 144 };
    const juce::Rectangle<int> rowB        { 14, 636, 1152, 136 };
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

    setupKnob (inKnob, "ENTRADA", Theme::mint);
    inKnob.att = std::make_unique<SliderAtt> (proc.apvts, "inGain", inKnob.slider);
    setupKnob (outKnob, "SALIDA", Theme::mint);
    outKnob.att = std::make_unique<SliderAtt> (proc.apvts, "outGain", outKnob.slider);

    // ---- EQ ----
    addAndMakeVisible (eqDisplay);
    eqDisplay.onBandSelected = [this] (int b) { selectBand (b); };

    for (auto* b : { &eqPower, &hpBtn, &lpBtn })
    {
        b->setColour (juce::ToggleButton::tickColourId, Theme::yellow);
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

    // ---- modulos ----
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

    auto& bc = addModule ("BITCRUSHER", Theme::mint, "bcOn");
    addKnob (bc, "bcBits", "Bits");
    addKnob (bc, "bcRate", "Reduccion");
    addKnob (bc, "bcMix", "Mezcla");

    auto& dly = addModule ("DELAY", Theme::sky, "dlyOn");
    addKnob (dly, "dlyTime", "Tiempo");
    addKnob (dly, "dlyFeedback", "Feedback");
    addKnob (dly, "dlyMix", "Mezcla");

    auto& rev = addModule ("REVERB", Theme::violet, "revOn");
    addKnob (rev, "revSize", "Tamano");
    addKnob (rev, "revDamp", "Damping");
    addKnob (rev, "revMix", "Mezcla");

    auto& lim = addModule ("LIMITADOR", Theme::yellow, "limOn");
    lim.minWeight = 1.7f;
    addKnob (lim, "limCeiling", "Techo");

    setSize (1180, 780);
}

//==============================================================================
void CHETOEditor::setupKnob (Knob& k, const juce::String& text, juce::Colour colour)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                  juce::MathConstants<float>::pi * 2.75f, true);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 16);
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
    m->power.setColour (juce::ToggleButton::tickColourId, colour);
    addAndMakeVisible (m->power);
    m->powerAtt = std::make_unique<ButtonAtt> (proc.apvts, powerId, m->power);

    modules.push_back (std::move (m));
    return *modules.back();
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
void CHETOEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour accent)
{
    auto f = r.toFloat();
    g.setGradientFill (juce::ColourGradient (Theme::panelTop, 0.0f, f.getY(), Theme::panelBot, 0.0f, f.getBottom(), false));
    g.fillRoundedRectangle (f, 14.0f);
    g.setColour (Theme::ink);
    g.drawRoundedRectangle (f.reduced (1.0f), 14.0f, 2.5f);
    g.setColour (accent.withAlpha (0.55f));
    g.drawRoundedRectangle (f.reduced (4.0f), 11.0f, 1.0f);
}

void CHETOEditor::paint (juce::Graphics& g)
{
    // fondo
    g.setGradientFill (juce::ColourGradient (Theme::bgTop, 0.0f, 0.0f, Theme::bgBottom, 0.0f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (Theme::green.withAlpha (0.08f));
    g.fillEllipse (-140.0f, 400.0f, 460.0f, 460.0f);
    g.setColour (Theme::yellow.withAlpha (0.05f));
    g.fillEllipse ((float) getWidth() - 280.0f, -160.0f, 460.0f, 460.0f);

    g.setColour (juce::Colours::white.withAlpha (0.025f));
    for (int x = -getHeight(); x < getWidth(); x += 26)
        g.drawLine ((float) x, 0.0f, (float) (x + getHeight()), (float) getHeight(), 1.0f);

    // ---- cabecera: logo + nombre ----
    const juce::Rectangle<float> badge (18.0f, 8.0f, 76.0f, 76.0f);
    g.setColour (Theme::ink);
    g.fillEllipse (badge.expanded (4.0f));
    g.setColour (Theme::yellow);
    g.fillEllipse (badge.expanded (1.5f));
    g.setColour (Theme::green);
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

    g.setFont (Theme::title (62.0f));
    g.setColour (Theme::ink);
    for (int dx = -3; dx <= 3; dx += 3)
        for (int dy = -3; dy <= 6; dy += 3)
            g.drawText ("CHETO", 112 + dx, 4 + dy, 330, 72, juce::Justification::centredLeft);
    g.setColour (Theme::yellow);
    g.drawText ("CHETO", 112, 4, 330, 72, juce::Justification::centredLeft);

    g.setColour (Theme::dim);
    g.setFont (Theme::body (12.5f, true));
    g.drawText ("EL MEJORADOR TOTAL   ·   EQ PRO   ·   AUTOTUNE   ·   BITCRUSHER   ·   COMPRESOR   ·   FX",
                330, 30, getWidth() - 346, 20, juce::Justification::centredRight);
    g.setColour (Theme::text.withAlpha (0.5f));
    g.drawText ("by Santino", 330, 50, getWidth() - 346, 20, juce::Justification::centredRight);

    // ---- columna MEJORAR TODO ----
    drawPanel (g, masterPanel, Theme::yellow);
    g.setColour (Theme::yellow);
    g.setFont (Theme::title (26.0f));
    g.drawText ("MEJORAR TODO", masterPanel.getX(), masterPanel.getY() + 8, masterPanel.getWidth(), 30, juce::Justification::centred);
    g.setColour (Theme::dim);
    g.setFont (Theme::body (12.0f));
    g.drawText ("una perilla, todo suena mejor", masterPanel.getX(), masterPanel.getY() + 36, masterPanel.getWidth(), 16, juce::Justification::centred);

    // ---- EQ ----
    drawPanel (g, eqPanel, Theme::yellow);
    g.setColour (Theme::yellow);
    g.setFont (Theme::title (22.0f));
    g.drawText ("EQ PRO", eqPanel.getX() + 16, eqPanel.getY() + 6, 100, 26, juce::Justification::centredLeft);
    g.setColour (Theme::dim);
    g.setFont (Theme::body (12.0f));
    g.drawText ("arrastra los puntos  ·  rueda = ancho (Q)  ·  doble click = prender / apagar",
                eqPanel.getX() + 110, eqPanel.getY() + 8, 560, 22, juce::Justification::centredLeft);

    const auto bc = Theme::bandColour (currentBand);
    g.setColour (bc);
    g.setFont (Theme::title (19.0f));
    g.drawText ("BANDA " + juce::String (currentBand + 1), eqPanel.getX() + 16, eqPanel.getBottom() - 80, 110, 26, juce::Justification::centredLeft);

    // ---- modulos ----
    for (auto& m : modules)
    {
        drawPanel (g, m->bounds, m->colour);
        g.setColour (m->colour);
        g.setFont (Theme::title (19.0f));
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
    const int gap = 8;
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

        auto inner = m.bounds.reduced (12, 9);
        m.header = inner.removeFromTop (24);
        m.power.setBounds (m.header.withWidth (38));
        inner.removeFromTop (2);

        const float u = (float) inner.getWidth() / weights[i];

        if (! m.combos.empty())
        {
            auto col = inner.removeFromLeft ((int) (u * 1.7f)).reduced (0, 4).withTrimmedRight (10);
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
    masterKnob.label.setBounds (0, 0, 0, 0);
    masterKnob.slider.setBounds (masterPanel.getX() + 12, masterPanel.getY() + 58, masterPanel.getWidth() - 24, 220);

    placeKnob (inKnob,  { masterPanel.getX() + 12,  masterPanel.getBottom() - 100, 100, 92 });
    placeKnob (outKnob, { masterPanel.getX() + 118, masterPanel.getBottom() - 100, 100, 92 });

    // EQ
    eqPower.setBounds (eqPanel.getRight() - 170, eqPanel.getY() + 8, 50, 22);
    hpBtn.setBounds   (eqPanel.getRight() - 114, eqPanel.getY() + 8, 50, 22);
    lpBtn.setBounds   (eqPanel.getRight() - 58,  eqPanel.getY() + 8, 46, 22);

    eqDisplay.setBounds (eqPanel.getX() + 10, eqPanel.getY() + 36, eqPanel.getWidth() - 20, 258);

    const int sy = eqPanel.getBottom() - 84;
    bandOn.setBounds (eqPanel.getX() + 128, sy + 4, 40, 22);
    placeKnob (bandFreq, { eqPanel.getX() + 190, sy, 120, 80 });
    placeKnob (bandGain, { eqPanel.getX() + 320, sy, 120, 80 });
    placeKnob (bandQ,    { eqPanel.getX() + 450, sy, 120, 80 });

    // modulos
    auto get = [this] (size_t i) { return modules[i].get(); };
    layoutRow (rowA, { get (0), get (1), get (2) });
    layoutRow (rowB, { get (3), get (4), get (5), get (6) });
}
