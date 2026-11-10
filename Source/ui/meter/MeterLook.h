#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../shared/Themes.h"

//==============================================================================
/** A Themes::Palette as colours. */
struct MeterTheme
{
    juce::Colour background, panel, line, text, dim, primary, secondary, hot;

    static MeterTheme of (int index)
    {
        const auto& palette = Themes::all[static_cast<size_t> (juce::jlimit (0, static_cast<int> (Themes::all.size()) - 1, index))];
        const auto opaque = [] (std::uint32_t rgb) { return juce::Colour (0xff000000u | rgb); };

        return { opaque (palette.background), opaque (palette.panel), opaque (palette.line), opaque (palette.text),
                 opaque (palette.dim), opaque (palette.primary), opaque (palette.secondary), opaque (palette.hot) };
    }
};

/** How the meter is drawn right now, shared by every view. */
struct MeterLook
{
    MeterTheme theme = MeterTheme::of (0);
    float reactivity = 1.0f;   // one of MeterScales::reactivityAmounts
    bool capture = false;      // clean capture mode: no titles or small labels
};
