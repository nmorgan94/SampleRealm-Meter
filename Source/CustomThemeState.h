#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "shared/Themes.h"

//==============================================================================
/**
 * The custom theme in the plugin's state, as a child of the parameters' tree, so sessions and
 * looks save it with them. A state without one, such as Init's, has the default.
 */
namespace CustomThemeState
{
    inline const juce::Identifier type { "CustomTheme" };
    inline const juce::Identifier baseProp { "base" }, primaryProp { "primary" }, secondaryProp { "secondary" }, hotProp { "hot" };

    /** From a tree's properties or an object's, as the web UI sends it. Anything missing or that isn't a colour keeps its default. */
    template <typename Source>
    Themes::Custom from (const Source& source)
    {
        Themes::Custom custom;

        const auto colour = [&source] (const juce::Identifier& property, std::uint32_t fallback)
        {
            return Themes::fromHex (source[property].toString().toStdString()).value_or (fallback);
        };

        if (const auto& base = source[baseProp]; ! base.isVoid())
            custom.base = juce::jlimit (0, Themes::customIndex - 1, static_cast<int> (base));

        custom.primary = colour (primaryProp, custom.primary);
        custom.secondary = colour (secondaryProp, custom.secondary);
        custom.hot = colour (hotProp, custom.hot);
        return custom;
    }

    inline Themes::Custom read (const juce::ValueTree& state)
    {
        return from (state.getChildWithName (type));
    }

    /** As CustomTheme in webui/src/backend.ts. */
    inline juce::var toVar (const Themes::Custom& custom)
    {
        auto object = std::make_unique<juce::DynamicObject>();
        object->setProperty (baseProp, custom.base);
        object->setProperty (primaryProp, juce::String (Themes::toHex (custom.primary)));
        object->setProperty (secondaryProp, juce::String (Themes::toHex (custom.secondary)));
        object->setProperty (hotProp, juce::String (Themes::toHex (custom.hot)));
        return object.release();
    }

    /** By value: the state's listeners hear each change as it's made, and may be what holds the caller's copy. */
    inline void write (juce::ValueTree state, Themes::Custom custom)
    {
        const auto existing = state.getChildWithName (type);
        auto tree = existing.isValid() ? existing : juce::ValueTree (type);

        tree.setProperty (baseProp, custom.base, nullptr);
        tree.setProperty (primaryProp, juce::String (Themes::toHex (custom.primary)), nullptr);
        tree.setProperty (secondaryProp, juce::String (Themes::toHex (custom.secondary)), nullptr);
        tree.setProperty (hotProp, juce::String (Themes::toHex (custom.hot)), nullptr);

        // A new one goes in whole
        if (! existing.isValid())
            state.appendChild (tree, nullptr);
    }
}
