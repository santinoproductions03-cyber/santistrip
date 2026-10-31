#pragma once

#include "PluginProcessor.h"
#include "CHETOLookAndFeel.h"
#include "EqDisplay.h"
#include <memory>
#include <vector>

class CHETOEditor : public juce::AudioProcessorEditor
{
public:
    explicit CHETOEditor (CHETOProcessor&);
    ~CHETOEditor() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAtt> att;
    };

    struct Combo
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<ComboAtt> att;
    };

    struct Module
    {
        juce::String title;
        juce::Colour colour;
        juce::ToggleButton power;
        std::unique_ptr<ButtonAtt> powerAtt;
        juce::ToggleButton chip;
        std::unique_ptr<ButtonAtt> chipAtt;
        bool hasChip = false;
        std::vector<std::unique_ptr<Knob>> knobs;
        std::vector<std::unique_ptr<Combo>> combos;
        float minWeight = 0.0f;
        juce::Rectangle<int> bounds, header;
    };

    Module& addModule (const juce::String& title, juce::Colour colour, const juce::String& powerId);
    void addChip (Module& m, const juce::String& id, const juce::String& text);
    void addKnob (Module& m, const juce::String& id, const juce::String& text);
    void addCombo (Module& m, const juce::String& id, const juce::String& text);
    void setupKnob (Knob& k, const juce::String& text, juce::Colour colour);
    void placeKnob (Knob& k, juce::Rectangle<int> cell);
    void layoutRow (juce::Rectangle<int> row, std::vector<Module*> mods);
    void selectBand (int band);
    void drawPanel (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour accent, int bandHeight);
    void drawWordmark (juce::Graphics& g, float x, float y, float h);

    CHETOProcessor& proc;
    CHETOLookAndFeel lnf;
    juce::Image logo;

    EqDisplay eqDisplay;
    std::vector<std::unique_ptr<Module>> modules;

    // Columna izquierda
    Knob masterKnob, inKnob, outKnob;

    // EQ: cabecera y tira de la banda elegida
    juce::ToggleButton eqPower { "EQ" }, hpBtn { "HP" }, lpBtn { "LP" }, bandOn;
    std::unique_ptr<ButtonAtt> eqAtt, hpAtt, lpAtt, bandOnAtt;
    Knob bandFreq, bandGain, bandQ;
    int currentBand = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CHETOEditor)
};
