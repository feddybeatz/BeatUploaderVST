#pragma once
#include <JuceHeader.h>

namespace ToxicColours
{
    inline juce::Colour background() { return juce::Colour::fromRGB (5, 8, 6); }
    inline juce::Colour card()       { return juce::Colour::fromRGB (13, 24, 18); }
    inline juce::Colour field()      { return juce::Colour::fromRGB (6, 11, 8); }
    inline juce::Colour raised()     { return juce::Colour::fromRGB (22, 40, 30); }
    inline juce::Colour lime()       { return juce::Colour::fromRGB (200, 255, 44); }
    inline juce::Colour violet()     { return juce::Colour::fromRGB (139, 83, 254); }
    inline juce::Colour muted()      { return juce::Colour::fromRGB (127, 145, 129); }
    inline juce::Colour danger()     { return juce::Colour::fromRGB (255, 99, 99); }
}

class ToxicPulseLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ToxicPulseLookAndFeel()
    {
        using namespace ToxicColours;

        setColour (juce::ResizableWindow::backgroundColourId, background());
        setColour (juce::Label::textColourId, lime());

        setColour (juce::TextEditor::backgroundColourId, field());
        setColour (juce::TextEditor::textColourId, juce::Colours::white);
        setColour (juce::TextEditor::outlineColourId, violet().withAlpha (0.5f));
        setColour (juce::TextEditor::focusedOutlineColourId, lime());
        setColour (juce::TextEditor::highlightColourId, violet().withAlpha (0.6f));
        setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
        setColour (juce::CaretComponent::caretColourId, lime());

        setColour (juce::ComboBox::backgroundColourId, field());
        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::ComboBox::outlineColourId, violet().withAlpha (0.5f));
        setColour (juce::ComboBox::focusedOutlineColourId, lime());
        setColour (juce::ComboBox::arrowColourId, lime());

        setColour (juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB (8, 17, 12));
        setColour (juce::PopupMenu::textColourId, juce::Colours::white);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, violet());
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        setColour (juce::ProgressBar::backgroundColourId, field());
        setColour (juce::ProgressBar::foregroundColourId, lime());
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isMouseOverButton, bool isButtonDown) override
    {
        using namespace ToxicColours;

        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        const float radius = 8.0f;
        const bool primary = (bool) button.getProperties().getWithDefault ("primary", false);
        const bool enabled = button.isEnabled();

        juce::Colour fill, border;

        if (primary && enabled)
        {
            fill = lime();
            if (isButtonDown)           fill = fill.darker (0.25f);
            else if (isMouseOverButton) fill = fill.brighter (0.2f);
            border = fill;
        }
        else
        {
            fill   = raised();
            border = violet().withAlpha (enabled ? 0.7f : 0.3f);

            if (enabled && isButtonDown)           fill = fill.darker (0.3f);
            else if (enabled && isMouseOverButton) { fill = fill.brighter (0.15f); border = lime(); }
        }

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, radius);
        g.setColour (border);
        g.drawRoundedRectangle (bounds, radius, 1.5f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        using namespace ToxicColours;

        const bool primary = (bool) button.getProperties().getWithDefault ("primary", false);
        const bool enabled = button.isEnabled();

        auto colour = (primary && enabled) ? background() : lime();

        g.setColour (colour.withAlpha (enabled ? 1.0f : 0.4f));
        g.setFont (juce::FontOptions (juce::jmin (primary ? 18.0f : 15.0f,
                              (float) button.getHeight() * 0.55f),
                          juce::Font::bold));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (8, 0),
                          juce::Justification::centred, 1);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        using namespace ToxicColours;

        auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);

        g.setColour (field());
        g.fillRoundedRectangle (bounds, 6.0f);

        g.setColour (box.hasKeyboardFocus (true) ? lime() : violet().withAlpha (0.5f));
        g.drawRoundedRectangle (bounds, 6.0f, 1.3f);

        juce::Path arrow;
        arrow.startNewSubPath ((float) width - 22.0f, (float) height * 0.4f);
        arrow.lineTo ((float) width - 16.0f, (float) height * 0.6f);
        arrow.lineTo ((float) width - 10.0f, (float) height * 0.4f);

        g.setColour (lime());
        g.strokePath (arrow, juce::PathStrokeType (2.0f));
    }
};