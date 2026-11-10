#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "BeatPulse.h"
#include "LoudnessMeter.h"
#include "SpectrumAnalyser.h"
#include "StereoField.h"
#include "TruePeak.h"
#include "../shared/MeterFrame.h"
#include "../shared/Transport.h"

//==============================================================================
/**
 * Everything the meter measures. process() takes the audio on the audio thread, without
 * changing it; updateFrame() turns what it heard into a MeterFrame for one editor to draw,
 * on that editor's thread. The plugin and the web demo's engine share it.
 *
 * Starting playback starts the integrated loudness, the range, the held peaks and the clip
 * count again, and they stop counting while the host is stopped. With no host transport they
 * always count.
 *
 * A host that stops sending audio stops process(), so updateFrame() lets the display fall away
 * by itself, and shows a reset that process() hasn't yet been able to make.
 */
class MeterEngine
{
public:
    static constexpr int maxChannels = 2;

    // The longest time one updateFrame() moves the meter on by: a stalled editor catches up over
    // a few frames rather than in one jump
    static constexpr double maxFrameSeconds = 0.25;

    MeterEngine();

    /** Call before processing, and whenever the sample rate or layout changes. */
    void prepare (double newSampleRate, int numChannelsToMeter);

    /** For the audio thread. */
    void process (const juce::AudioBuffer<float>& buffer, const Transport& transport) noexcept;

    /** From any thread: starts them again, as starting playback does, on the next process(). */
    void requestReset() noexcept;

    /** For one reader thread: the meter now, given how long it's been since the last call. */
    const MeterFrame::Data& updateFrame (double seconds) noexcept;

    /** For the reader thread, as an editor opens: drops audio that arrived while none was reading. */
    void discardPending() noexcept;

private:
    // What's kept for the reader: two thirds of a second at 48 kHz, which is many frames' worth and
    // holds the largest block a host is likely to send. The rest of a larger one is dropped
    static constexpr int fifoSize = 1 << 15;

    // Peaks fall this fast: slow enough to read a peak, quick enough to see the next
    static constexpr float peakFallDbPerSecond = 20.0f;

    // Audio that stops arriving counts as silence after this long, or after two of the largest
    // block the host has sent if that's longer, so the gaps between large blocks aren't taken for it
    static constexpr double idleSeconds = 0.15;

    //==============================================================================
    // The audio thread's
    LoudnessMeter loudness;
    TruePeak truePeak;
    bool wasPlaying = false;

    //==============================================================================
    // Between the threads
    std::atomic<bool> pendingReset { false };
    std::atomic<bool> readerReset { false };
    std::atomic<double> sampleRate { 48000.0 };
    std::atomic<int> numChannels { maxChannels };
    std::atomic<int> largestBlock { 0 };

    // Set by the reader once the audio stops, for process() to start the live loudness again
    std::atomic<bool> audioStopped { false };
    std::atomic<bool> integrating { true };

    juce::AbstractFifo fifo { fifoSize };
    std::array<std::array<float, fifoSize>, maxChannels> fifoData {};

    //==============================================================================
    // The reader thread's
    SpectrumAnalyser spectrum;
    StereoField stereo;
    BeatPulse beat;
    double readerRate = 0.0;
    double idleFor = 0.0;
    bool idle = false;
    bool resetShown = false;
    std::array<float, maxChannels> peakDb { MeterFrame::peakFloorDb, MeterFrame::peakFloorDb };
    std::array<std::array<float, fifoSize>, maxChannels> incoming {};
    std::array<float, fifoSize> mid {};
    MeterFrame::Data frame {};

    JUCE_DECLARE_NON_COPYABLE (MeterEngine)
};
