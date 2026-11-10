#include "TruePeak.h"

//==============================================================================
TruePeak::TruePeak()
{
    // A windowed sinc cutting off at the original Nyquist. The phase that lands on the samples
    // themselves is a plain delay, so only the other three need taps
    constexpr auto numTaps = factor * tapsPerPhase;
    constexpr auto centre = numTaps / 2;
    // A Kaiser window's shape. 7 was chosen by the tests: the true peak of a sine between the
    // samples reads within EBU Tech 3341's +0.2 and -0.4 dB
    constexpr auto beta = 7.0;

    const auto bessel = [] (double x)
    {
        double sum = 1.0, term = 1.0;

        for (int k = 1; k < 32; ++k)
        {
            term *= (x / (2.0 * k)) * (x / (2.0 * k));
            sum += term;
        }

        return sum;
    };

    for (int phase = 1; phase < factor; ++phase)
    {
        auto& taps = phases[static_cast<size_t> (phase - 1)];
        double sum = 0.0;

        for (int k = 0; k < tapsPerPhase; ++k)
        {
            const auto n = phase + factor * k;
            const auto x = juce::MathConstants<double>::pi * (n - centre) / factor;
            const auto position = static_cast<double> (n - centre) / centre;
            const auto window = bessel (beta * std::sqrt (juce::jmax (0.0, 1.0 - position * position))) / bessel (beta);
            const auto tap = std::sin (x) / x * window;

            taps[static_cast<size_t> (k)] = static_cast<float> (tap);
            sum += tap;
        }

        // A steady signal comes through each phase at its own level
        for (auto& tap : taps)
            tap = static_cast<float> (tap / sum);
    }
}

void TruePeak::prepare (double sampleRate, int numChannelsToMeter)
{
    numChannels = juce::jlimit (1, maxChannels, numChannelsToMeter);
    // 100 ms: one clipped note goes over on every cycle, and should count once
    clipHoldSamples = juce::roundToInt (sampleRate / 10.0);
    samplesSinceClip = clipHoldSamples;
    historyIndex = 0;

    for (auto& channel : history)
        channel.fill (0.0f);

    for (auto& peak : pending)
        peak = 0.0f;

    resetHeld();
}

void TruePeak::resetHeld() noexcept
{
    maxPeak = 0.0f;
    clipCount = 0;
    samplesSinceClip = clipHoldSamples;
    samplesToSkip = tapsPerPhase;
}

//==============================================================================
void TruePeak::process (const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = juce::jmin (numChannels, buffer.getNumChannels());
    const auto numSamples = buffer.getNumSamples();
    std::array<float, maxChannels> blockPeaks {};
    auto loudest = 0.0f;
    auto newClips = 0;

    for (int i = 0; i < numSamples; ++i)
    {
        // Backwards, so the newest sample is first in each run
        historyIndex = (historyIndex + tapsPerPhase - 1) % tapsPerPhase;
        auto samplePeak = 0.0f;

        for (int channel = 0; channel < channels; ++channel)
        {
            auto& samples = history[static_cast<size_t> (channel)];
            auto input = buffer.getSample (channel, i);

            // Kept out of the history, where it would spoil the next samples too
            const auto finite = std::isfinite (input);

            if (! finite)
                input = 0.0f;

            samples[static_cast<size_t> (historyIndex)] = samples[static_cast<size_t> (historyIndex + tapsPerPhase)] = input;

            const auto* recent = samples.data() + historyIndex;
            auto peak = std::abs (input);

            for (const auto& taps : phases)
            {
                auto sum = 0.0f;

                for (size_t k = 0; k < tapsPerPhase; ++k)
                    sum += taps[k] * recent[k];

                peak = juce::jmax (peak, std::abs (sum));
            }

            // Infinities and NaNs read as the loudest peak there is
            if (! finite)
                peak = std::numeric_limits<float>::max();

            auto& blockPeak = blockPeaks[static_cast<size_t> (channel)];
            blockPeak = juce::jmax (blockPeak, peak);
            samplePeak = juce::jmax (samplePeak, peak);
        }

        if (samplesToSkip > 0)
        {
            --samplesToSkip;
            continue;
        }

        loudest = juce::jmax (loudest, samplePeak);

        if (samplePeak > clipThreshold)
        {
            if (samplesSinceClip >= clipHoldSamples)
                ++newClips;

            samplesSinceClip = 0;
        }
        else if (samplesSinceClip < clipHoldSamples)
        {
            ++samplesSinceClip;
        }
    }

    for (int channel = 0; channel < channels; ++channel)
    {
        const auto blockPeak = blockPeaks[static_cast<size_t> (channel)];

        auto& waiting = pending[static_cast<size_t> (channel)];
        auto current = waiting.load();
        while (blockPeak > current && ! waiting.compare_exchange_weak (current, blockPeak)) {}
    }

    if (loudest > maxPeak.load())
        maxPeak = loudest;

    if (newClips > 0)
        clipCount += newClips;
}

float TruePeak::readAndReset (int channel) noexcept
{
    jassert (juce::isPositiveAndBelow (channel, maxChannels));
    return pending[static_cast<size_t> (channel)].exchange (0.0f);
}
