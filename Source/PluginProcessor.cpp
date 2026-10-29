#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;

    void addFloat (Layout& layout, const juce::String& id, const juce::String& name,
                   float minV, float maxV, float defV, const juce::String& unit,
                   float skewCentre = 0.0f)
    {
        juce::NormalisableRange<float> range (minV, maxV, 0.01f);

        if (skewCentre > 0.0f)
            range.setSkewForCentre (skewCentre);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, defV,
            juce::AudioParameterFloatAttributes().withLabel (unit)));
    }

    void addBool (Layout& layout, const juce::String& id, const juce::String& name, bool defV)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, defV));
    }

    inline float dbToGain (float db) { return juce::Decibels::decibelsToGain (db); }
}

//==============================================================================
SantiStripProcessor::SantiStripProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

Layout SantiStripProcessor::createLayout()
{
    Layout l;

    // Entrada / salida
    addFloat (l, "inGain",  "Ganancia de entrada", -24.0f, 24.0f, 0.0f, " dB");
    addFloat (l, "outGain", "Ganancia de salida",  -24.0f, 24.0f, 0.0f, " dB");

    // EQ
    addBool  (l, "eqOn", "EQ activo", true);
    addFloat (l, "lsFreq", "EQ Low Shelf Freq",   20.0f,   500.0f,  100.0f, " Hz", 100.0f);
    addFloat (l, "lsGain", "EQ Low Shelf Gain",  -15.0f,    15.0f,    0.0f, " dB");
    addFloat (l, "m1Freq", "EQ Mid 1 Freq",      100.0f,  2000.0f,  400.0f, " Hz", 450.0f);
    addFloat (l, "m1Gain", "EQ Mid 1 Gain",      -15.0f,    15.0f,    0.0f, " dB");
    addFloat (l, "m1Q",    "EQ Mid 1 Q",           0.3f,     6.0f,    1.0f, "");
    addFloat (l, "m2Freq", "EQ Mid 2 Freq",      500.0f,  8000.0f, 2500.0f, " Hz", 2000.0f);
    addFloat (l, "m2Gain", "EQ Mid 2 Gain",      -15.0f,    15.0f,    0.0f, " dB");
    addFloat (l, "m2Q",    "EQ Mid 2 Q",           0.3f,     6.0f,    1.0f, "");
    addFloat (l, "hsFreq", "EQ High Shelf Freq", 2000.0f, 18000.0f, 8000.0f, " Hz", 6000.0f);
    addFloat (l, "hsGain", "EQ High Shelf Gain", -15.0f,    15.0f,    0.0f, " dB");

    // Compresor
    addBool  (l, "compOn", "Compresor activo", true);
    addFloat (l, "cThresh",  "Comp Umbral",   -60.0f,    0.0f, -12.0f, " dB");
    addFloat (l, "cRatio",   "Comp Ratio",      1.0f,   20.0f,   2.0f, ":1", 4.0f);
    addFloat (l, "cAttack",  "Comp Ataque",     0.1f,  100.0f,  15.0f, " ms", 10.0f);
    addFloat (l, "cRelease", "Comp Release",   10.0f, 1000.0f, 150.0f, " ms", 150.0f);
    addFloat (l, "cMakeup",  "Comp Makeup",     0.0f,   24.0f,   0.0f, " dB");
    addFloat (l, "cMix",     "Comp Mezcla",     0.0f,  100.0f, 100.0f, " %");

    // Saturacion
    addBool  (l, "satOn", "Saturador activo", true);
    addFloat (l, "satDrive", "Sat Drive",  0.0f, 30.0f, 6.0f, " dB");
    addFloat (l, "satMix",   "Sat Mezcla", 0.0f, 100.0f, 0.0f, " %");

    // Delay
    addBool  (l, "dlyOn", "Delay activo", true);
    addFloat (l, "dlyTime",     "Delay Tiempo",   10.0f, 1000.0f, 300.0f, " ms", 250.0f);
    addFloat (l, "dlyFeedback", "Delay Feedback",  0.0f,   90.0f,  35.0f, " %");
    addFloat (l, "dlyMix",      "Delay Mezcla",    0.0f,  100.0f,   0.0f, " %");

    // Reverb
    addBool  (l, "revOn", "Reverb activo", true);
    addFloat (l, "revSize", "Reverb Tamano",  0.0f, 100.0f, 50.0f, " %");
    addFloat (l, "revDamp", "Reverb Damping", 0.0f, 100.0f, 50.0f, " %");
    addFloat (l, "revMix",  "Reverb Mezcla",  0.0f, 100.0f,  0.0f, " %");

    // Limitador
    addBool  (l, "limOn", "Limitador activo", true);
    addFloat (l, "limCeiling", "Limitador Techo", -12.0f, 0.0f, -0.3f, " dB");

    return l;
}

//==============================================================================
bool SantiStripProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void SantiStripProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    lowShelf.prepare (spec);
    mid1.prepare (spec);
    mid2.prepare (spec);
    highShelf.prepare (spec);
    lowShelf.reset();
    mid1.reset();
    mid2.reset();
    highShelf.reset();

    compressor.prepare (spec);
    compressor.reset();
    limiter.prepare (spec);
    limiter.reset();

    maxDelaySamples = (int) sampleRate + 2;
    delayLine.setMaximumDelayInSamples (maxDelaySamples);
    delayLine.prepare (spec);
    delayLine.reset();
    delaySmoothed.reset (sampleRate, 0.1);
    delaySmoothed.setCurrentAndTargetValue (param ("dlyTime") * 0.001f * (float) sampleRate);

    reverb.setSampleRate (sampleRate);
    reverb.reset();

    dryBuffer.setSize (2, samplesPerBlock, false, false, true);

    prevInGain  = dbToGain (param ("inGain"));
    prevOutGain = dbToGain (param ("outGain"));
    prevMakeup  = dbToGain (param ("cMakeup"));
}

void SantiStripProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();
    const double sr = getSampleRate();

    for (int ch = getTotalNumInputChannels(); ch < numCh; ++ch)
        buffer.clear (ch, 0, n);

    // ---- Ganancia de entrada ----
    {
        const float g = dbToGain (param ("inGain"));
        buffer.applyGainRamp (0, n, prevInGain, g);
        prevInGain = g;
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // ---- EQ ----
    if (param ("eqOn") > 0.5f)
    {
        const float nyq = (float) sr * 0.45f;

        *lowShelf.state  = *Coeffs::makeLowShelf  (sr, juce::jmin (param ("lsFreq"), nyq), 0.707f, dbToGain (param ("lsGain")));
        *mid1.state      = *Coeffs::makePeakFilter (sr, juce::jmin (param ("m1Freq"), nyq), param ("m1Q"), dbToGain (param ("m1Gain")));
        *mid2.state      = *Coeffs::makePeakFilter (sr, juce::jmin (param ("m2Freq"), nyq), param ("m2Q"), dbToGain (param ("m2Gain")));
        *highShelf.state = *Coeffs::makeHighShelf (sr, juce::jmin (param ("hsFreq"), nyq), 0.707f, dbToGain (param ("hsGain")));

        lowShelf.process (ctx);
        mid1.process (ctx);
        mid2.process (ctx);
        highShelf.process (ctx);
    }

    // ---- Compresor (con mezcla paralela) ----
    if (param ("compOn") > 0.5f)
    {
        const float mix = param ("cMix") * 0.01f;

        if (mix < 0.999f)
            dryBuffer.makeCopyOf (buffer, true);

        compressor.setThreshold (param ("cThresh"));
        compressor.setRatio (param ("cRatio"));
        compressor.setAttack (param ("cAttack"));
        compressor.setRelease (param ("cRelease"));
        compressor.process (ctx);

        const float makeup = dbToGain (param ("cMakeup"));
        buffer.applyGainRamp (0, n, prevMakeup, makeup);
        prevMakeup = makeup;

        if (mix < 0.999f)
        {
            for (int ch = 0; ch < numCh; ++ch)
            {
                buffer.applyGain (ch, 0, n, mix);
                buffer.addFrom (ch, 0, dryBuffer, ch, 0, n, 1.0f - mix);
            }
        }
    }

    // ---- Saturador ----
    if (param ("satOn") > 0.5f)
    {
        const float mix = param ("satMix") * 0.01f;

        if (mix > 0.0001f)
        {
            const float drive = dbToGain (param ("satDrive"));
            const float comp  = 1.0f / std::sqrt (drive);

            for (int ch = 0; ch < numCh; ++ch)
            {
                auto* d = buffer.getWritePointer (ch);

                for (int i = 0; i < n; ++i)
                {
                    const float x = d[i];
                    const float w = std::tanh (x * drive) * comp;
                    d[i] = x + mix * (w - x);
                }
            }
        }
    }

    // ---- Delay ----
    if (param ("dlyOn") > 0.5f)
    {
        const float mix = param ("dlyMix") * 0.01f;
        const float fb  = param ("dlyFeedback") * 0.01f;

        if (mix > 0.0001f)
        {
            const float target = juce::jlimit (2.0f, (float) (maxDelaySamples - 2),
                                               param ("dlyTime") * 0.001f * (float) sr);
            delaySmoothed.setTargetValue (target);

            const int chans = juce::jmin (numCh, 2);

            for (int i = 0; i < n; ++i)
            {
                const float d = delaySmoothed.getNextValue();

                for (int ch = 0; ch < chans; ++ch)
                {
                    auto* data = buffer.getWritePointer (ch);
                    const float in  = data[i];
                    const float del = delayLine.popSample (ch, d, true);

                    delayLine.pushSample (ch, in + del * fb);
                    data[i] = in + mix * del;
                }
            }
        }
        else
        {
            // sin mezcla: vaciamos la linea para que no suene una cola vieja al subir la mezcla
            delaySmoothed.setCurrentAndTargetValue (param ("dlyTime") * 0.001f * (float) sr);
            delayLine.reset();
        }
    }

    // ---- Reverb ----
    if (param ("revOn") > 0.5f)
    {
        const float mix = param ("revMix") * 0.01f;

        juce::Reverb::Parameters p;
        p.roomSize   = param ("revSize") * 0.01f;
        p.damping    = param ("revDamp") * 0.01f;
        p.wetLevel   = mix * 0.33f;           // JUCE multiplica el wet por 3
        p.dryLevel   = (1.0f - mix) * 0.5f;   // y el dry por 2
        p.width      = 1.0f;
        p.freezeMode = 0.0f;
        reverb.setParameters (p);

        if (numCh >= 2)
            reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
        else
            reverb.processMono (buffer.getWritePointer (0), n);
    }

    // ---- Ganancia de salida ----
    {
        const float g = dbToGain (param ("outGain"));
        buffer.applyGainRamp (0, n, prevOutGain, g);
        prevOutGain = g;
    }

    // ---- Limitador ----
    if (param ("limOn") > 0.5f)
    {
        limiter.setThreshold (param ("limCeiling"));
        limiter.setRelease (100.0f);
        limiter.process (ctx);
    }
}

//==============================================================================
juce::AudioProcessorEditor* SantiStripProcessor::createEditor()
{
    return new SantiStripEditor (*this);
}

void SantiStripProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SantiStripProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SantiStripProcessor();
}
