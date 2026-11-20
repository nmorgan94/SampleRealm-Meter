#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include "../shared/MeterFrame.h"

//==============================================================================
/**
 * The signal's last few seconds as columns of peaks, each split into its lows, mids and highs,
 * for a waveform that scrolls by as DJ software draws one. Not thread safe: one thread uses it.
 */
class WaveformHistory
{
public:
    static constexpr int numColumns = MeterFrame::numColumns;
    static constexpr int numBands = MeterFrame::waveformBands;

    // One column for each frame an editor draws, so the waveform moves on every frame. With
    // numColumns of them that's 8 seconds: a few bars at any usual tempo
    static constexpr double columnsPerSecond = 60.0;

    void setSampleRate (double sampleRate) noexcept
    {
        // Under the Nyquist frequency at any rate
        const auto within = [sampleRate] (double hz) { return std::min (hz, 0.45 * sampleRate); };

        low = Filter::lowPass (within (lowHz), sampleRate);
        midFrom = Filter::highPass (within (lowHz), sampleRate);
        midTo = Filter::lowPass (within (highHz), sampleRate);
        high = Filter::highPass (within (highHz), sampleRate);

        samplesPerColumn = std::max (1, static_cast<int> (std::lround (sampleRate / columnsPerSecond)));
        reset();
    }

    void reset() noexcept
    {
        for (auto* filter : { &low, &midFrom, &midTo, &high })
            filter->reset();

        columns.fill (0.0f);
        column.fill (0.0f);
        columnIndex = filled = 0;
        silentColumns = numColumns;
    }

    /** Takes the next samples of a mono signal. */
    void push (const float* samples, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto sample = std::isfinite (samples[i]) ? samples[i] : 0.0f;

            column[0] = std::max (column[0], std::abs (low.process (sample)));
            column[1] = std::max (column[1], std::abs (midTo.process (midFrom.process (sample))));
            column[2] = std::max (column[2], std::abs (high.process (sample)));

            if (++filled < samplesPerColumn)
                continue;

            // Under -100 dB there's nothing to draw, and silence leaves the columns at exactly 0
            for (auto& peak : column)
                if (peak < quietest)
                    peak = 0.0f;

            silentColumns = isSilent (column) ? std::min (silentColumns + 1, numColumns) : 0;
            std::copy (column.begin(), column.end(), columns.begin() + numBands * columnIndex);
            columnIndex = (columnIndex + 1) % numColumns;
            column.fill (0.0f);
            filled = 0;
        }

        for (auto* filter : { &low, &midFrom, &midTo, &high })
            filter->leaveDenormals();
    }

    /**
     * How much of the next column has been heard, from 0 to 1, for scrolling between columns.
     * Stays 0 while every column is silent, so the frame stops changing and the editors stop drawing.
     */
    float getPhase() const noexcept
    {
        if (silentColumns == numColumns && isSilent (column))
            return 0.0f;

        return static_cast<float> (filled) / static_cast<float> (samplesPerColumn);
    }

    /** Each column's low, mid and high peaks, from 0 to about 1, the oldest column first. */
    void copyColumns (float* destination) const noexcept
    {
        const auto first = columns.begin() + numBands * columnIndex;
        std::copy (columns.begin(), first, std::copy (first, columns.end(), destination));
    }

private:
    // Kicks and bass sit under the first, and hats, cymbals and the edges of voices over the
    // second: the split DJ software's waveforms make
    static constexpr double lowHz = 250.0;
    static constexpr double highHz = 2500.0;

    static constexpr float quietest = 1.0e-5f;

    /** A second-order Butterworth filter, from the Audio EQ Cookbook: 12 dB an octave, with no peak at the corner. */
    struct Filter
    {
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float s1 = 0.0f, s2 = 0.0f;

        static Filter make (double hz, double sampleRate, bool passesHighs) noexcept
        {
            const auto w = 2.0 * std::numbers::pi * hz / sampleRate;
            const auto cosine = std::cos (w);
            const auto alpha = std::sin (w) / std::numbers::sqrt2;
            const auto a0 = 1.0 + alpha;
            const auto edge = passesHighs ? 1.0 + cosine : 1.0 - cosine;

            Filter filter;
            filter.b0 = filter.b2 = static_cast<float> (0.5 * edge / a0);
            filter.b1 = static_cast<float> ((passesHighs ? -edge : edge) / a0);
            filter.a1 = static_cast<float> (-2.0 * cosine / a0);
            filter.a2 = static_cast<float> ((1.0 - alpha) / a0);
            return filter;
        }

        static Filter lowPass (double hz, double sampleRate) noexcept    { return make (hz, sampleRate, false); }
        static Filter highPass (double hz, double sampleRate) noexcept   { return make (hz, sampleRate, true); }

        float process (float in) noexcept
        {
            const auto out = b0 * in + s1;
            s1 = b1 * in - a1 * out + s2;
            s2 = b2 * in - a2 * out;
            return out;
        }

        void reset() noexcept   { s1 = s2 = 0.0f; }

        // 1e-20 is -400 dB, far under anything drawn, and far over where floats go denormal (1e-38)
        void leaveDenormals() noexcept
        {
            if (std::abs (s1) < 1.0e-20f)   s1 = 0.0f;
            if (std::abs (s2) < 1.0e-20f)   s2 = 0.0f;
        }
    };

    Filter low, midFrom, midTo, high;
    int samplesPerColumn = 1, filled = 0;

    using Column = std::array<float, numBands>;

    static bool isSilent (const Column& peaks) noexcept
    {
        return std::all_of (peaks.begin(), peaks.end(), [] (float peak) { return peak < quietest; });
    }

    Column column {};
    std::array<float, static_cast<size_t> (numBands * numColumns)> columns {};
    int columnIndex = 0;

    // How many of the latest columns in a row are silent, up to all of them
    int silentColumns = numColumns;
};
