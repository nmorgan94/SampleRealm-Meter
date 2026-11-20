#pragma once

#include <array>
#include <numeric>
#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/** Where the dashboard's panels go. As webui/src/meter/dashboardLayout.ts, with the same tests. */
namespace DashboardLayout
{
    // In the order of the view parameter's choices after the dashboard
    enum Panel { spectrum, stereo, loudness, waveform, numPanels };

    using Panels = std::array<juce::Rectangle<int>, numPanels>;

    /** Which panels the dashboard has. One that isn't shown gets an empty rectangle. */
    using Shown = std::array<bool, numPanels>;
    inline constexpr Shown allShown { true, true, true, true };

    inline constexpr int padding = 10, gap = 10;

    // A dashboard shorter than this is one row. Any taller and the lower of its rows is still
    // too short for the loudness readings
    inline constexpr int rowBelowHeight = 416;

    struct Weighted
    {
        Panel panel;
        int weight;
    };

    // The spectrum has most of the height, as it had before the waveform; the waveform is a strip
    inline constexpr int spectrumRowWeight = 58, waveformRowWeight = 25, lowerRowWeight = 42;
    inline constexpr std::array<Weighted, 2> lowerRow { { { stereo, 40 }, { loudness, 60 } } };

    // In one row the stereo field, which is square, needs the least width
    inline constexpr std::array<Weighted, 4> oneRow { { { spectrum, 3 }, { waveform, 3 }, { stereo, 2 }, { loudness, 3 } } };

    /** An area cut by weight into parts with gaps between, across or down. The last part takes what rounding leaves. */
    inline std::vector<juce::Rectangle<int>> divided (juce::Rectangle<int> area, const std::vector<int>& weights, bool across)
    {
        const auto length = across ? area.getWidth() : area.getHeight();
        const auto room = length - gap * (static_cast<int> (weights.size()) - 1);
        const auto total = std::accumulate (weights.begin(), weights.end(), 0);

        std::vector<juce::Rectangle<int>> parts;

        for (const auto weight : weights)
        {
            const auto size = parts.size() + 1 == weights.size() ? length : room * weight / total;
            parts.push_back (across ? area.removeFromLeft (size) : area.removeFromTop (size));
            across ? area.removeFromLeft (gap) : area.removeFromTop (gap);
        }

        return parts;
    }

    /**
     * The spectrum over the waveform, with the stereo field and the loudness side by side below,
     * or all in one row in a short dashboard. What's shown fills the space.
     */
    inline Panels dashboard (juce::Rectangle<int> bounds, const Shown& shown = allShown)
    {
        const auto inner = bounds.reduced (padding);
        Panels panels;

        const auto placeAcross = [&] (juce::Rectangle<int> area, const auto& row)
        {
            std::vector<Panel> present;
            std::vector<int> weights;

            for (const auto& [panel, weight] : row)
                if (shown[panel])
                    present.push_back (panel), weights.push_back (weight);

            const auto parts = divided (area, weights, true);

            for (size_t i = 0; i < present.size(); ++i)
                panels[present[i]] = parts[i];
        };

        if (bounds.getHeight() < rowBelowHeight)
        {
            placeAcross (inner, oneRow);
            return panels;
        }

        const auto hasLowerRow = shown[stereo] || shown[loudness];
        std::vector<int> weights;

        if (shown[spectrum])   weights.push_back (spectrumRowWeight);
        if (shown[waveform])   weights.push_back (waveformRowWeight);
        if (hasLowerRow)       weights.push_back (lowerRowWeight);

        const auto rows = divided (inner, weights, false);
        auto row = rows.begin();

        if (shown[spectrum])   panels[spectrum] = *row++;
        if (shown[waveform])   panels[waveform] = *row++;
        if (hasLowerRow)       placeAcross (*row, lowerRow);

        return panels;
    }

    /** The dashboard with one panel some way, from 0 to 1, to filling it. */
    inline Panels expanding (juce::Rectangle<int> bounds, const Shown& shown, Panel expanded, float amount)
    {
        auto panels = dashboard (bounds, shown);
        const auto to = bounds.reduced (padding).toFloat();

        // A panel the dashboard doesn't show grows from the middle
        const auto from = shown[expanded] ? panels[expanded].toFloat() : to.withSizeKeepingCentre (0.0f, 0.0f);
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
