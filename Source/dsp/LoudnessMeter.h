#pragma once

#include <array>
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../shared/MeterFrame.h"

//==============================================================================
/**
 * Programme loudness as ITU-R BS.1770-4 and EBU R 128 measure it: momentary (400 ms),
 * short-term (3 s), integrated (gated, since the last reset) and loudness range (EBU Tech 3342).
 * process() runs on the audio thread; the getters can be read from any thread.
 */
class LoudnessMeter
{
public:
    static constexpr int maxChannels = 2;
    static constexpr float noReading = MeterFrame::noLoudness;

    // EBU R 128's windows
    static constexpr double momentarySeconds = 0.4, shortTermSeconds = 3.0;

    LoudnessMeter() = default;

    void prepare (double sampleRate, int numChannelsToMeter);

    /** Channels beyond the prepared count are ignored. */
    void process (const juce::AudioBuffer<float>& buffer) noexcept;

    /** While false, the integrated loudness and the range stop counting. */
    void setIntegrating (bool shouldIntegrate) noexcept   { integrating = shouldIntegrate; }

    /** Starts the integrated loudness and the range again. */
    void resetIntegrated() noexcept;

    /** Starts the momentary and short-term loudness again, after a gap in the audio. */
    void forgetRecent() noexcept;

    float getMomentary() const noexcept    { return momentary.load(); }
    float getShortTerm() const noexcept    { return shortTerm.load(); }
    float getIntegrated() const noexcept   { return integrated.load(); }
    /** noReading until there has been 3 s to measure. */
    float getRange() const noexcept        { return range.load(); }

private:
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;

        double process (double x) noexcept
        {
            const auto y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    // Blocks by loudness, in 0.1 LU steps from the absolute gate up. Counting them this way, rather
    // than keeping every block, takes the same memory however long the programme runs. The price is
    // that the relative gates fall on a step, so a block up to 0.1 LU under a gate is still counted.
    // That moves the result far less than the 0.1 LU shown, and libebur128's histogram mode does the same
    struct Histogram
    {
        // From BS.1770's absolute gate of -70 LUFS up to +10, in steps finer than the 0.1 LU shown
        static constexpr int numBins = 800;
        static constexpr double bottom = -70.0;

        std::array<juce::uint32, numBins> counts {};
        std::array<double, numBins> powers {};

        void clear() noexcept;
        void add (double power) noexcept;

        /** The mean power of the blocks at or above a loudness, or 0 with none. */
        double meanFrom (double loudness) const noexcept;

        static int binFor (double loudness) noexcept;
    };

    // The windows in 100 ms blocks
    static constexpr int momentaryBlocks = 4, shortTermBlocks = 30;

    std::array<Biquad, maxChannels> shelf, highPass;
    int numChannels = maxChannels;
    int samplesPerBlock = 4800;

    // The mean square of each 100 ms, the newest at blockIndex - 1
    std::array<double, shortTermBlocks> blocks {};
    int blockIndex = 0;

    // How many whole blocks there have been since the integrated loudness last started
    int blocksIntegrated = 0;
    double blockSum = 0.0;
    int blockSamples = 0;

    bool integrating = true;
    Histogram gatingBlocks, shortTermHistogram;

    std::atomic<float> momentary { noReading }, shortTerm { noReading }, integrated { noReading }, range { noReading };

    void finishBlock() noexcept;
    double meanOfLast (int numBlocks) const noexcept;
    void updateIntegrated() noexcept;
    void updateRange() noexcept;

    JUCE_DECLARE_NON_COPYABLE (LoudnessMeter)
};
