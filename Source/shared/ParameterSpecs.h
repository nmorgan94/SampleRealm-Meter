#pragma once

#include <samplerealm/ParameterSpec.h>

//==============================================================================
namespace ParameterSpecs
{
    // The palettes are Source/shared/Themes.h, in this order, then the user's own
    inline constexpr std::array<const char*, 6> themeNames { "Neon", "Ember", "Phosphor", "Ultraviolet", "Ice", "Custom" };
    // How much the display moves with the beat
    inline constexpr std::array<const char*, 3> reactivityNames { "Off", "Subtle", "Full" };
    // The dashboard, or one of its panels filling the editor
    inline constexpr std::array<const char*, 5> viewNames { "Dashboard", "Spectrum", "Stereo", "Loudness", "Waveform" };

    // How the meter looks isn't something to automate
    inline constexpr Spec theme = notAutomatable (choice ("theme", "Theme", themeNames, 0));
    inline constexpr Spec reactivity = notAutomatable (choice ("reactivity", "Reactivity", reactivityNames, 2));
    inline constexpr Spec view = notAutomatable (choice ("view", "View", viewNames, 0));

    // Which panels the dashboard has
    inline constexpr Spec showSpectrum = notAutomatable (toggle ("showSpectrum", "Show Spectrum", true));
    inline constexpr Spec showWaveform = notAutomatable (toggle ("showWaveform", "Show Waveform", true));
    inline constexpr Spec showStereo = notAutomatable (toggle ("showStereo", "Show Stereo", true));
    inline constexpr Spec showLoudness = notAutomatable (toggle ("showLoudness", "Show Loudness", true));

    // The host's parameter order
    inline constexpr std::array all { &theme, &reactivity, &view, &showSpectrum, &showWaveform, &showStereo, &showLoudness };

    static_assert (areValid (all));
}
