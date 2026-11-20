#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../shared/Themes.h"

//==============================================================================
/** A Themes::Palette as colours. */
struct MeterTheme
{
    juce::Colour background, panel, line, text, dim, primary, secondary, hot;

    static juce::Colour opaque (std::uint32_t rgb)   { return juce::Colour (0xff000000u | rgb); }

    static MeterTheme of (const Themes::Palette& palette)
    {
        return { opaque (palette.background), opaque (palette.panel), opaque (palette.line), opaque (palette.text),
                 opaque (palette.dim), opaque (palette.primary), opaque (palette.secondary), opaque (palette.hot) };
    }

    /** A built-in theme. */
    static MeterTheme of (int index)   { return of (Themes::builtIn (index)); }
};

/** How the meter is drawn right now, shared by every view. */
struct MeterLook
{
    MeterTheme theme = MeterTheme::of (0);
    float reactivity = 1.0f;   // one of MeterScales::reactivityAmounts
    bool capture = false;      // clean capture mode: no titles or small labels
};
