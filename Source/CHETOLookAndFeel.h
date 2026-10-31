#pragma once
#include "Theme.h"

// Estilo "sticker": contornos marrones gruesos, sombras duras y colores calidos
class CHETOLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CHETOLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, Theme::cream);
        setColour (juce::ComboBox::textColourId, Theme::ink);
        setColour (juce::ComboBox::outlineColourId, Theme::ink);
        setColour (juce::ComboBox::arrowColourId, Theme::ink);
        setColour (juce::PopupMenu::backgroundColourId, Theme::cream);
        setColour (juce::PopupMenu::textColourId, Theme::ink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::yellow);
        setColour (juce::PopupMenu::highlightedTextColourId, Theme::ink);
        setColour (juce::Label::textColourId, Theme::ink);
        setColour (juce::Slider::textBoxTextColourId, Theme::ink);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    }

    juce::Font getLabelFont (juce::Label&) override        { return Theme::body (12.5f, true); }
    juce::Font getComboBoxFont (juce::ComboBox&) override  { return Theme::body (14.0f, true); }
    juce::Font getPopupMenuFont() override                 { return Theme::body (14.0f, true); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider& slider) override
    {
        const bool big = (bool) slider.getProperties().getWithDefault ("big", false);
        auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        const float diameter = juce::jmin (area.getWidth(), area.getHeight());
        auto b = area.withSizeKeepingCentre (diameter, diameter).reduced (big ? 6.0f : 3.0f);
        const float r = b.getWidth() * 0.5f;
        const auto c = b.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        const float lw = big ? 14.0f : 6.0f;
        const float arcR = r - lw * 0.5f - 2.0f;

        if (big)
        {
            juce::ColourGradient glow (accent.withAlpha (0.10f + 0.50f * pos), c.x, c.y,
                                       accent.withAlpha (0.0f), c.x + r * 1.1f, c.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (c.x - r * 1.1f, c.y - r * 1.1f, r * 2.2f, r * 2.2f);
        }

        // pista: contorno marron + fondo arena + arco de color
        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (Theme::ink);
        g.strokePath (track, juce::PathStrokeType (lw + 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xffd8c8a6));
        g.strokePath (track, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (pos > 0.001f)
        {
            juce::Path value;
            value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
            g.setColour (accent);
            g.strokePath (value, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // cuerpo de la perilla: crema, con sombra dura
        const float br = arcR - lw * 0.5f - (big ? 8.0f : 4.0f);
        g.setColour (Theme::ink);
        g.fillEllipse (c.x - br + 2.0f, c.y - br + 3.0f, br * 2.0f, br * 2.0f);
        juce::ColourGradient bodyGrad (juce::Colour (0xfffffaf0), c.x, c.y - br,
                                       juce::Colour (0xffe2d3b0), c.x, c.y + br, false);
        g.setGradientFill (bodyGrad);
        g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
        g.setColour (Theme::ink);
        g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, big ? 3.5f : 2.5f);

        const float s = std::sin (angle), co = std::cos (angle);
        g.setColour (Theme::ink);
        g.drawLine (c.x + s * br * 0.40f, c.y - co * br * 0.40f,
                    c.x + s * br * 0.86f, c.y - co * br * 0.86f, big ? 6.0f : 3.5f);

        if (big)
        {
            g.setColour (Theme::ink);
            g.setFont (Theme::title (br * 0.62f));
            g.drawText (juce::String (juce::roundToInt ((float) slider.getValue())) + "%",
                        juce::Rectangle<float> (c.x - br, c.y + br * 0.02f, br * 2.0f, br * 0.8f).toNearestInt(),
                        juce::Justification::centred);
        }
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& bt, bool highlighted, bool) override
    {
        auto r = bt.getLocalBounds().toFloat();
        const bool on = bt.getToggleState();
        const auto accent = bt.findColour (juce::ToggleButton::tickColourId);

        if (bt.getButtonText().isNotEmpty())   // boton tipo "chip"
        {
            r = r.reduced (1.5f);
            const float rad = r.getHeight() * 0.5f;
            g.setColour (on ? accent : Theme::ink.withAlpha (0.12f));
            g.fillRoundedRectangle (r, rad);
            g.setColour (Theme::ink.withAlpha (highlighted ? 1.0f : 0.8f));
            g.drawRoundedRectangle (r, rad, 2.0f);
            g.setColour (on ? (accent.getPerceivedBrightness() > 0.5f ? Theme::ink : Theme::cream)
                            : Theme::ink.withAlpha (0.75f));
            g.setFont (Theme::body (12.5f, true));
            g.drawText (bt.getButtonText(), r.toNearestInt(), juce::Justification::centred);
            return;
        }

        // interruptor de encendido
        auto pill = r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 36.0f), juce::jmin (r.getHeight(), 20.0f));
        const float rad = pill.getHeight() * 0.5f;
        g.setColour (on ? accent : Theme::ink.withAlpha (0.22f));
        g.fillRoundedRectangle (pill, rad);
        g.setColour (Theme::ink.withAlpha (highlighted ? 1.0f : 0.85f));
        g.drawRoundedRectangle (pill, rad, 2.0f);
        const float d = pill.getHeight() - 7.0f;
        const float kx = on ? pill.getRight() - d - 3.5f : pill.getX() + 3.5f;
        g.setColour (Theme::cream);
        g.fillEllipse (kx, pill.getY() + 3.5f, d, d);
        g.setColour (Theme::ink);
        g.drawEllipse (kx, pill.getY() + 3.5f, d, d, 1.5f);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (1.5f);
        g.setColour (Theme::ink);
        g.fillRoundedRectangle (r.translated (1.5f, 2.0f), 8.0f);
        g.setColour (Theme::cream);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (Theme::ink);
        g.drawRoundedRectangle (r, 8.0f, 2.0f);

        juce::Path arrow;
        const float ax = (float) w - 17.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4.5f, ay - 2.5f, ax + 4.5f, ay - 2.5f, ax, ay + 3.5f);
        g.setColour (Theme::ink);
        g.fillPath (arrow);
    }
};
