#pragma once

#include "PluginProcessor.h"
#include "Theme.h"

// Pantallita del master: muestra que tan lejos esta cada banda del balance de referencia
// (barra arriba = sobra, barra abajo = falta), la sonoridad medida y la ganancia aplicada.
class MasterDisplay : public juce::Component, private juce::Timer
{
public:
    explicit MasterDisplay (CHETOProcessor& p) : proc (p) { startTimerHz (15); }
    ~MasterDisplay() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        const auto full = getLocalBounds().toFloat();
        g.setColour (Theme::screen);
        g.fillRoundedRectangle (full, 10.0f);
        g.setColour (Theme::ink);
        g.drawRoundedRectangle (full.reduced (1.5f), 10.0f, 3.0f);

        const float lufs = proc.getMasterLufs();
        const bool hasData = lufs > -65.0f;

        g.setFont (Theme::body (12.0f, true));
        g.setColour (Theme::cream);
        g.drawText (hasData ? juce::String (lufs, 1) + " LUFS" : juce::String ("dale play..."),
                    juce::Rectangle<int> (12, 6, 130, 16), juce::Justification::centredLeft);

        const float gain = proc.getMasterGain();
        g.setColour (gain >= 0.0f ? Theme::mint : Theme::orange);
        g.drawText ((gain >= 0.0f ? "+" : "") + juce::String (gain, 1) + " dB",
                    juce::Rectangle<int> (getWidth() - 112, 6, 100, 16), juce::Justification::centredRight);

        auto area = full.reduced (12.0f, 0.0f).withTrimmedTop (28.0f).withTrimmedBottom (18.0f);
        const float midY = area.getCentreY();
        const float slot = area.getWidth() / 10.0f;

        g.setColour (Theme::screenLine);
        g.drawHorizontalLine ((int) midY, area.getX(), area.getRight());
        g.setColour (Theme::screenText.withAlpha (0.6f));
        g.drawHorizontalLine ((int) midY, area.getX(), area.getRight());

        static const char* names[] = { "31", "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k" };
        g.setFont (Theme::body (9.5f));

        for (int i = 0; i < 10; ++i)
        {
            const float d = juce::jlimit (-9.0f, 9.0f, proc.getMasterBandDiff (i));
            const float h = d / 9.0f * (area.getHeight() * 0.5f);
            const float x = area.getX() + slot * (float) i + slot * 0.18f;
            const float w = slot * 0.64f;

            if (hasData)
            {
                g.setColour (d >= 0.0f ? Theme::orange : Theme::teal);
                const juce::Rectangle<float> bar (x, d >= 0.0f ? midY - h : midY, w, std::max (2.0f, std::abs (h)));
                g.fillRoundedRectangle (bar, 3.0f);
            }

            g.setColour (Theme::screenText);
            g.drawText (names[i], juce::Rectangle<float> (area.getX() + slot * (float) i, area.getBottom() + 2.0f, slot, 14.0f).toNearestInt(),
                        juce::Justification::centred);
        }
    }

private:
    void timerCallback() override { repaint(); }

    CHETOProcessor& proc;
};
