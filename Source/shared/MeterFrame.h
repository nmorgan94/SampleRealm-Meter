#pragma once

#include <array>

//==============================================================================
/**
 * What the meter shows at one moment, as one flat array of floats: the scalars, then the
 * spectrum's bands, its held peaks, then the goniometer's points as x, y pairs.
 * MeterEngine fills it; the editors only draw it. uiInfoJson() tells the web UI this layout.
 */
namespace MeterFrame
{
    enum Scalar
    {
        momentary,      // LUFS
        shortTerm,      // LUFS
        integrated,     // LUFS
        range,          // LU, or noLoudness until there is one
        truePeakLeft,   // dBTP, falling
        truePeakRight,
        maxTruePeak,    // dBTP, held
        clipCount,
        correlation,    // -1 to 1
        balance,        // -1, all left, to 1
        pulse,          // 0 to 1, up on each low hit
        integrating,    // 1 while the integrated loudness is counting
        numScalars
    };

    inline constexpr std::array<const char*, numScalars> scalarNames {
        "momentary", "shortTerm", "integrated", "range", "truePeakLeft", "truePeakRight", "maxTruePeak",
        "clipCount", "correlation", "balance", "pulse", "integrating"
    };

    inline constexpr int numBands = 96;
    inline constexpr int numPoints = 256;

    inline constexpr int bandsOffset = numScalars;
    inline constexpr int peaksOffset = bandsOffset + numBands;
    inline constexpr int pointsOffset = peaksOffset + numBands;
    inline constexpr int size = pointsOffset + 2 * numPoints;

    using Data = std::array<float, size>;

    // Band b runs from minHz * (maxHz / minHz) ^ (b / numBands)
    inline constexpr float minHz = 20.0f;
    inline constexpr float maxHz = 20000.0f;

    // The bottom of each scale. A loudness at noLoudness has no reading yet
    inline constexpr float noLoudness = -100.0f;
    inline constexpr float peakFloorDb = -60.0f;
    inline constexpr float bandFloorDb = -120.0f;

    /** The frame with nothing heard yet. */
    constexpr Data silent()
    {
        Data frame {};
        frame[momentary] = frame[shortTerm] = frame[integrated] = frame[range] = frame[maxTruePeak] = noLoudness;
        frame[truePeakLeft] = frame[truePeakRight] = peakFloorDb;
        frame[integrating] = 1.0f;

        for (int i = bandsOffset; i < pointsOffset; ++i)
            frame[static_cast<size_t> (i)] = bandFloorDb;

        return frame;
    }
}
