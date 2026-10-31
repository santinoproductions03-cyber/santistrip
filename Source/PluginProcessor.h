#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "Autotune.h"
#include "NoiseReducer.h"

class CHETOProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    static constexpr int numBands  = 6;
    static constexpr int scopeSize = 2048;

    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    CHETOProcessor();
    ~CHETOProcessor() override { cancelPendingUpdate(); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CHETO"; }
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

    // Para el dibujo del EQ (los usa el editor)
    static Coeffs::Ptr makeBandCoeffs (int band, double sr, float freq, float q, float gainDb);
    static Coeffs::Ptr makeCutCoeffs (bool highPass, double sr, float freq);
    void readScope (float* dest) const;
    double getCurrentSampleRate() const { return currentSr; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float param (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void handleAsyncUpdate() override;

    using Filter     = juce::dsp::IIR::Filter<float>;
    using Duplicator = juce::dsp::ProcessorDuplicator<Filter, Coeffs>;

    struct Slot
    {
        Duplicator filter;
        float a = -1.0e9f, b = -1.0e9f, c = -1.0e9f;
    };

    double currentSr = 44100.0;

    // Autotune
    AutoTuner tuner;
    bool lastAutotuneOn = false;
    std::atomic<int> wantedLatency { 0 };
    int lastLatency = 0;

    // Reductor de ruido
    NoiseReducer denoiser;
    bool lastNrOn = false;

    // EQ pro
    std::array<Slot, numBands> eq;
    Slot hp, lp;

    // Compresor / limitador
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Limiter<float> limiter;
    juce::AudioBuffer<float> dryBuffer;

    // Bitcrusher
    std::array<float, 2> crushHeld { 0.0f, 0.0f };
    std::array<float, 2> crushCount { 0.0f, 0.0f };

    // Delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delaySmoothed;
    int maxDelaySamples = 48000;

    // Reverb
    juce::Reverb reverb;

    // Perilla MEJORAR TODO
    Slot enhHp, enhLow, enhMud, enhPres, enhAir;
    juce::dsp::Compressor<float> glue;

    float prevInGain = 1.0f, prevOutGain = 1.0f, prevMakeup = 1.0f, prevEnhGain = 1.0f;

    // Para el analizador de espectro
    std::array<float, 4096> scopeRing {};
    std::atomic<int> scopeWrite { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CHETOProcessor)
};
