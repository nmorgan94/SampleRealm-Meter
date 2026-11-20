#include "CustomThemeEditor.h"
#include "MeterLook.h"
#include "../../shared/MeterScales.h"

CustomThemeEditor::CustomThemeEditor (std::function<Themes::Custom()> themeGetter, std::function<void (const Themes::Custom&)> themeSetter)
    : getTheme (std::move (themeGetter)), setTheme (std::move (themeSetter))
{
    titleLabel.setText ("Custom theme", juce::dontSendNotification);
    closeButton.onClick = [this] { dismiss(); };

    // To the next built-in theme, and round to the first again
    baseButton.onClick = [this]
    {
        custom.base = (custom.base + 1) % Themes::customIndex;
        setTheme (custom);
        refresh();
    };

    hexLabel.setEditable (true);
    hexLabel.setTitle ("Hex colour");
    hexLabel.setFont (juce::FontOptions (12.0f));
    hexLabel.onTextChange = [this] { setHex (hexLabel.getText()); };

    addAndMakeVisible (closeButton);
    addAndMakeVisible (baseButton);
    addAndMakeVisible (hexLabel);
}

std::uint32_t& CustomThemeEditor::accentOf (Themes::Custom& theme, Accent accent) const
{
    return accent == primary ? theme.primary : accent == secondary ? theme.secondary : theme.hot;
}

void CustomThemeEditor::show()
{
    refresh();
    open();
}

void CustomThemeEditor::refresh()
{
    custom = getTheme();
    baseButton.setButtonText ("BASE: " + juce::String (ParameterSpecs::themeNames[static_cast<size_t> (custom.base)]).toUpperCase());

    const auto colour = MeterTheme::opaque (accentOf (custom, selected));
    hexLabel.setText ("#" + juce::String (Themes::toHex (accentOf (custom, selected))).toUpperCase(), juce::dontSendNotification);

    // Left alone where it already shows the colour, which keeps the hue of a grey
    if (juce::Colour::fromHSV (hue, saturation, value, 1.0f) != colour)
        colour.getHSB (hue, saturation, value);

    repaint();
}

void CustomThemeEditor::select (Accent accent)
{
    selected = accent;
    refresh();
}

void CustomThemeEditor::setHue (float newHue)
{
    hue = juce::jlimit (0.0f, 1.0f, newHue);
    applyPicked();
}

void CustomThemeEditor::setSaturationAndValue (float newSaturation, float newValue)
{
    saturation = juce::jlimit (0.0f, 1.0f, newSaturation);
    value = juce::jlimit (0.0f, 1.0f, newValue);
    applyPicked();
}

void CustomThemeEditor::applyPicked()
{
    accentOf (custom, selected) = juce::Colour::fromHSV (hue, saturation, value, 1.0f).getARGB() & 0xffffff;
    setTheme (custom);
    refresh();
}

void CustomThemeEditor::setHex (const juce::String& hex)
{
    if (const auto colour = Themes::fromHex (hex.trim().toStdString()))
    {
        accentOf (custom, selected) = *colour;
        setTheme (custom);
    }

    refresh();
}

//==============================================================================
void CustomThemeEditor::resized()
{
    const auto layout = getLayout();

    titleLabel.setBounds (layout.title);
    closeButton.setBounds (layout.close);
    baseButton.setBounds (layout.base);
    hexLabel.setBounds (layout.hex);
}

void CustomThemeEditor::paint (juce::Graphics& g)
{
    ModalOverlay::paint (g);

    const auto layout = getLayout();
    const auto text = findColour (juce::Label::textColourId);
    const auto outline = findColour (outlineColourId);

    g.setColour (text.withAlpha (0.6f));
    g.setFont (EditorFonts::of (*this, 9.0f, false));
    g.drawText ("STARTS FROM", layout.baseLabel, juce::Justification::centredLeft);
    g.drawText ("COLOURS", layout.coloursLabel, juce::Justification::centredLeft);

    for (size_t i = 0; i < layout.swatches.size(); ++i)
    {
        auto swatch = layout.swatches[i];
        const auto name = swatch.removeFromBottom (CustomThemeLayout::swatchNameHeight);
        const auto block = swatch.withTrimmedBottom (CustomThemeLayout::swatchGap).toFloat();
        const auto isSelected = i == static_cast<size_t> (selected);

        g.setColour (MeterTheme::opaque (accentOf (custom, static_cast<Accent> (i))));
        g.fillRoundedRectangle (block, 4.0f);

        g.setColour (isSelected ? text : outline);
        g.drawRoundedRectangle (block.reduced (0.5f), 4.0f, isSelected ? 2.0f : 1.0f);

        g.setColour (text.withAlpha (isSelected ? 1.0f : 0.6f));
        g.drawText (accentNames[i], name, juce::Justification::centred);
    }

    // The hex field's box, as the prompt's text field has
    g.setColour (findColour (juce::ResizableWindow::backgroundColourId));
    g.fillRoundedRectangle (layout.hex.toFloat(), 3.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (layout.hex.toFloat().reduced (0.5f), 3.0f, 1.0f);

    // The hue at full strength, whitened towards the left and darkened towards the bottom
    const auto square = layout.square.toFloat();
    g.setColour (juce::Colour::fromHSV (hue, 1.0f, 1.0f, 1.0f));
    g.fillRect (square);
    g.setGradientFill (juce::ColourGradient::horizontal (juce::Colours::white, square.getX(), juce::Colours::white.withAlpha (0.0f), square.getRight()));
    g.fillRect (square);
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colours::black.withAlpha (0.0f), square.getY(), juce::Colours::black, square.getBottom()));
    g.fillRect (square);

    const auto strip = layout.hue.toFloat();
    juce::ColourGradient hues (juce::Colours::red, strip.getX(), 0.0f, juce::Colours::red, strip.getRight(), 0.0f, false);

    // Red, yellow, green, cyan, blue, magenta and red again
    for (int i = 1; i < 6; ++i)
        hues.addColour (i / 6.0, juce::Colour::fromHSV (static_cast<float> (i) / 6.0f, 1.0f, 1.0f, 1.0f));

    g.setGradientFill (hues);
    g.fillRect (strip);

    g.setColour (outline);
    g.drawRect (square, 1.0f);
    g.drawRect (strip, 1.0f);

    // Where the colour is in each
    const juce::Point<float> spot (square.getX() + square.getWidth() * saturation, square.getY() + square.getHeight() * (1.0f - value));
    g.setColour (juce::Colours::black);
    g.drawEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (spot), 1.0f);
    g.setColour (juce::Colours::white);
    g.drawEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (spot), 1.5f);

    const auto mark = juce::Rectangle<float> (4.0f, strip.getHeight() + 4.0f).withCentre ({ strip.getX() + strip.getWidth() * hue, strip.getCentreY() });
    g.setColour (juce::Colours::black);
    g.drawRect (mark.expanded (1.0f), 1.0f);
    g.setColour (juce::Colours::white);
    g.drawRect (mark, 1.5f);
}

//==============================================================================
void CustomThemeEditor::mouseDown (const juce::MouseEvent& event)
{
    const auto layout = getLayout();
    const auto position = event.getPosition();

    // A drag that starts in one part of the picker stays with it
    draggingHue = layout.hue.expanded (0, 4).contains (position);

    for (size_t i = 0; i < layout.swatches.size(); ++i)
        if (layout.swatches[i].contains (position))
            return select (static_cast<Accent> (i));

    if (draggingHue || layout.square.contains (position))
        pick (event.position);
}

void CustomThemeEditor::mouseDrag (const juce::MouseEvent& event)
{
    const auto layout = getLayout();

    if (draggingHue || layout.square.contains (event.getMouseDownPosition()))
        pick (event.position);
}

void CustomThemeEditor::pick (juce::Point<float> position)
{
    using MeterScales::proportion;

    const auto layout = getLayout();

    if (draggingHue)
        return setHue (proportion (position.x, static_cast<float> (layout.hue.getX()), static_cast<float> (layout.hue.getRight())));

    const auto square = layout.square.toFloat();
    setSaturationAndValue (proportion (position.x, square.getX(), square.getRight()), 1.0f - proportion (position.y, square.getY(), square.getBottom()));
}
