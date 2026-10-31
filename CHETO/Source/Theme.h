#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace Theme
{
    const juce::Colour bgTop    { 0xff0a1a12 };
    const juce::Colour bgBottom { 0xff143323 };
    const juce::Colour panelTop { 0xff1d3c2a };
    const juce::Colour panelBot { 0xff122a1d };
    const juce::Colour edge     { 0xff2c5a3f };
    const juce::Colour ink      { 0xff1a0f0a };
    const juce::Colour yellow   { 0xfff7c531 };
    const juce::Colour green    { 0xff2fa35a };
    const juce::Colour mint     { 0xff6ff0a0 };
    const juce::Colour orange   { 0xffef6f45 };
    const juce::Colour teal     { 0xff3fc1c0 };
    const juce::Colour sky      { 0xff6aa9ff };
    const juce::Colour violet   { 0xffc47bff };
    const juce::Colour pink     { 0xffff6fb5 };
    const juce::Colour text     { 0xfff4f0e1 };
    const juce::Colour dim      { 0xff8fb09a };

    inline juce::Colour bandColour (int i)
    {
        static const juce::Colour c[] = { orange, yellow, mint, teal, sky, violet, dim, dim };
        return c[juce::jlimit (0, 7, i)];
    }

    // Fuentes del sistema (Windows): Impact para titulos, Bahnschrift para el resto
    inline juce::Font title (float h) { return juce::Font (juce::FontOptions ("Impact", h, juce::Font::plain)); }
    inline juce::Font body  (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions ("Bahnschrift", h, bold ? juce::Font::bold : juce::Font::plain));
    }
}
