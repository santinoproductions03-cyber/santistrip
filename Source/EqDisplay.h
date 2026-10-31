#pragma once

#include "PluginProcessor.h"
#include "Theme.h"
#include <functional>
#include <vector>

// Ecualizador grafico estilo Pro-Q: curva, espectro en vivo y puntos arrastrables.
class EqDisplay : public juce::Component, private juce::Timer
{
public:
    std::function<void (int)> onBandSelected;

    explicit EqDisplay (CHETOProcessor& p)
        : proc (p), fft (11),
          window ((size_t) CHETOProcessor::scopeSize, juce::dsp::WindowingFunction<float>::hann, false)
    {
        spectrum.fill (-100.0f);
        fftData.fill (0.0f);
        startTimerHz (30);
    }

    ~EqDisplay() override { stopTimer(); }

    void setSelected (int band) { selected = band; repaint(); }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        const auto full = getLocalBounds().toFloat();
        plot = full.reduced (8.0f).withTrimmedLeft (26.0f).withTrimmedBottom (16.0f);

        g.setColour (Theme::screen);
        g.fillRoundedRectangle (full, 10.0f);
        g.setColour (Theme::ink);
        g.drawRoundedRectangle (full.reduced (1.5f), 10.0f, 3.0f);

        // ---- rejilla ----
        g.setFont (Theme::body (10.5f));
        const float freqs[]  = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        const char* names[]  = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

        for (int i = 0; i < 10; ++i)
        {
            const float x = freqToX (freqs[i]);
            g.setColour (Theme::screenLine);
            g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
            g.setColour (Theme::screenText);
            g.drawText (names[i], (int) x - 16, (int) plot.getBottom() + 1, 32, 14, juce::Justification::centred);
        }

        for (int db = -18; db <= 18; db += 6)
        {
            const float y = gainToY ((float) db);
            g.setColour (db == 0 ? Theme::screenText.withAlpha (0.55f) : Theme::screenLine);
            g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
            g.setColour (Theme::screenText);
            g.drawText (db > 0 ? "+" + juce::String (db) : juce::String (db),
                        2, (int) y - 7, 24, 14, juce::Justification::centredRight);
        }

        g.saveState();
        g.reduceClipRegion (plot.toNearestInt());

        // ---- espectro ----
        {
            const float sr = (float) proc.getCurrentSampleRate();
            juce::Path sp;
            sp.startNewSubPath (plot.getX(), plot.getBottom());

            for (float x = plot.getX(); x <= plot.getRight(); x += 2.0f)
            {
                const float f = xToFreq (x);
                const float bin = juce::jlimit (0.0f, (float) numBins - 2.0f, f * (float) CHETOProcessor::scopeSize / sr);
                const int b0 = (int) bin;
                const float db = spectrum[(size_t) b0] + (bin - (float) b0) * (spectrum[(size_t) b0 + 1] - spectrum[(size_t) b0]);
                const float y = plot.getBottom() - plot.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 80.0f);
                sp.lineTo (x, y);
            }

            sp.lineTo (plot.getRight(), plot.getBottom());
            sp.closeSubPath();

            g.setGradientFill (juce::ColourGradient (Theme::mint.withAlpha (0.38f), 0.0f, plot.getY(),
                                                     Theme::green.withAlpha (0.04f), 0.0f, plot.getBottom(), false));
            g.fillPath (sp);
        }

        // ---- curva del EQ ----
        const bool eqActive = val ("eqOn") > 0.5f;
        const double sr = proc.getCurrentSampleRate();
        const int step = 2;
        const int np = (int) (plot.getWidth() / (float) step) + 1;
        std::vector<double> fr ((size_t) np), mags ((size_t) np, 1.0), tmp ((size_t) np);

        for (int i = 0; i < np; ++i)
            fr[(size_t) i] = (double) xToFreq (plot.getX() + (float) (i * step));

        auto multiply = [&] (const CHETOProcessor::Coeffs::Ptr& c)
        {
            c->getMagnitudeForFrequencyArray (fr.data(), tmp.data(), (size_t) np, sr);
            for (size_t i = 0; i < (size_t) np; ++i) mags[i] *= tmp[i];
        };

        if (val ("hpOn") > 0.5f) multiply (CHETOProcessor::makeCutCoeffs (true,  sr, val ("hpFreq")));
        if (val ("lpOn") > 0.5f) multiply (CHETOProcessor::makeCutCoeffs (false, sr, val ("lpFreq")));

        for (int b = 0; b < CHETOProcessor::numBands; ++b)
            if (val (bandId (b, "On")) > 0.5f)
                multiply (CHETOProcessor::makeBandCoeffs (b, sr, val (bandId (b, "Freq")), val (bandId (b, "Q")), val (bandId (b, "Gain"))));

        juce::Path curve, fill;
        for (int i = 0; i < np; ++i)
        {
            const float db = 20.0f * std::log10 (juce::jmax (1.0e-6f, (float) mags[(size_t) i]));
            const float y = juce::jlimit (plot.getY() - 4.0f, plot.getBottom() + 4.0f, gainToY (db));
            const float x = plot.getX() + (float) (i * step);
            if (i == 0) { curve.startNewSubPath (x, y); fill.startNewSubPath (x, plot.getCentreY()); fill.lineTo (x, y); }
            else        { curve.lineTo (x, y); fill.lineTo (x, y); }
        }
        fill.lineTo (plot.getRight(), plot.getCentreY());
        fill.closeSubPath();

        const auto lineColour = eqActive ? Theme::yellow : Theme::screenText;
        g.setColour (lineColour.withAlpha (eqActive ? 0.16f : 0.06f));
        g.fillPath (fill);
        g.setColour (lineColour.withAlpha (0.22f));
        g.strokePath (curve, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (lineColour);
        g.strokePath (curve, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.restoreState();

        // ---- puntos ----
        for (int i = 0; i < 8; ++i)
            drawNode (g, i);

        // ---- lectura mientras se arrastra ----
        if (dragNode >= 0)
        {
            const auto p = nodePos (dragNode);
            juce::String t = freqLabel (val (dragNode < 6 ? bandId (dragNode, "Freq") : (dragNode == 6 ? "hpFreq" : "lpFreq")));
            if (dragNode < 6)
                t << "   " << juce::String (val (bandId (dragNode, "Gain")), 1) << " dB   Q " << juce::String (val (bandId (dragNode, "Q")), 2);

            auto r = juce::Rectangle<float> (p.x - 80.0f, p.y - 40.0f, 160.0f, 20.0f);
            r = r.withPosition (juce::jlimit (plot.getX(), plot.getRight() - 160.0f, r.getX()), juce::jmax (plot.getY(), r.getY()));
            g.setColour (Theme::cream);
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (Theme::ink);
            g.setFont (Theme::body (12.0f, true));
            g.drawText (t, r.toNearestInt(), juce::Justification::centred);
        }
    }

    //==========================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        dragNode = hitNode (e.position);
        if (dragNode < 0) return;

        selected = dragNode;
        if (dragNode < 6 && onBandSelected) onBandSelected (dragNode);

        gestures.clear();
        gestures.push_back (par (dragNode < 6 ? bandId (dragNode, "Freq") : (dragNode == 6 ? "hpFreq" : "lpFreq")));
        if (dragNode < 6) gestures.push_back (par (bandId (dragNode, "Gain")));
        for (auto* p : gestures) p->beginChangeGesture();
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragNode < 0) return;

        const float x = juce::jlimit (plot.getX(), plot.getRight(), e.position.x);
        const float y = juce::jlimit (plot.getY(), plot.getBottom(), e.position.y);

        setReal (dragNode < 6 ? bandId (dragNode, "Freq") : (dragNode == 6 ? "hpFreq" : "lpFreq"), xToFreq (x));
        if (dragNode < 6)
            setReal (bandId (dragNode, "Gain"), std::round (yToGain (y) * 10.0f) / 10.0f);
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        for (auto* p : gestures) p->endChangeGesture();
        gestures.clear();
        dragNode = -1;
        repaint();
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const int h = hitNode (e.position);
        if (h != hoverNode) { hoverNode = h; repaint(); }
        setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const int n = hitNode (e.position);
        if (n < 0) return;

        auto* p = par (n < 6 ? bandId (n, "On") : (n == 6 ? "hpOn" : "lpOn"));
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
        p->endChangeGesture();
        repaint();
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        int n = hitNode (e.position);
        if (n < 0 || n > 5) n = (selected >= 0 && selected < 6) ? selected : -1;
        if (n < 0) return;

        auto* p = par (bandId (n, "Q"));
        const float q = val (bandId (n, "Q")) * std::exp (w.deltaY * 1.2f);
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (q));
        p->endChangeGesture();
        repaint();
    }

private:
    static constexpr int numBins = CHETOProcessor::scopeSize / 2;

    juce::String bandId (int band, const char* suffix) const { return "b" + juce::String (band + 1) + suffix; }
    juce::RangedAudioParameter* par (const juce::String& id) const { return proc.apvts.getParameter (id); }
    float val (const juce::String& id) const { return proc.apvts.getRawParameterValue (id)->load(); }

    void setReal (const juce::String& id, float v)
    {
        auto* p = par (id);
        p->setValueNotifyingHost (p->convertTo0to1 (v));
    }

    static juce::String freqLabel (float f)
    {
        return f >= 1000.0f ? juce::String (f / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz";
    }

    float freqToX (float f) const { return plot.getX() + plot.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); }
    float xToFreq (float x) const { return 20.0f * std::pow (1000.0f, (x - plot.getX()) / plot.getWidth()); }
    float gainToY (float db) const { return plot.getCentreY() - db / 18.0f * (plot.getHeight() * 0.5f); }
    float yToGain (float y) const { return (plot.getCentreY() - y) / (plot.getHeight() * 0.5f) * 18.0f; }

    juce::Point<float> nodePos (int i) const
    {
        if (i < 6) return { freqToX (val (bandId (i, "Freq"))), gainToY (val (bandId (i, "Gain"))) };
        return { freqToX (val (i == 6 ? "hpFreq" : "lpFreq")), plot.getCentreY() };
    }

    int hitNode (juce::Point<float> p) const
    {
        int best = -1;
        float bestD = 14.0f;

        for (int i = 0; i < 8; ++i)
        {
            const float d = nodePos (i).getDistanceFrom (p);
            if (d < bestD) { bestD = d; best = i; }
        }
        return best;
    }

    void drawNode (juce::Graphics& g, int i)
    {
        const auto p = nodePos (i);
        const bool isBand = i < 6;
        const bool on = val (isBand ? bandId (i, "On") : (i == 6 ? "hpOn" : "lpOn")) > 0.5f
                        && val ("eqOn") > 0.5f;
        const auto col = isBand ? Theme::bandColour (i) : Theme::cream;
        const bool hot = (i == selected) || (i == hoverNode) || (i == dragNode);

        if (isBand)
        {
            if (hot)
            {
                g.setColour (col.withAlpha (0.28f));
                g.fillEllipse (p.x - 16.0f, p.y - 16.0f, 32.0f, 32.0f);
            }
            g.setColour (on ? col : Theme::screenLine);
            g.fillEllipse (p.x - 9.0f, p.y - 9.0f, 18.0f, 18.0f);
            g.setColour (on ? Theme::ink : col.withAlpha (0.7f));
            g.drawEllipse (p.x - 9.0f, p.y - 9.0f, 18.0f, 18.0f, 2.0f);
            g.setFont (Theme::body (11.0f, true));
            g.drawText (juce::String (i + 1), juce::Rectangle<float> (p.x - 9.0f, p.y - 9.0f, 18.0f, 18.0f).toNearestInt(),
                        juce::Justification::centred);
        }
        else
        {
            juce::Path d;
            d.addTriangle (p.x - 8.0f, p.y + 7.0f, p.x + 8.0f, p.y + 7.0f, p.x, p.y - 8.0f);
            g.setColour (on ? col : Theme::screenLine);
            g.fillPath (d);
            g.setColour (on ? Theme::ink : Theme::screenText);
            g.strokePath (d, juce::PathStrokeType (2.0f));
            g.setColour (Theme::screenText);
            g.setFont (Theme::body (10.0f, true));
            g.drawText (i == 6 ? "HP" : "LP", (int) p.x - 12, (int) p.y - 24, 24, 12, juce::Justification::centred);
        }
    }

    void timerCallback() override
    {
        proc.readScope (fftData.data());
        std::fill (fftData.begin() + CHETOProcessor::scopeSize, fftData.end(), 0.0f);
        window.multiplyWithWindowingTable (fftData.data(), (size_t) CHETOProcessor::scopeSize);
        fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

        const float sr = (float) proc.getCurrentSampleRate();

        for (int i = 0; i < numBins; ++i)
        {
            float db = juce::Decibels::gainToDecibels (fftData[(size_t) i] / 512.0f, -120.0f);
            const float f = juce::jmax (20.0f, (float) i * sr / (float) CHETOProcessor::scopeSize);
            db += 4.5f * std::log2 (f / 1000.0f);   // inclinacion visual, como Pro-Q
            spectrum[(size_t) i] = spectrum[(size_t) i] * 0.6f + db * 0.4f;
        }

        repaint();
    }

    CHETOProcessor& proc;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, 2 * CHETOProcessor::scopeSize> fftData;
    std::array<float, CHETOProcessor::scopeSize / 2 + 1> spectrum;

    juce::Rectangle<float> plot;
    int selected = 0, hoverNode = -1, dragNode = -1;
    std::vector<juce::RangedAudioParameter*> gestures;
};
