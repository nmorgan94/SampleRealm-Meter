#pragma once

#include <editor/editor.h>
#include "../../shared/Themes.h"

//==============================================================================
/** Where the custom theme's editor puts its parts. As webui/src/meter/customThemeLayout.ts, with the same tests. */
namespace CustomThemeLayout
{
    // Short enough for the editor at its shortest
    inline constexpr int panelWidth = 560, panelHeight = 188;

    inline constexpr int numAccents = 3;

    // A swatch is its colour over its name
    inline constexpr int swatchNameHeight = 12, swatchGap = 4;

    struct Layout
    {
        juce::Rectangle<int> title, close;
        juce::Rectangle<int> baseLabel, base, coloursLabel;
        std::array<juce::Rectangle<int>, numAccents> swatches;
        juce::Rectangle<int> hex;
        juce::Rectangle<int> square, hue;   // the picker: saturation across and brightness up, over the hues
    };

    inline Layout layOut (juce::Rectangle<int> panel)
    {
        Layout layout;
        auto area = panel.reduced (16, 12);

        layout.title = area.removeFromTop (24);
        layout.close = layout.title.removeFromRight (70);
        area.removeFromTop (8);

        auto left = area.removeFromLeft (250);
        area.removeFromLeft (20);

        layout.hue = area.removeFromBottom (16);
        area.removeFromBottom (8);
        layout.square = area;

        layout.baseLabel = left.removeFromTop (12);
        layout.base = left.removeFromTop (24);
        left.removeFromTop (8);
        layout.coloursLabel = left.removeFromTop (12);

        auto row = left.removeFromTop (44);

        for (auto& swatch : layout.swatches)
        {
            swatch = row.removeFromLeft (78);
            row.removeFromLeft (8);
        }

        left.removeFromTop (8);
        layout.hex = left.removeFromTop (24);
        return layout;
    }
}

//==============================================================================
/**
 * The custom theme's editor, over the meter: the built-in theme it starts from and its three
 * accent colours, each set with the picker or as hex. Mirrored by webui/src/meter/CustomThemeEditor.tsx.
 */
class CustomThemeEditor : public ModalOverlay
{
public:
    enum Accent { primary, secondary, hot };

    CustomThemeEditor (std::function<Themes::Custom()> themeGetter, std::function<void (const Themes::Custom&)> themeSetter);

    void show();

    /** Shows the theme as it is now: after an edit, or a look loading under the editor. */
    void refresh();

    /** The accent the picker and the hex field edit. */
    void select (Accent accent);

    void setHue (float hue);
    void setSaturationAndValue (float saturation, float value);

    /** As typed into the hex field. Anything that isn't a colour is ignored. */
    void setHex (const juce::String& hex);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

    CustomThemeLayout::Layout getLayout() const   { return CustomThemeLayout::layOut (getPanelBounds()); }

private:
    static constexpr std::array<const char*, CustomThemeLayout::numAccents> accentNames { "MAIN", "ACCENT", "ALERT" };

    std::function<Themes::Custom()> getTheme;
    std::function<void (const Themes::Custom&)> setTheme;

    juce::TextButton closeButton { "Close" }, baseButton;
    juce::Label hexLabel;

    Themes::Custom custom;
    Accent selected = primary;

    // The picker's own, as a grey or a black has no hue to read back
    float hue = 0.0f, saturation = 0.0f, value = 0.0f;
    bool draggingHue = false;

    juce::Rectangle<int> getPanelBounds() const override
    {
        return getLocalBounds().withSizeKeepingCentre (CustomThemeLayout::panelWidth, juce::jmin (CustomThemeLayout::panelHeight, getHeight()));
    }

    std::uint32_t& accentOf (Themes::Custom& theme, Accent accent) const;
    void pick (juce::Point<float> position);
    void applyPicked();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomThemeEditor)
};
