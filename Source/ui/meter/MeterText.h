#pragma once

#include <juce_core/juce_core.h>
#include "../../shared/MeterFrame.h"

//==============================================================================
/** The meter's readings as text. As webui/src/meter/text.ts, with the same tests. */
namespace MeterText
{
    inline const juce::String none { "--.-" };

    inline bool hasReading (float value)   { return value > MeterFrame::noLoudness + 1.0f; }

    /** LUFS or LU to one place. */
    inline juce::String loudness (float value)
    {
        return hasReading (value) ? juce::String (value, 1) : none;
    }

    /** dBTP to one place, with its sign. */
    inline juce::String truePeak (float value)
    {
        if (! hasReading (value))
            return none;

        const auto tenths = juce::roundToInt (value * 10.0f);
        const auto text = juce::String (std::abs (tenths) / 10) + "." + juce::String (std::abs (tenths) % 10);
        return tenths > 0 ? "+" + text : tenths < 0 ? "-" + text : text;
    }

    /** In Hz up to a thousand, then in kHz to three figures. */
    inline juce::String frequency (float hz)
    {
        if (hz < 999.5f)
            return juce::String (juce::roundToInt (hz)) + " Hz";

        return juce::String (hz / 1000.0f, hz < 9995.0f ? 2 : 1) + " kHz";
    }

    /** dB to one place. */
    inline juce::String level (float db)   { return juce::String (db, 1) + " dB"; }

    inline juce::String clips (float count)
    {
        return count > 999.0f ? juce::String ("999+") : juce::String (juce::roundToInt (count));
    }
}
