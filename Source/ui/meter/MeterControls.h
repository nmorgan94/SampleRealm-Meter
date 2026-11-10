#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
 * The header's buttons for the meter: the theme and the reactivity, which step to the next
 * choice on each click, then reset and capture mode. Mirrored by webui/src/meter/MeterControls.tsx.
 */
class MeterControls : public juce::Component
{
public:
    static constexpr int themeWidth = 104, reactivityWidth = 84, resetWidth = 56, captureWidth = 74, gap = 4;
    static constexpr int width = themeWidth + reactivityWidth + resetWidth + captureWidth + 3 * gap;

    MeterControls()
    {
        for (auto* button : { &themeButton, &reactivityButton, &resetButton, &captureButton })
            addAndMakeVisible (button);

        themeButton.setTitle ("Theme");
        reactivityButton.setTitle ("Reactivity");
    }

    juce::TextButton themeButton, reactivityButton, resetButton { "RESET" }, captureButton { "CAPTURE" };

    void setThemeName (const juce::String& name)        { themeButton.setButtonText (name.toUpperCase()); }
    void setReactivityName (const juce::String& name)   { reactivityButton.setButtonText ("FX " + name.toUpperCase()); }

    void resized() override
    {
        auto bounds = getLocalBounds();

        captureButton.setBounds (bounds.removeFromRight (captureWidth));
        bounds.removeFromRight (gap);
        resetButton.setBounds (bounds.removeFromRight (resetWidth));
        bounds.removeFromRight (gap);
        reactivityButton.setBounds (bounds.removeFromRight (reactivityWidth));
        bounds.removeFromRight (gap);
        themeButton.setBounds (bounds.removeFromRight (themeWidth));
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterControls)
};
