#pragma once

#include "PresetActions.h"
#include "PresetBar.h"

//==============================================================================
/**
 * The strip along the top of the editor: the title, the preset bar, the menu and the plugin's own controls.
 * Mirrored by webui/src/HeaderBar.tsx.
 */
class HeaderBar : public juce::Component
{
public:
    enum ColourIds
    {
        backgroundColourId = 0x5d10500,
        titleColourId
    };

    /** Each plugin's sizes, as HeaderLayout in webui/src/HeaderBar.tsx. */
    struct Layout
    {
        int height;
        int edge;           // left of the title, right of the controls
        int titleWidth;     // a longer title is squeezed to fit
        float titleHeight;
        int gap;            // either side of the preset bar and menu
        int controlsHeight;
        int trailingWidth;  // the plugin's controls, at the right
        int trailingInset;  // above and below them
    };

    static constexpr float minimumTitleScale = 0.7f;
    static juce::Font titleFont (const Layout&);

    HeaderBar (const juce::String& title, const Layout&, PresetManager&, PromptOverlay&,
               std::function<void()> showManual, const juce::String& aboutText, juce::Component& trailingControls);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    const Layout layout;
    PromptOverlay& prompt;
    const std::function<void()> showManual;
    const juce::String aboutText;

    PresetActions presetActions;
    PresetBar presetBar;
    juce::TextButton menuButton { "..." };
    juce::Component& trailing;

    void showMenu();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderBar)
};
