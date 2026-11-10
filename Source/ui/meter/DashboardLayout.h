#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/** Where the dashboard's panels go. As webui/src/meter/dashboardLayout.ts, with the same tests. */
namespace DashboardLayout
{
    enum Panel { spectrum, stereo, loudness, numPanels };

    using Panels = std::array<juce::Rectangle<int>, numPanels>;

    inline constexpr int padding = 10, gap = 10;

    /** The spectrum across the top, the stereo field and the loudness side by side below. */
    inline Panels dashboard (juce::Rectangle<int> bounds)
    {
        auto inner = bounds.reduced (padding);
        Panels panels;

        panels[spectrum] = inner.removeFromTop ((inner.getHeight() - gap) * 58 / 100);
        inner.removeFromTop (gap);
        panels[stereo] = inner.removeFromLeft ((inner.getWidth() - gap) * 40 / 100);
        inner.removeFromLeft (gap);
        panels[loudness] = inner;

        return panels;
    }

    /** The dashboard with one panel some way, from 0 to 1, to filling it. */
    inline Panels expanding (juce::Rectangle<int> bounds, Panel expanded, float amount)
    {
        auto panels = dashboard (bounds);
        const auto from = panels[expanded].toFloat();
        const auto to = bounds.reduced (padding).toFloat();
        const auto along = [amount] (float a, float b) { return a + (b - a) * amount; };

        panels[expanded] = juce::Rectangle<float> (along (from.getX(), to.getX()), along (from.getY(), to.getY()),
                                                   along (from.getWidth(), to.getWidth()), along (from.getHeight(), to.getHeight()))
                               .toNearestInt();
        return panels;
    }

    /** Slow at each end. */
    constexpr float ease (float amount)
    {
        return amount * amount * (3.0f - 2.0f * amount);
    }
}
