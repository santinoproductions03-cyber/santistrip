#pragma once
#include "Theme.h"

class CHETOLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CHETOLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, Theme::ink.withAlpha (0.55f));
        setColour (juce::ComboBox::textColourId, Theme::text);
        setColour (juce::ComboBox::outlineColourId, Theme::edge);
        setColour (juce::ComboBox::arrowColourId, Theme::yellow);
        setColour (juce::PopupMenu::backgroundColourId, Theme::panelBot);
        setColour (juce::PopupMenu::textColourId, Theme::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::green);
        setColour (juce::PopupMenu::highlightedTextColourId, Theme::text);
        setColour (juce::Label::textColourId, Theme::text);
        setColour (juce::Slider::textBoxTextColourId, Theme::text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    }

    juce::Font getLabelFont (juce::Label&) override        { return Theme::body (12.5f); }
    juce::Font getComboBoxFont (juce::ComboBox&) override  { return Theme::body (14.0f, true); }
    juce::Font getPopupMenuFont() override                 { return Theme::body (14.0f, true); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider& slider) override
    {
        const bool big = (bool) slider.getProperties().getWithDefault ("big", false);
        auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        const float diameter = juce::jmin (area.getWidth(), area.getHeight());
        auto b = area.withSizeKeepingCentre (diameter, diameter).reduced (big ? 8.0f : 3.0f);
        const float r = b.getWidth() * 0.5f;
        const auto c = b.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        const float lw = big ? 13.0f : 5.0f;

        if (big)
        {
            juce::ColourGradient glow (accent.withAlpha (0.10f + 0.45f * pos), c.x, c.y,
                                       accent.withAlpha (0.0f), c.x + r * 1.15f, c.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (c.x - r * 1.15f, c.y - r * 1.15f, r * 2.3f, r * 2.3f);
        }

        juce::Path track;
        track.addCentredArc (c.x, c.y, r - lw * 0.5f, r - lw * 0.5f, 0.0f, startAngle, endAngle, true);
        g.setColour (Theme::ink.withAlpha (0.7f));
        g.strokePath (track, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (pos > 0.001f)
        {
            juce::Path value;
            value.addCentredArc (c.x, c.y, r - lw * 0.5f, r - lw * 0.5f, 0.0f, startAngle, angle, true);
            g.setColour (accent);
            g.strokePath (value, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float br = r - lw - (big ? 7.0f : 3.0f);
        juce::ColourGradient bodyGrad (juce::Colour (0xff46695a), c.x, c.y - br,
                                       juce::Colour (0xff0f1f17), c.x, c.y + br, false);
        g.setGradientFill (bodyGrad);
        g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
        g.setColour (Theme::ink);
        g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, big ? 3.0f : 2.0f);

        const float s = std::sin (angle), co = std::cos (angle);
        g.setColour (big ? Theme::yellow : Theme::text);
        g.drawLine (c.x + s * br * 0.35f, c.y - co * br * 0.35f,
                    c.x + s * br * 0.88f, c.y - co * br * 0.88f, big ? 5.0f : 3.0f);

        if (big)
        {
            g.setColour (Theme::yellow);
            g.setFont (Theme::title (br * 0.62f));
            g.drawText (juce::String (juce::roundToInt ((float) slider.getValue())) + "%",
                        juce::Rectangle<float> (c.x - br, c.y + br * 0.05f, br * 2.0f, br * 0.8f).toNearestInt(),
                        juce::Justification::centred);
        }
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& bt, bool highlighted, bool) override
    {
        auto r = bt.getLocalBounds().toFloat();
        const bool on = bt.getToggleState();
        const auto accent = bt.findColour (juce::ToggleButton::tickColourId);

        if (bt.getButtonText().isNotEmpty())
        {
            r = r.reduced (1.0f);
            g.setColour (on ? accent : Theme::ink.withAlpha (0.6f));
            g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            g.setColour (highlighted ? Theme::text : (on ? Theme::ink : Theme::edge));
            g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.5f);
            g.setColour (on ? Theme::ink : Theme::dim);
            g.setFont (Theme::body (12.5f, true));
            g.drawText (bt.getButtonText(), r.toNearestInt(), juce::Justification::centred);
            return;
        }

        auto pill = r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 34.0f), juce::jmin (r.getHeight(), 18.0f));
        g.setColour (on ? accent : Theme::ink.withAlpha (0.7f));
        g.fillRoundedRectangle (pill, pill.getHeight() * 0.5f);
        g.setColour (highlighted ? Theme::text : Theme::ink);
        g.drawRoundedRectangle (pill, pill.getHeight() * 0.5f, 1.5f);
        const float d = pill.getHeight() - 6.0f;
        const float kx = on ? pill.getRight() - d - 3.0f : pill.getX() + 3.0f;
        g.setColour (on ? Theme::ink : Theme::dim);
        g.fillEllipse (kx, pill.getY() + 3.0f, d, d);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
        g.setColour (Theme::ink.withAlpha (0.6f));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (box.hasKeyboardFocus (true) ? Theme::yellow : Theme::edge);
        g.drawRoundedRectangle (r, 8.0f, 1.5f);

        juce::Path arrow;
        const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
        g.setColour (Theme::yellow);
        g.fillPath (arrow);
    }
};
