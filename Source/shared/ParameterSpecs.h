#pragma once

#include <algorithm>
#include <array>
#include <span>

//==============================================================================
namespace ParameterSpecs
{
    enum class Kind { real, integer, choice };

    struct Spec
    {
        const char* id;
        const char* name;
        Kind kind;
        float min, max, defaultValue;
        float step = 0.0f;
        float skew = 1.0f;
        const char* unit = "";
        std::span<const char* const> choices = {};
    };

    constexpr Spec integer (const char* id, const char* name, int min, int max, int defaultValue, const char* unit = "")
    {
        return { id, name, Kind::integer, static_cast<float> (min), static_cast<float> (max),
                 static_cast<float> (defaultValue), 1.0f, 1.0f, unit };
    }

    constexpr Spec choice (const char* id, const char* name, std::span<const char* const> choices, int defaultValue)
    {
        return { id, name, Kind::choice, 0.0f, static_cast<float> (choices.size() - 1),
                 static_cast<float> (defaultValue), 1.0f, 1.0f, "", choices };
    }

    // The palettes are Source/shared/Themes.h, in this order
    inline constexpr std::array<const char*, 5> themeNames { "Neon", "Ember", "Phosphor", "Ultraviolet", "Ice" };
    // How much the display moves with the beat
    inline constexpr std::array<const char*, 3> reactivityNames { "Off", "Subtle", "Full" };
    // The dashboard, or one of its panels filling the editor
    inline constexpr std::array<const char*, 4> viewNames { "Dashboard", "Spectrum", "Stereo", "Loudness" };

    inline constexpr Spec theme = choice ("theme", "Theme", themeNames, 0);
    inline constexpr Spec reactivity = choice ("reactivity", "Reactivity", reactivityNames, 2);
    inline constexpr Spec view = choice ("view", "View", viewNames, 0);

    // The host's parameter order
    inline constexpr std::array all { &theme, &reactivity, &view };

    static_assert (std::ranges::all_of (all, [] (const Spec* spec)
    {
        if (spec->kind == Kind::real)
            return true;

        return spec->step == 1.0f
            && (spec->kind == Kind::integer
                || (spec->min == 0.0f && static_cast<size_t> (spec->max) == spec->choices.size() - 1));
    }), "Integer and choice specs need step 1, and a choice spec's range is 0 to its last choice");
}
