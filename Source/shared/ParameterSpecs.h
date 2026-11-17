#pragma once

#include <samplerealm/ParameterSpec.h>

//==============================================================================
namespace ParameterSpecs
{
    // The palettes are Source/shared/Themes.h, in this order
    inline constexpr std::array<const char*, 5> themeNames { "Neon", "Ember", "Phosphor", "Ultraviolet", "Ice" };
    // How much the display moves with the beat
    inline constexpr std::array<const char*, 3> reactivityNames { "Off", "Subtle", "Full" };
    // The dashboard, or one of its panels filling the editor
    inline constexpr std::array<const char*, 4> viewNames { "Dashboard", "Spectrum", "Stereo", "Loudness" };

    // How the meter looks isn't something to automate
    inline constexpr Spec theme = notAutomatable (choice ("theme", "Theme", themeNames, 0));
    inline constexpr Spec reactivity = notAutomatable (choice ("reactivity", "Reactivity", reactivityNames, 2));
    inline constexpr Spec view = notAutomatable (choice ("view", "View", viewNames, 0));

    // The host's parameter order
    inline constexpr std::array all { &theme, &reactivity, &view };

    static_assert (areValid (all));
}
