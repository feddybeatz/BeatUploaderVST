#pragma once
#include <JuceHeader.h>

namespace ToxicColours
{
    inline juce::Colour background() { return juce::Colour::fromRGB (26, 18, 51); }
    inline juce::Colour card()       { return juce::Colour::fromRGB (38, 27, 77); }
    inline juce::Colour field()      { return juce::Colour::fromRGB (18, 11, 38); }
    inline juce::Colour raised()     { return juce::Colour::fromRGB (51, 37, 106); }
    inline juce::Colour border()     { return juce::Colour::fromRGB (75, 58, 148); }
    inline juce::Colour dashed()     { return juce::Colour::fromRGB (122, 102, 201); }
    inline juce::Colour ink()        { return juce::Colour::fromRGB (26, 18, 51); }
    inline juce::Colour muted()      { return juce::Colour::fromRGB (201, 191, 234); }
    inline juce::Colour violetText() { return juce::Colour::fromRGB (196, 169, 255); }
    inline juce::Colour violet()     { return juce::Colour::fromRGB (176, 124, 255); }
    inline juce::Colour lime()       { return juce::Colour::fromRGB (212, 255, 58); }
    inline juce::Colour cyan()       { return juce::Colour::fromRGB (61, 232, 255); }
    inline juce::Colour pink()       { return juce::Colour::fromRGB (255, 92, 200); }
    inline juce::Colour orange()     { return juce::Colour::fromRGB (255, 122, 31); }
    inline juce::Colour green()      { return juce::Colour::fromRGB (61, 235, 106); }
    inline juce::Colour danger()     { return juce::Colour::fromRGB (255, 107, 107); }
}

namespace ToxicFonts
{
    inline juce::FontOptions ui (float size, bool bold = false)
    {
        return juce::FontOptions ("Segoe UI", size, bold ? juce::Font::bold : juce::Font::plain);
    }
}

class ToxicPulseLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ToxicPulseLookAndFeel()
    {
        using namespace ToxicColours;

        setColour (juce::ResizableWindow::backgroundColourId, background());
        setColour (juce::Label::textColourId, juce::Colours::white);

        setColour (juce::TextEditor::backgroundColourId, field());
        setColour (juce::TextEditor::textColourId, juce::Colours::white);
        setColour (juce::TextEditor::outlineColourId, border());
        setColour (juce::TextEditor::focusedOutlineColourId, violet());
        setColour (juce::TextEditor::highlightColourId, violet().withAlpha (0.55f));
        setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
        setColour (juce::CaretComponent::caretColourId, lime());

        setColour (juce::ScrollBar::thumbColourId, dashed());
    }

    static bool isPrimary (const juce::Button& b) { return (bool) b.getProperties().getWithDefault ("primary", false); }
    static bool isLink    (const juce::Button& b) { return (bool) b.getProperties().getWithDefault ("link", false); }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isMouseOverButton, bool isButtonDown) override
    {
        using namespace ToxicColours;

        if (isLink (button))
            return;

        auto bounds = button.getLocalBounds().toFloat().reduced (1.5f);
        const float radius = 12.0f;
        const bool enabled = button.isEnabled();

        if (isPrimary (button))
        {
            if (enabled)
            {
                juce::DropShadow (lime().withAlpha (0.35f), 16, { 0, 5 }).drawForRectangle (g, bounds.toNearestInt());

                auto fill = lime();
                if (isButtonDown)           fill = fill.darker (0.2f);
                else if (isMouseOverButton) fill = fill.brighter (0.15f);
                g.setColour (fill);
            }
            else
            {
                g.setColour (raised());
            }

            g.fillRoundedRectangle (bounds, radius);
            return;
        }

        auto fill = raised();
        if (enabled && isButtonDown)           fill = fill.darker (0.25f);
        else if (enabled && isMouseOverButton) fill = fill.brighter (0.12f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, radius);
        g.setColour (enabled ? (isMouseOverButton ? lime() : violet()) : border());
        g.drawRoundedRectangle (bounds, radius, 1.5f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool isMouseOverButton, bool) override
    {
        using namespace ToxicColours;

        const bool enabled = button.isEnabled();

        if (isLink (button))
        {
            g.setColour (isMouseOverButton ? juce::Colours::white : violetText());
            g.setFont (ToxicFonts::ui (12.0f, true));
            g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centredRight);
            return;
        }

        const bool primary = isPrimary (button);
        auto colour = (primary && enabled) ? ink() : lime();

        g.setColour (colour.withAlpha (enabled ? 1.0f : 0.4f));
        g.setFont (ToxicFonts::ui (juce::jmin (primary ? 18.0f : 15.0f, (float) button.getHeight() * 0.42f), true));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (8, 0),
                          juce::Justification::centred, 1);
    }
};