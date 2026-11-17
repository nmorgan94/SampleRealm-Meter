#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include "MeterFrame.h"
#include "ParameterSpecs.h"

//==============================================================================
/**
 * Where values sit on the editor's scales, and how fast it moves. meterInfoJson() tells the web UI
 * the numbers; the two functions are also in webui/src/meter/scales.ts.
 */
namespace MeterScales
{
    // The spectrum, in dB after its tilt, which leaves music well under 0
    inline constexpr float spectrumTopDb = -6.0f, spectrumBottomDb = -84.0f;
    inline constexpr std::array<float, 8> spectrumGridHz { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f };
    inline constexpr std::array<const char*, 8> spectrumGridLabels { "50", "100", "200", "500", "1k", "2k", "5k", "10k" };
    inline constexpr std::array<int, 3> spectrumGridDb { -24, -48, -72 };

    // The loudness and true peak bars share one scale, in LUFS and dBTP
    inline constexpr float levelTop = 3.0f, levelBottom = -48.0f;
    inline constexpr std::array<int, 5> levelTicks { 0, -12, -24, -36, -48 };

    // How far the display moves with the beat, by ParameterSpecs::reactivityNames
    inline constexpr std::array<float, 3> reactivityAmounts { 0.0f, 0.4f, 1.0f };
    static_assert (reactivityAmounts.size() == ParameterSpecs::reactivityNames.size());

    // How long a panel takes to grow over the dashboard or shrink back: quick, but slow enough to follow
    inline constexpr double expandSeconds = 0.22;

    // How long the goniometer's trace takes to fade to about a third: a few frames of afterglow
    inline constexpr double traceFadeSeconds = 0.07;

    /** How far up a scale a value is, from 0 at the bottom to 1. */
    constexpr float proportion (float value, float bottom, float top)
    {
        return std::clamp ((value - bottom) / (top - bottom), 0.0f, 1.0f);
    }

    /** How far across the spectrum a frequency is, from 0 at MeterFrame::minHz to 1. */
    inline float acrossSpectrum (float hz)
    {
        return std::log (hz / MeterFrame::minHz) / std::log (MeterFrame::maxHz / MeterFrame::minHz);
    }
}
