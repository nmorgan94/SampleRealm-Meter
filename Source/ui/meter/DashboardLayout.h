#pragma once

#include <algorithm>
#include <array>
#include <span>
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

    // A drag leaves a panel at least this big: about its title beside the expand button, and
    // its title over a strip of content
    inline constexpr int minPanelWidth = 120, minPanelHeight = 64;

    /** The spaces that are shared out: the dashboard's rows, the lower of them, and the one row of a short dashboard. */
    enum class Group { rows, lower, oneRow };

    inline constexpr int maxSlots = 4;

    // The panel in each slot of the groups that hold panels. The rows are these two, then the lower row
    inline constexpr std::array<Panel, 2> rowPanels { spectrum, waveform };
    inline constexpr std::array<Panel, 2> lowerPanels { stereo, loudness };
    inline constexpr std::array<Panel, 4> oneRowPanels { spectrum, waveform, stereo, loudness };
    inline constexpr size_t lowerRowSlot = 2;

    /** How each group's space is shared between its slots, as weights. Dragging the gap between two panels changes them. */
    struct Sizes
    {
        // The spectrum has most of the height, as it had before the waveform; the waveform is a strip
        std::array<double, 3> rows { 58.0, 25.0, 42.0 };
        std::array<double, 2> lower { 40.0, 60.0 };

        // In one row the stereo field, which is square, needs the least width
        std::array<double, 4> oneRow { 3.0, 3.0, 2.0, 3.0 };

        std::span<double> of (Group group)
        {
            return group == Group::rows ? std::span<double> (rows) : group == Group::lower ? std::span<double> (lower) : std::span<double> (oneRow);
        }

        std::span<const double> of (Group group) const
        {
            return group == Group::rows ? std::span<const double> (rows) : group == Group::lower ? std::span<const double> (lower) : std::span<const double> (oneRow);
        }

        bool operator== (const Sizes&) const = default;
    };

    /** Which of a group's slots have anything in them. */
    inline std::array<bool, maxSlots> present (Group group, const Shown& shown)
    {
        if (group == Group::rows)
            return { shown[spectrum], shown[waveform], shown[stereo] || shown[loudness], false };

        if (group == Group::lower)
            return { shown[stereo], shown[loudness], false, false };

        return { shown[spectrum], shown[waveform], shown[stereo], shown[loudness] };
    }

    struct Part
    {
        size_t slot;
        juce::Rectangle<int> area;
    };

    /** Whether a group's parts sit side by side. */
    inline bool isAcross (Group group)
    {
        return group != Group::rows;
    }

    /** An area cut by weight into its present slots with gaps between, across or down. The last part takes what rounding leaves. */
    inline std::vector<Part> divided (juce::Rectangle<int> area, std::span<const double> weights, const std::array<bool, maxSlots>& slots, bool across)
    {
        std::vector<Part> parts;
        auto total = 0.0;

        for (size_t slot = 0; slot < weights.size(); ++slot)
        {
            if (slots[slot])
            {
                parts.push_back ({ slot, {} });
                total += weights[slot];
            }
        }

        const auto length = across ? area.getWidth() : area.getHeight();
        const auto room = length - gap * (static_cast<int> (parts.size()) - 1);

        for (auto& part : parts)
        {
            const auto size = &part == &parts.back() ? length : static_cast<int> (room * weights[part.slot] / total);
            part.area = across ? area.removeFromLeft (size) : area.removeFromTop (size);
            across ? area.removeFromLeft (gap) : area.removeFromTop (gap);
        }

        return parts;
    }

    inline bool isOneRow (juce::Rectangle<int> bounds)
    {
        return bounds.getHeight() < rowBelowHeight;
    }

    inline std::vector<Part> lowerParts (juce::Rectangle<int> row, const Shown& shown, const Sizes& sizes)
    {
        return divided (row, sizes.lower, present (Group::lower, shown), isAcross (Group::lower));
    }

    /** A group's parts in a dashboard that has it. */
    inline std::vector<Part> partsOf (Group group, juce::Rectangle<int> bounds, const Shown& shown, const Sizes& sizes)
    {
        if (group != Group::lower)
            return divided (bounds.reduced (padding), sizes.of (group), present (group, shown), isAcross (group));

        for (const auto& row : partsOf (Group::rows, bounds, shown, sizes))
            if (row.slot == lowerRowSlot)
                return lowerParts (row.area, shown, sizes);

        return {};
    }

    /**
     * The spectrum over the waveform, with the stereo field and the loudness side by side below,
     * or all in one row in a short dashboard. What's shown fills the space.
     */
    inline Panels dashboard (juce::Rectangle<int> bounds, const Shown& shown = allShown, const Sizes& sizes = {})
    {
        Panels panels;

        if (isOneRow (bounds))
        {
            for (const auto& part : partsOf (Group::oneRow, bounds, shown, sizes))
                panels[oneRowPanels[part.slot]] = part.area;

            return panels;
        }

        for (const auto& row : partsOf (Group::rows, bounds, shown, sizes))
        {
            if (row.slot != lowerRowSlot)
            {
                panels[rowPanels[row.slot]] = row.area;
                continue;
            }

            for (const auto& part : lowerParts (row.area, shown, sizes))
                panels[lowerPanels[part.slot]] = part.area;
        }

        return panels;
    }

    /** The gap between two neighbouring parts of a group, which a drag moves. */
    struct Divider
    {
        Group group;
        size_t before, after;   // the slots either side
        bool across;         // between parts side by side
        juce::Rectangle<int> area;

        bool operator== (const Divider&) const = default;
    };

    inline std::vector<Divider> dividers (juce::Rectangle<int> bounds, const Shown& shown = allShown, const Sizes& sizes = {})
    {
        std::vector<Divider> found;

        const auto add = [&] (Group group)
        {
            const auto parts = partsOf (group, bounds, shown, sizes);
            const auto across = isAcross (group);

            for (size_t i = 1; i < parts.size(); ++i)
            {
                const auto& before = parts[i - 1].area;
                found.push_back ({ group, parts[i - 1].slot, parts[i].slot, across,
                                   across ? before.withX (before.getRight()).withWidth (gap) : before.withY (before.getBottom()).withHeight (gap) });
            }
        };

        if (isOneRow (bounds))
        {
            add (Group::oneRow);
        }
        else
        {
            add (Group::rows);
            add (Group::lower);
        }

        return found;
    }

    /**
     * The sizes after a divider has been dragged some pixels right or down. Its two neighbours share
     * what they had, neither going under the minimum, and the group's weights become its parts' pixels.
     */
    inline Sizes dragged (juce::Rectangle<int> bounds, const Shown& shown, Sizes sizes, const Divider& divider, int pixels)
    {
        const auto parts = partsOf (divider.group, bounds, shown, sizes);
        const auto lengthOf = [&divider] (const Part& part) { return divider.across ? part.area.getWidth() : part.area.getHeight(); };

        const auto lengthIn = [&] (size_t slot)
        {
            const auto part = std::find_if (parts.begin(), parts.end(), [slot] (const Part& p) { return p.slot == slot; });
            return part != parts.end() ? lengthOf (*part) : 0;
        };

        const auto before = lengthIn (divider.before), after = lengthIn (divider.after);

        // Nothing there to share out
        if (std::any_of (parts.begin(), parts.end(), [&] (const Part& part) { return lengthOf (part) < 1; }) || before < 1 || after < 1)
            return sizes;

        // A panel already under the minimum, in a small editor, isn't made to grow
        const auto smallest = divider.across ? minPanelWidth : minPanelHeight;
        const auto moved = juce::jlimit (juce::jmin (smallest, before), before + after - juce::jmin (smallest, after), before + pixels);

        if (moved == before)
            return sizes;

        const auto weights = sizes.of (divider.group);
        auto total = 0.0;
        auto room = 0;

        for (const auto& part : parts)
        {
            total += weights[part.slot];
            room += lengthOf (part);
        }

        // The slots that aren't showing keep their share
        for (auto& weight : weights)
            weight *= room / total;

        for (const auto& part : parts)
            weights[part.slot] = lengthOf (part);

        weights[divider.before] = moved;
        weights[divider.after] = before + after - moved;
        return sizes;
    }

    /** The sizes with a group's back as they began. */
    inline Sizes reset (Sizes sizes, Group group)
    {
        const Sizes defaults;
        std::ranges::copy (defaults.of (group), sizes.of (group).begin());
        return sizes;
    }

    /** The dashboard with one panel some way, from 0 to 1, to filling it. */
    inline Panels expanding (juce::Rectangle<int> bounds, const Shown& shown, Panel expanded, float amount, const Sizes& sizes = {})
    {
        auto panels = dashboard (bounds, shown, sizes);
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
