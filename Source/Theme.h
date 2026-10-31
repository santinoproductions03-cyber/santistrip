#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Paleta "beige": arena, crema y marron oscuro, con los colores del logo como acentos
namespace Theme
{
    const juce::Colour bgTop      { 0xffeee3cc };
    const juce::Colour bgBottom   { 0xffd9c7a5 };
    const juce::Colour panelTop   { 0xfffaf3e2 };
    const juce::Colour panelBot   { 0xffebddbd };
    const juce::Colour cream      { 0xfffaf3e2 };
    const juce::Colour edge       { 0xffbfa982 };
    const juce::Colour ink        { 0xff2b1d12 };   // marron oscuro (contorno y texto)

    const juce::Colour yellow     { 0xfff0b429 };   // remera
    const juce::Colour green      { 0xff3a8f5a };   // gorra
    const juce::Colour mint       { 0xff7fcfa3 };
    const juce::Colour orange     { 0xffe0693b };   // parche
    const juce::Colour teal       { 0xff37a39c };   // parche
    const juce::Colour sky        { 0xff6a9fe0 };
    const juce::Colour violet     { 0xffa57fd6 };
    const juce::Colour pink       { 0xffec7aa4 };
    const juce::Colour peach      { 0xfff0a67a };

    const juce::Colour text       { 0xff2b1d12 };
    const juce::Colour dim        { 0xff7a6448 };

    // pantalla oscura del EQ
    const juce::Colour screen     { 0xff2a1f16 };
    const juce::Colour screenLine { 0xff54402e };
    const juce::Colour screenText { 0xffc9b392 };

    inline juce::Colour bandColour (int i)
    {
        static const juce::Colour c[] = { orange, yellow, mint, teal, sky, violet, cream, cream };
        return c[juce::jlimit (0, 7, i)];
    }

    // Fuentes del sistema (Windows): Impact para titulos, Bahnschrift para el resto
    inline juce::Font title (float h) { return juce::Font (juce::FontOptions ("Impact", h, juce::Font::plain)); }
    inline juce::Font body  (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions ("Bahnschrift", h, bold ? juce::Font::bold : juce::Font::plain));
    }
}
