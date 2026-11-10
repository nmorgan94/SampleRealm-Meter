#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
 * A neon line: the line itself over wider, fainter strokes of the same colour. No blur, so
 * it's cheap to draw every frame and looks the same in webui/src/meter/glow.ts.
 */
namespace Glow
{
    struct Pass
    {
        float width, alpha;
    };

    /** The strokes for a line of a thickness, the widest first. More amount, from 0 to 1, glows more. */
    constexpr std::array<Pass, 3> passes (float thickness, float amount)
    {
        return { { { thickness * (4.0f + 2.5f * amount), 0.10f + 0.06f * amount },
                   { thickness * (2.2f + amount), 0.22f + 0.08f * amount },
                   { thickness, 1.0f } } };
    }

    inline void stroke (juce::Graphics& g, const juce::Path& path, const juce::FillType& fill, float thickness, float amount)
    {
        for (const auto& pass : passes (thickness, amount))
        {
            g.setFillType (fill);
            g.setOpacity (pass.alpha);
            g.strokePath (path, juce::PathStrokeType (pass.width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setOpacity (1.0f);
    }
}
