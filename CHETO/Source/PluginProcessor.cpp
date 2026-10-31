#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;

    inline float dbToGain (float db) { return juce::Decibels::decibelsToGain (db); }

    juce::String freqText (float v)
    {
        return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
    }

    float freqFromText (const juce::String& t)
    {
        float v = t.getFloatValue();
        if (t.containsIgnoreCase ("k")) v *= 1000.0f;
        return v;
    }

    void addFloat (Layout& layout, const juce::String& id, const juce::String& name,
                   float minV, float maxV, float defV, const juce::String& unit,
                   float skewCentre = 0.0f)
    {
        juce::NormalisableRange<float> range (minV, maxV, 0.01f);
        if (skewCentre > 0.0f) range.setSkewForCentre (skewCentre);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, defV,
            juce::AudioParameterFloatAttributes().withLabel (unit)));
    }

    void addFreq (Layout& layout, const juce::String& id, const juce::String& name,
                  float minV, float maxV, float defV, float skewCentre)
    {
        juce::NormalisableRange<float> range (minV, maxV, 0.01f);
        range.setSkewForCentre (skewCentre);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, defV,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int) { return freqText (v); })
                .withValueFromStringFunction ([] (const juce::String& s) { return freqFromText (s); })));
    }

    void addBool (Layout& layout, const juce::String& id, const juce::String& name, bool defV)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, defV));
    }

    void addChoice (Layout& layout, const juce::String& id, const juce::String& name,
                    const juce::StringArray& items, int defIndex)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, defIndex));
    }
}

//==============================================================================
CHETOProcessor::CHETOProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

Layout CHETOProcessor::createLayout()
{
    Layout l;

    addFloat (l, "master", "MEJORAR TODO", 0.0f, 100.0f, 0.0f, " %");
    addFloat (l, "inGain",  "Ganancia de entrada", -24.0f, 24.0f, 0.0f, " dB");
    addFloat (l, "outGain", "Ganancia de salida",  -24.0f, 24.0f, 0.0f, " dB");

    // Autotune
    addBool   (l, "atOn", "Autotune activo", false);
    addChoice (l, "atKey", "Autotune Nota", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0);
    addChoice (l, "atScale", "Autotune Escala", { "Cromatica", "Mayor", "Menor" }, 0);
    addFloat  (l, "atSpeed",  "Autotune Velocidad", 0.0f, 100.0f, 25.0f, " %");
    addFloat  (l, "atAmount", "Autotune Cantidad",  0.0f, 100.0f, 100.0f, " %");

    // EQ
    addBool (l, "eqOn", "EQ activo", true);
    addBool (l, "hpOn", "EQ Corte graves activo", false);
    addFreq (l, "hpFreq", "EQ Corte graves Freq", 20.0f, 500.0f, 30.0f, 100.0f);
    addBool (l, "lpOn", "EQ Corte agudos activo", false);
    addFreq (l, "lpFreq", "EQ Corte agudos Freq", 2000.0f, 20000.0f, 18000.0f, 8000.0f);

    const float defFreq[numBands] = { 90.0f, 250.0f, 900.0f, 2800.0f, 6000.0f, 10000.0f };
    const float defQ[numBands]    = { 0.71f, 1.0f, 1.0f, 1.0f, 1.0f, 0.71f };

    for (int i = 0; i < numBands; ++i)
    {
        const juce::String n (i + 1);
        addBool (l, "b" + n + "On",   "EQ Banda " + n + " activa", true);
        addFreq (l, "b" + n + "Freq", "EQ Banda " + n + " Freq", 20.0f, 20000.0f, defFreq[i], 1000.0f);
        addFloat (l, "b" + n + "Gain", "EQ Banda " + n + " Gain", -18.0f, 18.0f, 0.0f, " dB");
        addFloat (l, "b" + n + "Q",    "EQ Banda " + n + " Q", 0.1f, 10.0f, defQ[i], "", 1.0f);
    }

    // Compresor
    addBool  (l, "compOn", "Compresor activo", true);
    addFloat (l, "cThresh",  "Comp Umbral",   -60.0f,    0.0f, -12.0f, " dB");
    addFloat (l, "cRatio",   "Comp Ratio",      1.0f,   20.0f,   2.0f, ":1", 4.0f);
    addFloat (l, "cAttack",  "Comp Ataque",     0.1f,  100.0f,  15.0f, " ms", 10.0f);
    addFloat (l, "cRelease", "Comp Release",   10.0f, 1000.0f, 150.0f, " ms", 150.0f);
    addFloat (l, "cMakeup",  "Comp Makeup",     0.0f,   24.0f,   0.0f, " dB");
    addFloat (l, "cMix",     "Comp Mezcla",     0.0f,  100.0f, 100.0f, " %");

    // Saturador
    addBool  (l, "satOn", "Saturador activo", true);
    addFloat (l, "satDrive", "Sat Drive",  0.0f, 30.0f, 6.0f, " dB");
    addFloat (l, "satMix",   "Sat Mezcla", 0.0f, 100.0f, 0.0f, " %");

    // Bitcrusher
    addBool  (l, "bcOn", "Bitcrusher activo", true);
    addFloat (l, "bcBits", "Bitcrusher Bits", 1.0f, 16.0f, 16.0f, " bits");
    addFloat (l, "bcRate", "Bitcrusher Reduccion", 1.0f, 40.0f, 1.0f, "x", 4.0f);
    addFloat (l, "bcMix",  "Bitcrusher Mezcla", 0.0f, 100.0f, 0.0f, " %");

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
CHETOProcessor::Coeffs::Ptr CHETOProcessor::makeBandCoeffs (int band, double sr, float f, float q, float gainDb)
{
    f = juce::jlimit (10.0f, (float) (sr * 0.45), f);
    const float g = dbToGain (gainDb);

    if (band == 0)             return Coeffs::makeLowShelf  (sr, f, q, g);
    if (band == numBands - 1)  return Coeffs::makeHighShelf (sr, f, q, g);
    return Coeffs::makePeakFilter (sr, f, q, g);
}

CHETOProcessor::Coeffs::Ptr CHETOProcessor::makeCutCoeffs (bool highPass, double sr, float f)
{
    f = juce::jlimit (10.0f, (float) (sr * 0.45), f);
    return highPass ? Coeffs::makeHighPass (sr, f, 0.707f) : Coeffs::makeLowPass (sr, f, 0.707f);
}

void CHETOProcessor::readScope (float* dest) const
{
    const int w = scopeWrite.load();
    for (int k = 0; k < scopeSize; ++k)
        dest[k] = scopeRing[(size_t) ((w - scopeSize + k) & 4095)];
}

void CHETOProcessor::handleAsyncUpdate()
{
    setLatencySamples (wantedLatency.load());
}

//==============================================================================
bool CHETOProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void CHETOProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    auto prep = [&] (Slot& s) { s.filter.prepare (spec); s.filter.reset(); s.a = s.b = s.c = -1.0e9f; };
    for (auto& s : eq) prep (s);
    prep (hp); prep (lp);
    prep (enhHp); prep (enhLow); prep (enhMud); prep (enhPres); prep (enhAir);

    compressor.prepare (spec);  compressor.reset();
    glue.prepare (spec);        glue.reset();
    limiter.prepare (spec);     limiter.reset();

    maxDelaySamples = (int) sampleRate + 2;
    delayLine.setMaximumDelayInSamples (maxDelaySamples);
    delayLine.prepare (spec);
    delayLine.reset();
    delaySmoothed.reset (sampleRate, 0.1);
    delaySmoothed.setCurrentAndTargetValue (param ("dlyTime") * 0.001f * (float) sampleRate);

    reverb.setSampleRate (sampleRate);
    reverb.reset();

    tuner.prepare (sampleRate);
    lastAutotuneOn = false;
    wantedLatency = 0;
    setLatencySamples (0);

    crushHeld.fill (0.0f);
    crushCount.fill (0.0f);

    dryBuffer.setSize (2, samplesPerBlock, false, false, true);

    prevInGain  = dbToGain (param ("inGain"));
    prevOutGain = dbToGain (param ("outGain"));
    prevMakeup  = dbToGain (param ("cMakeup"));
    prevEnhGain = 1.0f;
}

void CHETOProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();
    const double sr = currentSr;

    for (int ch = getTotalNumInputChannels(); ch < numCh; ++ch)
        buffer.clear (ch, 0, n);

    // Cambia los coeficientes de un filtro solo si cambiaron los parametros
    auto setCoeffs = [] (Slot& s, float a, float b, float c, auto&& make)
    {
        if (a != s.a || b != s.b || c != s.c)
        {
            s.a = a; s.b = b; s.c = c;
            *s.filter.state = *make();
        }
    };

    // ---- Ganancia de entrada ----
    {
        const float g = dbToGain (param ("inGain"));
        buffer.applyGainRamp (0, n, prevInGain, g);
        prevInGain = g;
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // ---- Autotune ----
    {
        const bool on = param ("atOn") > 0.5f;

        if (on != lastAutotuneOn)
        {
            lastAutotuneOn = on;
            if (! on) tuner.reset();
            wantedLatency = on ? AutoTuner::latency : 0;
            triggerAsyncUpdate();
        }

        if (on)
        {
            AutoTuner::Settings s;
            s.key    = (int) param ("atKey");
            s.scale  = (int) param ("atScale");
            s.speed  = param ("atSpeed") * 0.01f;
            s.amount = param ("atAmount") * 0.01f;
            tuner.process (buffer.getArrayOfWritePointers(), numCh, n, s);
        }
    }

    // ---- EQ pro: corte de graves, 6 bandas, corte de agudos ----
    if (param ("eqOn") > 0.5f)
    {
        if (param ("hpOn") > 0.5f)
        {
            const float f = param ("hpFreq");
            setCoeffs (hp, f, 0.0f, 0.0f, [&] { return makeCutCoeffs (true, sr, f); });
            hp.filter.process (ctx);
        }

        for (int i = 0; i < numBands; ++i)
        {
            const juce::String id = "b" + juce::String (i + 1);
            if (param ((id + "On").toRawUTF8()) < 0.5f) continue;

            const float f = param ((id + "Freq").toRawUTF8());
            const float g = param ((id + "Gain").toRawUTF8());
            const float q = param ((id + "Q").toRawUTF8());
            if (std::abs (g) < 0.02f) continue;

            setCoeffs (eq[(size_t) i], f, g, q, [&] { return makeBandCoeffs (i, sr, f, q, g); });
            eq[(size_t) i].filter.process (ctx);
        }

        if (param ("lpOn") > 0.5f)
        {
            const float f = param ("lpFreq");
            setCoeffs (lp, f, 0.0f, 0.0f, [&] { return makeCutCoeffs (false, sr, f); });
            lp.filter.process (ctx);
        }
    }

    // ---- Compresor (con mezcla paralela) ----
    if (param ("compOn") > 0.5f)
    {
        const float mix = param ("cMix") * 0.01f;
        if (mix < 0.999f) dryBuffer.makeCopyOf (buffer, true);

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
                    d[i] = x + mix * (std::tanh (x * drive) * comp - x);
                }
            }
        }
    }

    // ---- Bitcrusher ----
    if (param ("bcOn") > 0.5f)
    {
        const float mix = param ("bcMix") * 0.01f;

        if (mix > 0.0001f)
        {
            const float levels = std::pow (2.0f, param ("bcBits") - 1.0f);
            const float step   = param ("bcRate");

            for (int ch = 0; ch < juce::jmin (numCh, 2); ++ch)
            {
                auto* d = buffer.getWritePointer (ch);
                auto& held = crushHeld[(size_t) ch];
                auto& cnt  = crushCount[(size_t) ch];

                for (int i = 0; i < n; ++i)
                {
                    const float x = d[i];

                    cnt -= 1.0f;
                    if (cnt <= 0.0f)
                    {
                        held = std::round (x * levels) / levels;
                        cnt += step;
                    }

                    d[i] = x + mix * (held - x);
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
        p.wetLevel   = mix * 0.33f;
        p.dryLevel   = (1.0f - mix) * 0.5f;
        p.width      = 1.0f;
        p.freezeMode = 0.0f;
        reverb.setParameters (p);

        if (numCh >= 2) reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
        else            reverb.processMono (buffer.getWritePointer (0), n);
    }

    // ---- MEJORAR TODO: mini masterizacion automatica ----
    {
        const float m = param ("master") * 0.01f;

        if (m > 0.001f)
        {
            setCoeffs (enhHp,   30.0f, 0.0f, 0.0f, [&] { return Coeffs::makeHighPass (sr, 30.0f, 0.707f); });
            setCoeffs (enhLow,  m, 0.0f, 0.0f, [&] { return Coeffs::makeLowShelf   (sr, 110.0f,   0.707f, dbToGain (1.6f * m)); });
            setCoeffs (enhMud,  m, 0.0f, 0.0f, [&] { return Coeffs::makePeakFilter (sr, 280.0f,   0.9f,   dbToGain (-2.6f * m)); });
            setCoeffs (enhPres, m, 0.0f, 0.0f, [&] { return Coeffs::makePeakFilter (sr, 3200.0f,  0.8f,   dbToGain (2.0f * m)); });
            setCoeffs (enhAir,  m, 0.0f, 0.0f, [&] { return Coeffs::makeHighShelf  (sr, 10500.0f, 0.707f, dbToGain (3.8f * m)); });

            enhHp.filter.process (ctx);
            enhLow.filter.process (ctx);
            enhMud.filter.process (ctx);
            enhPres.filter.process (ctx);
            enhAir.filter.process (ctx);

            glue.setThreshold (-10.0f - 8.0f * m);
            glue.setRatio (1.0f + 1.6f * m);
            glue.setAttack (25.0f);
            glue.setRelease (140.0f);
            glue.process (ctx);

            const float drive = dbToGain (3.0f + 3.0f * m);
            const float comp  = 1.0f / std::sqrt (drive);
            const float satMix = 0.22f * m;

            for (int ch = 0; ch < numCh; ++ch)
            {
                auto* d = buffer.getWritePointer (ch);
                for (int i = 0; i < n; ++i)
                {
                    const float x = d[i];
                    d[i] = x + satMix * (std::tanh (x * drive) * comp - x);
                }
            }

            const float g = dbToGain (2.0f * m);
            buffer.applyGainRamp (0, n, prevEnhGain, g);
            prevEnhGain = g;
        }
        else if (prevEnhGain != 1.0f)
        {
            buffer.applyGainRamp (0, n, prevEnhGain, 1.0f);
            prevEnhGain = 1.0f;
        }
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

    // ---- Para el analizador ----
    {
        int w = scopeWrite.load();
        const auto* l = buffer.getReadPointer (0);
        const auto* r = buffer.getReadPointer (numCh > 1 ? 1 : 0);

        for (int i = 0; i < n; ++i)
        {
            scopeRing[(size_t) (w & 4095)] = 0.5f * (l[i] + r[i]);
            ++w;
        }

        scopeWrite.store (w & 4095);
    }
}

//==============================================================================
juce::AudioProcessorEditor* CHETOProcessor::createEditor()
{
    return new CHETOEditor (*this);
}

void CHETOProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void CHETOProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CHETOProcessor();
}
