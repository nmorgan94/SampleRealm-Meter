#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
 * The header's buttons for the meter: the theme, chosen from a menu, the view menu with the
 * panels to show and the reactivity, then reset and capture mode. Mirrored by webui/src/meter/MeterControls.tsx.
 */
class MeterControls : public juce::Component
{
public:
    static constexpr int themeWidth = 104, viewWidth = 56, resetWidth = 56, captureWidth = 74, gap = 4;
    static constexpr int width = themeWidth + viewWidth + resetWidth + captureWidth + 3 * gap;

    MeterControls()
    {
        for (auto* button : { &themeButton, &viewButton, &resetButton, &captureButton })
            addAndMakeVisible (button);

        themeButton.setTitle ("Theme");
    }

    juce::TextButton themeButton, viewButton { "VIEW" }, resetButton { "RESET" }, captureButton { "CAPTURE" };

    void setThemeName (const juce::String& name)   { themeButton.setButtonText (name.toUpperCase()); }

    void resized() override
    {
        auto bounds = getLocalBounds();

        captureButton.setBounds (bounds.removeFromRight (captureWidth));
        bounds.removeFromRight (gap);
        resetButton.setBounds (bounds.removeFromRight (resetWidth));
        bounds.removeFromRight (gap);
        viewButton.setBounds (bounds.removeFromRight (viewWidth));
        bounds.removeFromRight (gap);
        themeButton.setBounds (bounds.removeFromRight (themeWidth));
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterControls)
};
