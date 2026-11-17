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

    std::string json = "{\"frame\":{\"scalars\":[" + joined (MeterFrame::scalarNames, quoted);
    json += "],\"numBands\":" + std::to_string (MeterFrame::numBands);
    json += ",\"numPoints\":" + std::to_string (MeterFrame::numPoints);
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

    // Each theme's colours as "rrggbb", by Themes::colourNames
    for (size_t i = 0; i < Themes::all.size(); ++i)
    {
        const auto colours = Themes::coloursOf (Themes::all[i]);
        json += (i == 0 ? "{" : ",{");

        for (size_t c = 0; c < colours.size(); ++c)
        {
            static constexpr char digits[] = "0123456789abcdef";
            std::string hex;

            for (int shift = 20; shift >= 0; shift -= 4)
                hex += digits[(colours[c] >> shift) & 0xf];

            json += (c == 0 ? "" : ",") + quoted (Themes::colourNames[c]) + ":" + quoted (hex.c_str());
        }

        json += "}";
    }

    return json + "]}";
}
