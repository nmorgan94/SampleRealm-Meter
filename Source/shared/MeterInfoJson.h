#pragma once

#include <string>
#include <samplerealm/UiInfoJson.h>
#include "MeterFrame.h"
#include "MeterScales.h"
#include "Themes.h"

//==============================================================================
/** What the web editor draws the meter from, beside uiInfoJson: MeterInfo in webui/src/backend.ts. */
inline std::string meterInfoJson()
{
    using JsonText::number, JsonText::quoted;

    // A list's items as text, with commas between
    const auto joined = [] (const auto& values, const auto& asText)
    {
        std::string result;

        for (const auto& value : values)
            result += (result.empty() ? "" : ",") + asText (value);

        return result;
    };

    const auto whole = [] (int value) { return std::to_string (value); };
    const auto hex = [] (std::uint32_t colour) { return quoted (Themes::toHex (colour).c_str()); };

    std::string json = "{\"frame\":{\"scalars\":[" + joined (MeterFrame::scalarNames, quoted);
    json += "],\"numBands\":" + std::to_string (MeterFrame::numBands);
    json += ",\"numPoints\":" + std::to_string (MeterFrame::numPoints);
    json += ",\"numColumns\":" + std::to_string (MeterFrame::numColumns);
    json += ",\"columnsPerSecond\":" + number (MeterFrame::columnsPerSecond);
    json += ",\"waveformBands\":" + std::to_string (MeterFrame::waveformBands);
    json += ",\"minHz\":" + number (MeterFrame::minHz) + ",\"maxHz\":" + number (MeterFrame::maxHz);
    json += ",\"noLoudness\":" + number (MeterFrame::noLoudness);
    json += ",\"peakFloorDb\":" + number (MeterFrame::peakFloorDb);
    json += ",\"bandFloorDb\":" + number (MeterFrame::bandFloorDb) + "},\"scales\":{";

    // MeterScales, by name
    json += "\"spectrumTopDb\":" + number (MeterScales::spectrumTopDb) + ",\"spectrumBottomDb\":" + number (MeterScales::spectrumBottomDb);
    json += ",\"spectrumGridHz\":[" + joined (MeterScales::spectrumGridHz, number);
    json += "],\"spectrumGridLabels\":[" + joined (MeterScales::spectrumGridLabels, quoted);
    json += "],\"spectrumGridDb\":[" + joined (MeterScales::spectrumGridDb, whole);
    json += "],\"levelTop\":" + number (MeterScales::levelTop) + ",\"levelBottom\":" + number (MeterScales::levelBottom);
    json += ",\"levelTicks\":[" + joined (MeterScales::levelTicks, whole);
    json += "],\"reactivityAmounts\":[" + joined (MeterScales::reactivityAmounts, number);
    json += "],\"expandSeconds\":" + number (static_cast<float> (MeterScales::expandSeconds));
    json += ",\"traceFadeSeconds\":" + number (static_cast<float> (MeterScales::traceFadeSeconds)) + "},\"themes\":[";

    // Each built-in theme's colours as "rrggbb", by Themes::colourNames
    for (size_t i = 0; i < Themes::all.size(); ++i)
    {
        const auto colours = Themes::coloursOf (Themes::all[i]);
        json += (i == 0 ? "{" : ",{");

        for (size_t c = 0; c < colours.size(); ++c)
            json += (c == 0 ? "" : ",") + quoted (Themes::colourNames[c]) + ":" + hex (colours[c]);

        json += "}";
    }

    json += "],\"backdrops\":[" + joined (Themes::backdrops, hex);

    // The custom theme before the user has changed it
    const Themes::Custom custom;
    json += "],\"customTheme\":{\"base\":" + std::to_string (custom.base);
    json += ",\"primary\":" + hex (custom.primary);
    json += ",\"secondary\":" + hex (custom.secondary);
    json += ",\"hot\":" + hex (custom.hot);

    return json + "}}";
}
