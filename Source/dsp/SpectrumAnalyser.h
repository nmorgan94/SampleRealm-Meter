#pragma once

#include <array>
#include <vector>
#include "../shared/MeterFrame.h"

//==============================================================================
/**
 * The spectrum of a mono signal as MeterFrame::numBands levels in dB, spaced evenly in pitch,
 * with the bands' held peaks. A full-scale sine at 1 kHz reads 0 dB; the scale tilts up
 * 4.5 dB an octave, which shows most music about level. Not thread safe: one thread uses it.
 */
class SpectrumAnalyser
{
public:
    static constexpr int numBands = MeterFrame::numBands;
    static constexpr float floorDb = MeterFrame::bandFloorDb;
    // Music's level falls with frequency, so an untilted display slopes down to the right.
    // 4.5 dB an octave is a common choice for an analyser's tilt
    static constexpr float tiltDbPerOctave = 4.5f;

    SpectrumAnalyser();

    void setSampleRate (double newSampleRate);

    /** Adds the signal's next samples. */
    void push (const float* samples, int numSamples) noexcept;

    /** Moves the bands on by a time, towards the newest samples' levels: they rise fast and fall slowly. */
    void update (double seconds) noexcept;

    /** Drops the held peaks to the current levels. */
    void resetPeaks() noexcept                                   { peaks = bands; }

    const std::array<float, numBands>& getBands() const noexcept   { return bands; }
    const std::array<float, numBands>& getPeaks() const noexcept   { return peaks; }

    /** The frequency in the middle of a band. */
    static float centreOf (int band) noexcept;

private:
    // 16384 samples, for the highest sample rates
    static constexpr int maxOrder = 14;
    static constexpr int maxSize = 1 << maxOrder;

    struct Band
    {
        int firstBin = 0, lastBin = 0;   // to take the loudest of, where the band spans whole bins
        float centreBin = 0.0f;          // to read between two bins, where it doesn't
        float tiltDb = 0.0f;
        bool aboveNyquist = false;       // nothing can be there at this sample rate
    };

    int size = 1 << 12;

    std::vector<float> ring, window, real, imaginary, cosines, sines;
    std::vector<int> reversed;
    int writeIndex = 0;

    // Whether there are samples the magnitudes haven't been made from, and how many zeros the newest run of them is
    bool stale = false;
    int silentRun = maxSize;

    std::array<Band, numBands> layout;
    std::array<float, numBands> bands, peaks;

    void analyse() noexcept;
    void transform() noexcept;
};
