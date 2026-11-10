#pragma once

#include <array>
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/**
 * The true peak of each channel, as ITU-R BS.1770-4: the loudest point of the signal at
 * four times its sample rate, which catches peaks between samples. Also counts clips.
 * process() runs on the audio thread; the rest can be called from any thread.
 */
class TruePeak
{
public:
    static constexpr int maxChannels = 2;

    TruePeak();

    void prepare (double sampleRate, int numChannelsToMeter);

    /** Channels beyond the prepared count are ignored. */
    void process (const juce::AudioBuffer<float>& buffer) noexcept;

    /** The highest peak on a channel since the last call, as linear gain. */
    float readAndReset (int channel) noexcept;

    /** The highest peak on any channel since the last resetHeld(), as linear gain. */
    float getMax() const noexcept      { return maxPeak.load(); }

    /** How many times the signal has gone over 0 dBTP. Overs within 100 ms count once. */
    int getClipCount() const noexcept  { return clipCount.load(); }

    /** Call from the audio thread, or while it isn't processing. */
    void resetHeld() noexcept;

private:
    // BS.1770 asks for at least four times the sample rate, and its example filter has 12 taps for
    // each of the four phases: enough to read within a few tenths of a dB at little cost
    static constexpr int factor = 4, tapsPerPhase = 12;

    // A hundredth of a dB over full scale: the filter is only that exact, and a signal sitting
    // on full scale isn't a clip
    static constexpr float clipThreshold = 1.001f;

    // The interpolating filter's taps for each of the points between two samples
    std::array<std::array<float, tapsPerPhase>, factor - 1> phases {};

    // Each channel's last samples, written twice so any run of them is contiguous
    std::array<std::array<float, 2 * tapsPerPhase>, maxChannels> history {};
    int historyIndex = 0;

    int numChannels = maxChannels;
    int clipHoldSamples = 4800, samplesSinceClip = 4800;

    // After a reset, until the history holds only what came after it
    int samplesToSkip = 0;

    std::array<std::atomic<float>, maxChannels> pending {};
    std::atomic<float> maxPeak { 0.0f };
    std::atomic<int> clipCount { 0 };

    JUCE_DECLARE_NON_COPYABLE (TruePeak)
};
