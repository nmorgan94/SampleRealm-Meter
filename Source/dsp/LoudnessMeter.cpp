#include "LoudnessMeter.h"

namespace
{
    // Under -100 dB, which reads as no loudness at all rather than as a number
    constexpr double silentPower = 1.0e-10;

    double toLoudness (double power) noexcept
    {
        // BS.1770's -0.691 makes up for the K-weighting's gain at 1 kHz, so a full-scale 1 kHz sine
        // in both channels reads the same in LUFS as it does in dBFS
        return power > silentPower ? -0.691 + 10.0 * std::log10 (power) : static_cast<double> (LoudnessMeter::noReading);
    }
}

//==============================================================================
void LoudnessMeter::Histogram::clear() noexcept
{
    counts.fill (0);
    powers.fill (0.0);
}

int LoudnessMeter::Histogram::binFor (double loudness) noexcept
{
    return juce::jlimit (0, numBins - 1, static_cast<int> (std::floor ((loudness - bottom) * 10.0)));
}

void LoudnessMeter::Histogram::add (double power) noexcept
{
    const auto loudness = toLoudness (power);

    if (loudness < bottom)
        return;

    const auto bin = static_cast<size_t> (binFor (loudness));
    ++counts[bin];
    powers[bin] += power;
}

double LoudnessMeter::Histogram::meanFrom (double loudness) const noexcept
{
    double total = 0.0;
    juce::uint64 count = 0;

    for (auto bin = static_cast<size_t> (binFor (loudness)); bin < numBins; ++bin)
    {
        total += powers[bin];
        count += counts[bin];
    }

    return count > 0 ? total / static_cast<double> (count) : 0.0;
}

//==============================================================================
void LoudnessMeter::prepare (double sampleRate, int numChannelsToMeter)
{
    numChannels = juce::jlimit (1, maxChannels, numChannelsToMeter);

    // 100 ms, the step the standard's 400 ms and 3 s windows move in
    samplesPerBlock = juce::jmax (1, juce::roundToInt (sampleRate / 10.0));

    // The K-weighting of BS.1770: a high shelf for the head's effect on what reaches the ear, then a
    // high-pass, as people hear less of the low end. The standard only gives the two filters'
    // coefficients at 48 kHz. The frequencies, gain and Qs below are the analogue filters those
    // coefficients come from, so the same curve can be made at any sample rate. They are the values
    // libebur128 uses (https://github.com/jiixyj/libebur128, MIT licence)
    Biquad shelfFilter, highPassFilter;

    {
        // A shelf at 1682 Hz, lifting the highs by 4 dB
        const auto k = std::tan (juce::MathConstants<double>::pi * 1681.974450955533 / sampleRate);
        const auto q = 0.7071752369554196;
        const auto vh = std::pow (10.0, 3.999843853973347 / 20.0);

        // The gain halfway up the shelf: about the square root of the full gain
        const auto vb = std::pow (vh, 0.4996667741545416);
        const auto a0 = 1.0 + k / q + k * k;

        shelfFilter.b0 = (vh + vb * k / q + k * k) / a0;
        shelfFilter.b1 = 2.0 * (k * k - vh) / a0;
        shelfFilter.b2 = (vh - vb * k / q + k * k) / a0;
        shelfFilter.a1 = 2.0 * (k * k - 1.0) / a0;
        shelfFilter.a2 = (1.0 - k / q + k * k) / a0;
    }

    {
        // A second-order high-pass at 38 Hz
        const auto k = std::tan (juce::MathConstants<double>::pi * 38.13547087602444 / sampleRate);
        const auto q = 0.5003270373238773;
        const auto a0 = 1.0 + k / q + k * k;

        highPassFilter.b0 = 1.0;
        highPassFilter.b1 = -2.0;
        highPassFilter.b2 = 1.0;
        highPassFilter.a1 = 2.0 * (k * k - 1.0) / a0;
        highPassFilter.a2 = (1.0 - k / q + k * k) / a0;
    }

    shelf.fill (shelfFilter);
    highPass.fill (highPassFilter);

    forgetRecent();
    resetIntegrated();
}

void LoudnessMeter::resetIntegrated() noexcept
{
    gatingBlocks.clear();
    shortTermHistogram.clear();
    blocksIntegrated = 0;

    // The block in progress starts again too, so nothing from before is counted
    blockSum = 0.0;
    blockSamples = 0;

    integrated = noReading;
    range = noReading;
}

void LoudnessMeter::forgetRecent() noexcept
{
    // The filters are still ringing with what came before the gap
    for (auto* filters : { &shelf, &highPass })
        for (auto& filter : *filters)
            filter.z1 = filter.z2 = 0.0;

    blocks.fill (0.0);
    blockSum = 0.0;
    blockSamples = 0;

    // Only whole windows from after the gap are counted
    blocksIntegrated = 0;

    momentary = noReading;
    shortTerm = noReading;
}

//==============================================================================
void LoudnessMeter::process (const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = juce::jmin (numChannels, buffer.getNumChannels());
    const auto numSamples = buffer.getNumSamples();

    for (int start = 0; start < numSamples;)
    {
        const auto length = juce::jmin (numSamples - start, samplesPerBlock - blockSamples);

        for (int channel = 0; channel < channels; ++channel)
        {
            auto& first = shelf[static_cast<size_t> (channel)];
            auto& second = highPass[static_cast<size_t> (channel)];
            const auto* samples = buffer.getReadPointer (channel, start);
            double sum = 0.0;

            for (int i = 0; i < length; ++i)
            {
                const auto weighted = second.process (first.process (static_cast<double> (samples[i])));
                sum += weighted * weighted;
            }

            blockSum += sum;
        }

        blockSamples += length;
        start += length;

        if (blockSamples == samplesPerBlock)
            finishBlock();
    }

    // Silence would otherwise leave the filters ringing in denormals, and a NaN would stay for good
    for (auto* filters : { &shelf, &highPass })
        for (auto& filter : *filters)
            for (auto* state : { &filter.z1, &filter.z2 })
                // Far below anything audible, and well above where floats turn denormal
                if (! (std::abs (*state) >= 1.0e-30) || ! std::isfinite (*state))
                    *state = 0.0;

    if (! std::isfinite (blockSum))
        blockSum = 0.0;
}

void LoudnessMeter::finishBlock() noexcept
{
    if (! std::isfinite (blockSum))
        blockSum = 0.0;

    blocks[static_cast<size_t> (blockIndex)] = blockSum / samplesPerBlock;
    blockIndex = (blockIndex + 1) % shortTermBlocks;
    blockSum = 0.0;
    blockSamples = 0;

    const auto momentaryPower = meanOfLast (momentaryBlocks);
    const auto shortTermPower = meanOfLast (shortTermBlocks);

    momentary = static_cast<float> (toLoudness (momentaryPower));
    shortTerm = static_cast<float> (toLoudness (shortTermPower));

    if (! integrating)
        return;

    blocksIntegrated = juce::jmin (blocksIntegrated + 1, shortTermBlocks);

    // Whole windows only: 400 ms blocks overlapping by 75%, and 3 s ones every 100 ms
    if (blocksIntegrated >= momentaryBlocks)
    {
        gatingBlocks.add (momentaryPower);
        updateIntegrated();
    }

    if (blocksIntegrated >= shortTermBlocks)
    {
        shortTermHistogram.add (shortTermPower);
        updateRange();
    }
}

// Blocks not yet heard count as silence
double LoudnessMeter::meanOfLast (int numBlocks) const noexcept
{
    double sum = 0.0;

    for (int i = 1; i <= numBlocks; ++i)
        sum += blocks[static_cast<size_t> ((blockIndex - i + shortTermBlocks) % shortTermBlocks)];

    return sum / numBlocks;
}

// The mean of the blocks within 10 LU of the mean of all those over -70 LUFS. Both gates are BS.1770's:
// the first leaves out silence, the second quiet passages, so neither pulls a programme's loudness down
void LoudnessMeter::updateIntegrated() noexcept
{
    const auto ungated = gatingBlocks.meanFrom (Histogram::bottom);

    if (ungated <= 0.0)
    {
        integrated = noReading;
        return;
    }

    integrated = static_cast<float> (toLoudness (gatingBlocks.meanFrom (toLoudness (ungated) - 10.0)));
}

// The spread between the 10th and 95th percentiles of the short-term loudness, of the blocks
// within 20 LU of the mean of all those over -70 LUFS. The gates and percentiles are EBU Tech 3342's:
// the top 5% is left out so one loud moment doesn't set the range, and the bottom 10% for fades
void LoudnessMeter::updateRange() noexcept
{
    const auto ungated = shortTermHistogram.meanFrom (Histogram::bottom);

    if (ungated <= 0.0)
    {
        range = noReading;
        return;
    }

    const auto firstBin = static_cast<size_t> (Histogram::binFor (toLoudness (ungated) - 20.0));
    juce::uint64 total = 0;

    for (auto bin = firstBin; bin < Histogram::numBins; ++bin)
        total += shortTermHistogram.counts[bin];

    if (total == 0)
    {
        range = noReading;
        return;
    }

    // The bin holding the block a proportion of the way up, the quietest first
    const auto percentile = [&] (double proportion) -> double
    {
        const auto target = static_cast<juce::uint64> (std::llround (proportion * static_cast<double> (total - 1)));
        juce::uint64 seen = 0;

        for (auto bin = firstBin; bin < Histogram::numBins; ++bin)
        {
            seen += shortTermHistogram.counts[bin];

            if (seen > target)
                return static_cast<double> (bin);
        }

        return static_cast<double> (Histogram::numBins - 1);
    };

    // Bins are a tenth of a LU
    range = static_cast<float> ((percentile (0.95) - percentile (0.10)) / 10.0);
}
