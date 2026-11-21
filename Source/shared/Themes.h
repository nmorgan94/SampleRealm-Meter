#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include "ParameterSpecs.h"

//==============================================================================
/** The editor's palettes, in the order of ParameterSpecs::themeNames, and the user's own after them. Colours are 0xRRGGBB. */
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

    inline constexpr std::array<Palette, 5> all {{
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

    //==============================================================================
    // The theme after the built-in ones is the user's own
    inline constexpr int customIndex = static_cast<int> (all.size());
    static_assert (ParameterSpecs::themeNames.size() == all.size() + 1);

    /** The user's own theme: one of the built-in palettes with its three accents changed. */
    struct Custom
    {
        int base = 0;
        std::uint32_t primary = all[0].primary, secondary = all[0].secondary, hot = all[0].hot;

        bool operator== (const Custom&) const = default;
    };

    /** A built-in palette, kept within the list. */
    constexpr const Palette& builtIn (int index)
    {
        return all[static_cast<size_t> (index < 0 ? 0 : index >= customIndex ? customIndex - 1 : index)];
    }

    /** The palette for a value of the theme parameter. */
    constexpr Palette paletteOf (int theme, const Custom& custom)
    {
        if (theme < customIndex)
            return builtIn (theme);

        auto palette = builtIn (custom.base);
        palette.primary = custom.primary;
        palette.secondary = custom.secondary;
        palette.hot = custom.hot;
        return palette;
    }

    //==============================================================================
    // The flat colours after ParameterSpecs::backdropNames' "Theme". Pure green and blue are what video
    // editors key out, and black is for laying the meter over video with a screen or add blend
    inline constexpr std::array<std::uint32_t, 3> backdrops { 0x000000, 0x00ff00, 0x0000ff };
    static_assert (ParameterSpecs::backdropNames.size() == backdrops.size() + 1);

    /** Whether a value of the backdrop parameter is one of the flat colours. */
    constexpr bool isFlat (int backdrop)   { return backdrop >= 1 && backdrop <= static_cast<int> (backdrops.size()); }

    /** The palette over a backdrop: the window and the panels in its one colour. */
    constexpr Palette withBackdrop (Palette palette, int backdrop)
    {
        if (isFlat (backdrop))
            palette.background = palette.panel = backdrops[static_cast<size_t> (backdrop - 1)];

        return palette;
    }

    //==============================================================================
    /** A colour as "rrggbb", as saved and as the web UI has it. */
    inline std::string toHex (std::uint32_t colour)
    {
        static constexpr char digits[] = "0123456789abcdef";
        std::string hex;

        for (int shift = 20; shift >= 0; shift -= 4)
            hex += digits[(colour >> shift) & 0xf];

        return hex;
    }

    /** From six hex digits in either case, with or without a "#" before them. */
    inline std::optional<std::uint32_t> fromHex (std::string_view text)
    {
        if (! text.empty() && text.front() == '#')
            text.remove_prefix (1);

        if (text.size() != 6)
            return std::nullopt;

        std::uint32_t colour = 0;

        for (const auto character : text)
        {
            const auto digit = character >= '0' && character <= '9' ? character - '0'
                             : character >= 'a' && character <= 'f' ? character - 'a' + 10
                             : character >= 'A' && character <= 'F' ? character - 'A' + 10
                             : -1;

            if (digit < 0)
                return std::nullopt;

            colour = (colour << 4) | static_cast<std::uint32_t> (digit);
        }

        return colour;
    }
}
