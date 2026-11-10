#pragma once

#include <array>
#include <cstdint>
#include "ParameterSpecs.h"

//==============================================================================
/** The editor's palettes, one per ParameterSpecs::themeNames. Colours are 0xRRGGBB. */
namespace Themes
{
    struct Palette
    {
        std::uint32_t background;   // the window
        std::uint32_t panel;        // behind each view
        std::uint32_t line;         // outlines and grids
        std::uint32_t text;
        std::uint32_t dim;          // labels and scales
        std::uint32_t primary;      // traces and bars
        std::uint32_t secondary;    // the far end of a trace's gradient, and held peaks
        std::uint32_t hot;          // clips and out-of-phase
    };

    inline constexpr std::array<Palette, ParameterSpecs::themeNames.size()> all {{
        { 0x070a14, 0x0c1120, 0x1d2a44, 0xe8f4ff, 0x6f84a8, 0x19e6ff, 0xff2bd6, 0xff3d5a },   // Neon
        { 0x120906, 0x1b0f0a, 0x402418, 0xfff1e2, 0xa88468, 0xffa31a, 0xff4d2e, 0xff2a2a },   // Ember
        { 0x040d08, 0x08160e, 0x163a26, 0xe4ffe9, 0x6aa07c, 0x3dff7a, 0xc8ff3d, 0xff5a3d },   // Phosphor
        { 0x0b0716, 0x130c24, 0x2c1f52, 0xf0e8ff, 0x8a79b8, 0xa36bff, 0xff5ecb, 0xff4466 },   // Ultraviolet
        { 0x080c10, 0x0e141a, 0x25323e, 0xf2f8fc, 0x8296a6, 0xbfe9ff, 0x5aa8ff, 0xff5c6c },   // Ice
    }};

    /** The names of Palette's colours, in its order, for the web UI. */
    inline constexpr std::array<const char*, 8> colourNames {
        "background", "panel", "line", "text", "dim", "primary", "secondary", "hot"
    };

    constexpr std::array<std::uint32_t, 8> coloursOf (const Palette& p)
    {
        return { p.background, p.panel, p.line, p.text, p.dim, p.primary, p.secondary, p.hot };
    }
}
