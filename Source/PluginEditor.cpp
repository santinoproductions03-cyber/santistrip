#include "PluginEditor.h"

namespace
{
    const juce::Colour bgColour     { 0xff14161c };
    const juce::Colour panelColour  { 0xff1e222b };
    const juce::Colour accentColour { 0xff4fd1c5 };
    const juce::Colour textColour   { 0xffd8dde6 };
}

SantiStripEditor::SantiStripEditor (SantiStripProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    // 0 - Entrada
    auto& in = addSection ("ENTRADA", {});
    addKnob (in, "inGain", "Entrada");

    // 1 - EQ
    auto& eq = addSection ("ECUALIZADOR", "eqOn");
    addKnob (eq, "lsFreq", "LS Freq");
    addKnob (eq, "lsGain", "LS Gain");
    addKnob (eq, "m1Freq", "M1 Freq");
    addKnob (eq, "m1Gain", "M1 Gain");
    addKnob (eq, "m1Q",    "M1 Q");
    addKnob (eq, "m2Freq", "M2 Freq");
    addKnob (eq, "m2Gain", "M2 Gain");
    addKnob (eq, "m2Q",    "M2 Q");
    addKnob (eq, "hsFreq", "HS Freq");
    addKnob (eq, "hsGain", "HS Gain");

    // 2 - Compresor
    auto& comp = addSection ("COMPRESOR", "compOn");
    addKnob (comp, "cThresh",  "Umbral");
    addKnob (comp, "cRatio",   "Ratio");
    addKnob (comp, "cAttack",  "Ataque");
    addKnob (comp, "cRelease", "Release");
    addKnob (comp, "cMakeup",  "Makeup");
    addKnob (comp, "cMix",     "Mezcla");

    // 3 - Saturador
    auto& sat = addSection ("SATURADOR", "satOn");
    addKnob (sat, "satDrive", "Drive");
    addKnob (sat, "satMix",   "Mezcla");

    // 4 - Delay
    auto& dly = addSection ("DELAY", "dlyOn");
    addKnob (dly, "dlyTime",     "Tiempo");
    addKnob (dly, "dlyFeedback", "Feedback");
    addKnob (dly, "dlyMix",      "Mezcla");

    // 5 - Reverb
    auto& rev = addSection ("REVERB", "revOn");
    addKnob (rev, "revSize", "Tamano");
    addKnob (rev, "revDamp", "Damping");
    addKnob (rev, "revMix",  "Mezcla");

    // 6 - Limitador / salida
    auto& lim = addSection ("LIMITADOR", "limOn");
    addKnob (lim, "limCeiling", "Techo");
    addKnob (lim, "outGain",    "Salida");

    setSize (1120, 470);
}

SantiStripEditor::Section& SantiStripEditor::addSection (const juce::String& title,
                                                         const juce::String& powerParamId)
{
    auto s = std::make_unique<Section>();

    s->title.setText (title, juce::dontSendNotification);
    s->title.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
    s->title.setColour (juce::Label::textColourId, accentColour);
    addAndMakeVisible (s->title);

    s->hasPower = powerParamId.isNotEmpty();

    if (s->hasPower)
    {
        s->power.setColour (juce::ToggleButton::tickColourId, accentColour);
        s->power.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
        addAndMakeVisible (s->power);
        s->powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            proc.apvts, powerParamId, s->power);
    }

    sections.push_back (std::move (s));
    return *sections.back();
}

void SantiStripEditor::addKnob (Section& section, const juce::String& paramId, const juce::String& text)
{
    auto k = std::make_unique<Knob>();

    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 16);
    k->slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    k->slider.setColour (juce::Slider::thumbColourId, accentColour);
    k->slider.setColour (juce::Slider::textBoxTextColourId, textColour);
    k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    k->label.setText (text, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (juce::Font (juce::FontOptions (11.0f)));
    k->label.setColour (juce::Label::textColourId, textColour.withAlpha (0.8f));

    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);

    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        proc.apvts, paramId, k->slider);

    section.knobs.push_back (std::move (k));
}

void SantiStripEditor::paint (juce::Graphics& g)
{
    g.fillAll (bgColour);

    g.setColour (textColour);
    g.setFont (juce::Font (juce::FontOptions (22.0f).withStyle ("Bold")));
    g.drawText ("SantiStrip", 14, 8, 300, 28, juce::Justification::centredLeft);

    g.setColour (textColour.withAlpha (0.45f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("EQ  |  Compresor  |  Saturador  |  Delay  |  Reverb  |  Limitador",
                getWidth() - 520, 8, 506, 28, juce::Justification::centredRight);

    for (auto& s : sections)
    {
        g.setColour (panelColour);
        g.fillRoundedRectangle (s->bounds.toFloat(), 8.0f);
    }
}

void SantiStripEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (38);

    const int gap = 8;
    auto row1 = area.removeFromTop ((area.getHeight() - gap) / 2);
    area.removeFromTop (gap);

    layoutRow (row1, { 0, 1 });
    layoutRow (area, { 2, 3, 4, 5, 6 });
}

void SantiStripEditor::layoutRow (juce::Rectangle<int> row, std::initializer_list<int> indices)
{
    const int gap = 8;
    const int numSections = (int) indices.size();

    int totalKnobs = 0;
    for (auto i : indices)
        totalKnobs += (int) sections[(size_t) i]->knobs.size();

    const float knobW = (float) (row.getWidth() - gap * (numSections - 1)) / (float) totalKnobs;

    for (auto i : indices)
    {
        auto& s = *sections[(size_t) i];
        const int count = (int) s.knobs.size();

        s.bounds = row.removeFromLeft ((int) std::round (knobW * (float) count));
        row.removeFromLeft (gap);

        auto inner  = s.bounds.reduced (4);
        auto header = inner.removeFromTop (24);

        if (s.hasPower)
            s.power.setBounds (header.removeFromLeft (24));

        s.title.setBounds (header);

        const int kw = inner.getWidth() / count;

        for (auto& k : s.knobs)
        {
            auto kb = inner.removeFromLeft (kw);
            k->label.setBounds (kb.removeFromTop (16));
            k->slider.setBounds (kb);
        }
    }
}
