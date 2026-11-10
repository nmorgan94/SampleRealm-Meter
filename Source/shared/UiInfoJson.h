#pragma once

#include <cmath>
#include <cstdlib>
#include <string>
#include "BuildType.h"
#include "MeterFrame.h"
#include "MeterScales.h"
#include "ParameterSpecs.h"
#include "Themes.h"

//==============================================================================
inline std::string uiInfoJson (const char* version, const char* commit)
{
    const auto quoted = [] (const char* text)
    {
        std::string result = "\"";

        for (auto* c = text; *c != '\0'; ++c)
        {
            if (*c == '"' || *c == '\\')
                result += '\\';

            result += *c;
        }

        return result + "\"";
    };

    // Not printf, which follows the host's locale
    const auto number = [] (float value)
    {
        const auto scaled = std::llround (static_cast<double> (value) * 10000.0);
        const auto magnitude = std::llabs (scaled);
        auto text = std::string (scaled < 0 ? "-" : "") + std::to_string (magnitude / 10000);

        if (const auto fraction = magnitude % 10000; fraction != 0)
        {
            auto digits = std::to_string (fraction);
            digits.insert (0, 4 - digits.size(), '0');
            digits.erase (digits.find_last_not_of ('0') + 1);
            text += "." + digits;
        }

        return text;
    };

    // A list's items as text, with commas between
    const auto joined = [] (const auto& values, const auto& asText)
    {
        std::string result;

        for (const auto& value : values)
            result += (result.empty() ? "" : ",") + asText (value);

        return result;
    };

    const auto whole = [] (int value) { return std::to_string (value); };

    const auto kindName = [] (ParameterSpecs::Kind kind)
    {
        switch (kind)
        {
            case ParameterSpecs::Kind::integer: return "integer";
            case ParameterSpecs::Kind::choice:  return "choice";
            case ParameterSpecs::Kind::real:    break;
        }

        return "real";
    };

    std::string json = "{\"version\":" + quoted (version) + ",\"commit\":" + quoted (commit) + ",\"build\":" + quoted (BuildType::name) + ",\"params\":[";

    for (size_t i = 0; i < ParameterSpecs::all.size(); ++i)
    {
        const auto& spec = *ParameterSpecs::all[i];

        json += (i == 0 ? "{" : ",{");
        json += "\"id\":" + quoted (spec.id) + ",\"name\":" + quoted (spec.name);
        json += ",\"kind\":" + quoted (kindName (spec.kind));
        json += ",\"min\":" + number (spec.min) + ",\"max\":" + number (spec.max);
        json += ",\"defaultValue\":" + number (spec.defaultValue);
        json += ",\"step\":" + number (spec.step) + ",\"skew\":" + number (spec.skew);
        json += ",\"unit\":" + quoted (spec.unit) + ",\"choices\":[" + joined (spec.choices, quoted) + "]}";
    }

    json += "],\"frame\":{\"scalars\":[" + joined (MeterFrame::scalarNames, quoted);
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
