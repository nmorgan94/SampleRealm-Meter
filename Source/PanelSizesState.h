#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "ui/meter/DashboardLayout.h"

//==============================================================================
/**
 * The panels' sizes in the plugin's state, as a child of the parameters' tree, so sessions and
 * looks save them with them. A state without one, such as Init's, has the default.
 */
namespace PanelSizesState
{
    using Sizes = DashboardLayout::Sizes;
    using Group = DashboardLayout::Group;

    inline const juce::Identifier type { "PanelSizes" };
    inline const juce::Identifier rowsProp { "rows" }, lowerProp { "lower" }, oneRowProp { "oneRow" };

    inline const std::array<std::pair<Group, juce::Identifier>, 3> groups { { { Group::rows, rowsProp }, { Group::lower, lowerProp }, { Group::oneRow, oneRowProp } } };

    inline juce::StringArray tokensOf (const juce::String& list)
    {
        auto tokens = juce::StringArray::fromTokens (list, false);
        tokens.removeEmptyStrings();
        return tokens;
    }

    /** Not a number unless the whole token is one, as sanitiseSizes in the web UI reads it. */
    inline double numberIn (const juce::String& token)
    {
        auto text = token.getCharPointer();
        const auto number = juce::CharacterFunctions::readDoubleValue (text);
        return text.isEmpty() ? number : std::numeric_limits<double>::quiet_NaN();
    }

    /**
     * From a tree's properties, each a list with spaces between, or an object's arrays, as the web UI
     * sends them. A list of the wrong length, or with a weight that isn't over 0, keeps its default.
     */
    template <typename Source>
    Sizes from (const Source& source)
    {
        Sizes sizes;

        for (const auto& [group, property] : groups)
        {
            const auto& value = source[property];
            juce::Array<juce::var> list;

            if (const auto* array = value.getArray())
                list = *array;
            else
                for (const auto& token : tokensOf (value.toString()))
                    list.add (numberIn (token));

            const auto weights = sizes.of (group);

            const auto isWeight = [] (const juce::var& weight)
            {
                const double value = weight;
                return std::isfinite (value) && value > 0.0;
            };

            if (static_cast<size_t> (list.size()) == weights.size() && std::all_of (list.begin(), list.end(), isWeight))
                std::copy (list.begin(), list.end(), weights.begin());
        }

        return sizes;
    }

    inline Sizes read (const juce::ValueTree& state)
    {
        return from (state.getChildWithName (type));
    }

    /** As PanelSizes in webui/src/backend.ts. */
    inline juce::var toVar (const Sizes& sizes)
    {
        auto object = std::make_unique<juce::DynamicObject>();

        for (const auto& [group, property] : groups)
        {
            const auto weights = sizes.of (group);
            object->setProperty (property, juce::Array<juce::var> (weights.data(), static_cast<int> (weights.size())));
        }

        return object.release();
    }

    /** By value: the state's listeners hear each change as it's made, and may be what holds the caller's copy. */
    inline void write (juce::ValueTree state, Sizes sizes)
    {
        const auto existing = state.getChildWithName (type);
        auto tree = existing.isValid() ? existing : juce::ValueTree (type);

        for (const auto& [group, property] : groups)
        {
            juce::StringArray list;

            // A var's text reads back as the same double, where juce::String (double) keeps 6 digits
            for (const auto weight : sizes.of (group))
                list.add (juce::var (weight).toString());

            tree.setProperty (property, list.joinIntoString (" "), nullptr);
        }

        // A new one goes in whole
        if (! existing.isValid())
            state.appendChild (tree, nullptr);
    }
}
