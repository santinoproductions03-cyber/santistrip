#pragma once

#include "PluginProcessor.h"
#include <initializer_list>
#include <memory>
#include <vector>

class SantiStripEditor : public juce::AudioProcessorEditor
{
public:
    explicit SantiStripEditor (SantiStripProcessor&);
    ~SantiStripEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Section
    {
        juce::Label title;
        juce::ToggleButton power;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;
        bool hasPower = false;
        std::vector<std::unique_ptr<Knob>> knobs;
        juce::Rectangle<int> bounds;
    };

    Section& addSection (const juce::String& title, const juce::String& powerParamId);
    void addKnob (Section& section, const juce::String& paramId, const juce::String& text);
    void layoutRow (juce::Rectangle<int> row, std::initializer_list<int> indices);

    SantiStripProcessor& proc;
    std::vector<std::unique_ptr<Section>> sections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SantiStripEditor)
};
