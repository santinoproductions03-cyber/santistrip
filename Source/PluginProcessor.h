#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class SantiStripProcessor : public juce::AudioProcessor
{
public:
    SantiStripProcessor();
    ~SantiStripProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SantiStrip"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    float param (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    using Filter    = juce::dsp::IIR::Filter<float>;
    using Coeffs    = juce::dsp::IIR::Coefficients<float>;
    using Duplicator = juce::dsp::ProcessorDuplicator<Filter, Coeffs>;

    // EQ de 4 bandas
    Duplicator lowShelf, mid1, mid2, highShelf;

    // Compresor y limitador
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Limiter<float> limiter;
    juce::AudioBuffer<float> dryBuffer;

    // Delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delaySmoothed;
    int maxDelaySamples = 48000;

    // Reverb
    juce::Reverb reverb;

    // Para suavizar los cambios de ganancia
    float prevInGain = 1.0f, prevOutGain = 1.0f, prevMakeup = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SantiStripProcessor)
};
