#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"
#include "dsp/BeatPulse.h"
#include "dsp/MeterEngine.h"
#include "dsp/SpectrumAnalyser.h"
#include "dsp/StereoField.h"
#include "shared/MeterScales.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

namespace
{
    constexpr double frameSeconds = 1.0 / 60.0;

    int bandOf (float hz)
    {
        return static_cast<int> (std::floor (MeterFrame::numBands * MeterScales::acrossSpectrum (hz)));
    }

    std::vector<float> noise (int numSamples, juce::int64 seed)
    {
        juce::Random random (seed);
        std::vector<float> samples (static_cast<size_t> (numSamples));

        for (auto& sample : samples)
            sample = random.nextFloat() - 0.5f;

        return samples;
    }

    // The engine with a host that reads a frame after every block
    struct EngineFixture
    {
        EngineFixture()   { engine.prepare (sampleRate, 2); }

        const MeterFrame::Data& play (const juce::AudioBuffer<float>& audio, const Transport& transport = {})
        {
            inBlocks (audio, [&] (juce::AudioBuffer<float>& block)
            {
                engine.process (block, transport);
                engine.updateFrame (static_cast<double> (block.getNumSamples()) / sampleRate);
            });

            return engine.updateFrame (0.0);
        }

        MeterEngine engine;
    };

    // A waveform column's low, mid and high peaks, counting back from the newest at 0
    std::array<float, MeterFrame::waveformBands> columnsAgo (const MeterFrame::Data& frame, int ago)
    {
        std::array<float, MeterFrame::waveformBands> peaks;
        std::copy_n (frame.begin() + MeterFrame::waveformOffset + MeterFrame::waveformBands * (MeterFrame::numColumns - 1 - ago), peaks.size(), peaks.begin());
        return peaks;
    }

    float loudestOf (const std::array<float, MeterFrame::waveformBands>& peaks)
    {
        return *std::max_element (peaks.begin(), peaks.end());
    }

    const Transport stopped { 120.0, false, 0.0, true };
    const Transport playing { 120.0, true, 8.25, true, true };
    const Transport playingWithNoPosition { 120.0, false, 0.0, true, true };
}

//==============================================================================
TEST_CASE ("A full-scale 1 kHz sine reads 0 dB in its band and nothing far from it, at any sample rate", "[spectrum]")
{
    const auto rate = GENERATE (44100.0, 48000.0, 96000.0, 192000.0);
    CAPTURE (rate);

    SpectrumAnalyser spectrum;
    spectrum.setSampleRate (rate);

    const auto tone = makeTone (1, 1.0, 1000.0, 1.0f, rate);
    spectrum.push (tone.getReadPointer (0), tone.getNumSamples());

    for (int i = 0; i < 30; ++i)
        spectrum.update (frameSeconds);

    const auto& bands = spectrum.getBands();
    const auto loudest = static_cast<int> (std::max_element (bands.begin(), bands.end()) - bands.begin());

    CHECK (loudest == bandOf (1000.0f));
    CHECK_THAT (bands[static_cast<size_t> (loudest)], WithinAbs (0.0, 1.5));
    CHECK (bands[static_cast<size_t> (bandOf (100.0f))] < -60.0f);
    CHECK (bands[static_cast<size_t> (bandOf (10000.0f))] < -60.0f);
}

TEST_CASE ("Bands above half the sample rate read nothing", "[spectrum]")
{
    // 15 kHz is under half of 32 kHz, and the bands from 16 kHz up are over it
    SpectrumAnalyser spectrum;
    spectrum.setSampleRate (32000.0);

    const auto tone = makeTone (1, 1.0, 15000.0, 1.0f, 32000.0);
    spectrum.push (tone.getReadPointer (0), tone.getNumSamples());
    spectrum.update (1.0);

    CHECK (spectrum.getBands()[static_cast<size_t> (bandOf (15000.0f))] > -10.0f);
    CHECK_THAT (spectrum.getBands()[static_cast<size_t> (bandOf (17000.0f))], WithinAbs (SpectrumAnalyser::floorDb, 0.0));
    CHECK_THAT (spectrum.getBands().back(), WithinAbs (SpectrumAnalyser::floorDb, 0.0));
}

TEST_CASE ("The spectrum tilts up 4.5 dB an octave around 1 kHz", "[spectrum]")
{
    // From 116 Hz, where a band is narrower than the analysis can tell apart, to 13 kHz
    const auto band = GENERATE (24, 40, 70, 90);
    const auto hz = SpectrumAnalyser::centreOf (band);
    CAPTURE (band, hz);

    SpectrumAnalyser spectrum;
    const auto tone = makeTone (1, 1.0, hz, 1.0f);
    spectrum.push (tone.getReadPointer (0), tone.getNumSamples());

    for (int i = 0; i < 30; ++i)
        spectrum.update (frameSeconds);

    CHECK_THAT (spectrum.getBands()[static_cast<size_t> (band)], WithinAbs (4.5 * std::log2 (hz / 1000.0), 1.8));
    CHECK_THAT (SpectrumAnalyser::centreOf (bandOf (1000.0f)), WithinAbs (1000.0, 40.0));
}

TEST_CASE ("The spectrum's bands fall slowly, and its peaks hold until they're reset", "[spectrum]")
{
    SpectrumAnalyser spectrum;
    const auto band = static_cast<size_t> (bandOf (1000.0f));

    const auto tone = makeTone (1, 1.0, 1000.0, 1.0f);
    spectrum.push (tone.getReadPointer (0), tone.getNumSamples());

    for (int i = 0; i < 30; ++i)
        spectrum.update (frameSeconds);

    const auto level = spectrum.getBands()[band];

    // A second of silence
    const std::vector<float> silence (48000, 0.0f);
    spectrum.push (silence.data(), 48000);

    for (int i = 0; i < 60; ++i)
        spectrum.update (frameSeconds);

    CHECK_THAT (spectrum.getBands()[band], WithinAbs (level - 24.0f, 0.5));
    CHECK_THAT (spectrum.getPeaks()[band], WithinAbs (level, 1.0e-4));

    spectrum.resetPeaks();
    CHECK_THAT (spectrum.getPeaks()[band], WithinAbs (spectrum.getBands()[band], 0.0));

    for (int i = 0; i < 600; ++i)
        spectrum.update (frameSeconds);

    CHECK_THAT (spectrum.getBands()[band], WithinAbs (SpectrumAnalyser::floorDb, 0.0));
}

//==============================================================================
TEST_CASE ("The correlation and balance follow how the channels relate", "[stereo]")
{
    StereoField stereo;
    stereo.setSampleRate (sampleRate);

    const auto left = noise (48000, 1);
    auto right = left;

    SECTION ("The same in both channels")
    {
        stereo.push (left.data(), right.data(), 48000);
        CHECK_THAT (stereo.getCorrelation(), WithinAbs (1.0, 1.0e-3));
        CHECK_THAT (stereo.getBalance(), WithinAbs (0.0, 1.0e-3));
    }

    SECTION ("One channel inverted")
    {
        for (auto& sample : right)
            sample = -sample;

        stereo.push (left.data(), right.data(), 48000);
        CHECK_THAT (stereo.getCorrelation(), WithinAbs (-1.0, 1.0e-3));
    }

    SECTION ("Unrelated channels")
    {
        right = noise (48000, 2);
        stereo.push (left.data(), right.data(), 48000);
        CHECK_THAT (stereo.getCorrelation(), WithinAbs (0.0, 0.1));
    }

    SECTION ("Only the left channel")
    {
        std::fill (right.begin(), right.end(), 0.0f);
        stereo.push (left.data(), right.data(), 48000);
        CHECK_THAT (stereo.getBalance(), WithinAbs (-1.0, 1.0e-3));
        CHECK_THAT (stereo.getCorrelation(), WithinAbs (0.0, 1.0e-3));
    }

    SECTION ("Silence")
    {
        const std::vector<float> silence (48000, 0.0f);
        stereo.push (silence.data(), silence.data(), 48000);
        CHECK_THAT (stereo.getCorrelation(), WithinAbs (0.0, 0.0));
        CHECK_THAT (stereo.getBalance(), WithinAbs (0.0, 0.0));
    }
}

TEST_CASE ("The goniometer's points stand up for mono, lie down for out of phase, and lean to a channel", "[stereo]")
{
    StereoField stereo;
    stereo.setSampleRate (sampleRate);

    // Much quieter than full scale, which the points are scaled up from
    const auto level = GENERATE (0.5f, 0.05f);
    const auto tone = makeTone (1, 0.5, 220.0, level);
    const std::vector<float> silence (static_cast<size_t> (tone.getNumSamples()), 0.0f);
    std::vector<float> inverted (silence.size());

    for (size_t i = 0; i < inverted.size(); ++i)
        inverted[i] = -tone.getSample (0, static_cast<int> (i));

    std::array<float, 2 * StereoField::numPoints> points;

    const auto furthest = [&points] (int offset)
    {
        auto result = 0.0f;

        for (size_t i = 0; i < StereoField::numPoints; ++i)
            result = std::max (result, std::abs (points[2 * i + static_cast<size_t> (offset)]));

        return result;
    };

    SECTION ("Mono")
    {
        stereo.push (tone.getReadPointer (0), tone.getReadPointer (0), tone.getNumSamples());
        stereo.copyPoints (points.data());
        CHECK_THAT (furthest (0), WithinAbs (0.0, 1.0e-6));
        CHECK_THAT (furthest (1), WithinAbs (0.98, 0.02));
    }

    SECTION ("Out of phase")
    {
        stereo.push (tone.getReadPointer (0), inverted.data(), tone.getNumSamples());
        stereo.copyPoints (points.data());
        CHECK_THAT (furthest (0), WithinAbs (0.98, 0.02));
        CHECK_THAT (furthest (1), WithinAbs (0.0, 1.0e-6));
    }

    SECTION ("Left only")
    {
        stereo.push (tone.getReadPointer (0), silence.data(), tone.getNumSamples());
        stereo.copyPoints (points.data());

        // Every point as far left as it is up. A failure names the point furthest off
        size_t worst = 0;
        auto offTheDiagonal = 0.0f;

        for (size_t i = 0; i < StereoField::numPoints; ++i)
            if (const auto off = std::abs (points[2 * i] + points[2 * i + 1]); off > offTheDiagonal)
                offTheDiagonal = off, worst = i;

        CAPTURE (worst, points[2 * worst], points[2 * worst + 1]);
        CHECK_THAT (offTheDiagonal, WithinAbs (0.0, 1.0e-6));

        CHECK_THAT (furthest (0), WithinAbs (0.49, 0.02));
    }
}

//==============================================================================
TEST_CASE ("The beat's pulse jumps on a low hit and dies away, and a steady tone leaves it alone", "[beat]")
{
    BeatPulse beat;
    beat.setSampleRate (sampleRate);

    const auto frame = juce::roundToInt (sampleRate * frameSeconds);
    const std::vector<float> silence (static_cast<size_t> (frame), 0.0f);

    // A kick: a low sine dying away
    auto kick = makeTone (1, 0.25, 55.0, 0.8f);

    for (int i = 0; i < kick.getNumSamples(); ++i)
        kick.applyGain (0, i, 1, std::exp (-static_cast<float> (i) / 2400.0f));

    const auto play = [&] (const float* samples, int numSamples)
    {
        for (int start = 0; start < numSamples; start += frame)
            beat.push (samples + start, juce::jmin (frame, numSamples - start), frameSeconds);
    };

    for (int hit = 0; hit < 3; ++hit)
    {
        play (kick.getReadPointer (0), frame * 3);
        CHECK (beat.get() > 0.6f);

        play (kick.getReadPointer (0) + frame * 3, kick.getNumSamples() - frame * 3);

        for (int i = 0; i < 30; ++i)
            beat.push (silence.data(), frame, frameSeconds);

        CHECK (beat.get() < 0.1f);
    }

    // A held bass note settles to nothing
    const auto bass = makeTone (1, 3.0, 55.0, 0.8f);
    play (bass.getReadPointer (0), bass.getNumSamples());
    CHECK (beat.get() < 0.05f);
}

//==============================================================================
TEST_CASE_METHOD (EngineFixture, "The waveform shows a tone in its own band: low, mid or high", "[waveform]")
{
    const auto [hz, band] = GENERATE (table<double, size_t> ({ { 100.0, 0 }, { 1000.0, 1 }, { 8000.0, 2 } }));
    const auto peaks = columnsAgo (play (makeTone (2, 1.0, hz, 0.5f)), 0);

    for (size_t i = 0; i < peaks.size(); ++i)
    {
        CAPTURE (hz, i, peaks[i]);

        // At 8 kHz a peak falls between samples
        CHECK ((i == band ? peaks[i] > 0.4f && peaks[i] < 0.55f : peaks[i] < 0.1f));
    }
}

TEST_CASE_METHOD (EngineFixture, "The waveform scrolls by at 60 columns a second", "[waveform]")
{
    // Half a second of tone, then two seconds and half a column of silence
    const auto tone = makeTone (2, 0.5, 1000.0, 0.5f);
    juce::AudioBuffer<float> audio (2, tone.getNumSamples() + 2 * static_cast<int> (sampleRate) + 400);
    audio.clear();

    for (int channel = 0; channel < 2; ++channel)
        audio.copyFrom (channel, 0, tone, channel, 0, tone.getNumSamples());

    const auto& frame = play (audio);

    // The column after the tone has the filters ringing from its sudden end
    CHECK (loudestOf (columnsAgo (frame, 118)) < 0.01f);
    CHECK (loudestOf (columnsAgo (frame, 120)) > 0.4f);
    CHECK (loudestOf (columnsAgo (frame, 149)) > 0.4f);
    CHECK (loudestOf (columnsAgo (frame, 150)) < 0.01f);
    CHECK_THAT (frame[MeterFrame::waveformPhase], WithinAbs (0.5, 1.0e-6));
}

TEST_CASE ("The waveform is the same however often it's read", "[waveform]")
{
    const auto readEvery = [] (int blocks)
    {
        MeterEngine engine;
        engine.prepare (sampleRate, 2);
        auto count = 0;

        inBlocks (makeTone (2, 2.0, 90.0, 0.5f), [&] (juce::AudioBuffer<float>& block)
        {
            engine.process (block, {});

            if (++count % blocks == 0)
                engine.updateFrame (blocks * static_cast<double> (block.getNumSamples()) / sampleRate);
        });

        const auto& frame = engine.updateFrame (0.0);
        return std::vector<float> (frame.begin() + MeterFrame::waveformOffset, frame.end());
    };

    CHECK (readEvery (1) == readEvery (7));
}

//==============================================================================
TEST_CASE_METHOD (EngineFixture, "A frame holds everything the engine heard", "[engine]")
{
    const auto& frame = play (makeTone (2, 20.0, 1000.0, dbToGain (-23.0f)));

    CHECK_THAT (frame[MeterFrame::momentary], WithinAbs (-23.0, 0.1));
    CHECK_THAT (frame[MeterFrame::shortTerm], WithinAbs (-23.0, 0.1));
    CHECK_THAT (frame[MeterFrame::integrated], WithinAbs (-23.0, 0.1));
    CHECK_THAT (frame[MeterFrame::range], WithinAbs (0.0, 0.2));
    CHECK_THAT (frame[MeterFrame::truePeakLeft], WithinAbs (-23.0, 0.3));
    CHECK_THAT (frame[MeterFrame::truePeakRight], WithinAbs (-23.0, 0.3));
    CHECK_THAT (frame[MeterFrame::maxTruePeak], WithinAbs (-23.0, 0.1));
    CHECK_THAT (frame[MeterFrame::clipCount], WithinAbs (0.0, 0.0));
    CHECK_THAT (frame[MeterFrame::correlation], WithinAbs (1.0, 1.0e-3));
    CHECK_THAT (frame[MeterFrame::balance], WithinAbs (0.0, 1.0e-3));
    CHECK_THAT (frame[MeterFrame::integrating], WithinAbs (1.0, 0.0));

    const auto band = static_cast<size_t> (MeterFrame::bandsOffset + bandOf (1000.0f));
    CHECK_THAT (frame[band], WithinAbs (-23.0, 1.5));
    CHECK (frame[band + MeterFrame::numBands] >= frame[band]);

    // Mono: every point on the upright
    auto worst = 0;
    auto offTheUpright = 0.0f;

    for (int i = 0; i < MeterFrame::numPoints; ++i)
        if (const auto off = std::abs (frame[static_cast<size_t> (MeterFrame::pointsOffset + 2 * i)]); off > offTheUpright)
            offTheUpright = off, worst = i;

    CAPTURE (worst);
    CHECK_THAT (offTheUpright, WithinAbs (0.0, 1.0e-6));
}

TEST_CASE_METHOD (EngineFixture, "A mono buffer is metered as the same signal in both channels", "[engine]")
{
    engine.prepare (sampleRate, 1);
    const auto& frame = play (makeTone (1, 3.0, 1000.0, 0.5f));

    CHECK_THAT (frame[MeterFrame::truePeakLeft], WithinAbs (-6.02, 0.3));
    CHECK_THAT (frame[MeterFrame::truePeakRight], WithinAbs (frame[MeterFrame::truePeakLeft], 0.0));
    CHECK_THAT (frame[MeterFrame::correlation], WithinAbs (1.0, 1.0e-3));
    CHECK_THAT (frame[MeterFrame::momentary], WithinAbs (-9.03, 0.1));
}

TEST_CASE_METHOD (EngineFixture, "With a host transport, the integrated loudness counts only while playing and starts again on play", "[engine][transport]")
{
    const auto quiet = makeTone (2, 5.0, 1000.0, dbToGain (-23.0f));
    const auto loud = makeTone (2, 5.0, 1000.0, dbToGain (-10.0f));

    auto frame = play (quiet, playing);
    CHECK_THAT (frame[MeterFrame::integrated], WithinAbs (-23.0, 0.1));
    CHECK_THAT (frame[MeterFrame::integrating], WithinAbs (1.0, 0.0));

    // Stopped: the live readings move, the integrated one keeps what was measured
    frame = play (loud, stopped);
    CHECK_THAT (frame[MeterFrame::integrated], WithinAbs (-23.0, 0.1));
    CHECK_THAT (frame[MeterFrame::shortTerm], WithinAbs (-10.0, 0.1));
    CHECK_THAT (frame[MeterFrame::integrating], WithinAbs (0.0, 0.0));
    CHECK_THAT (frame[MeterFrame::maxTruePeak], WithinAbs (-10.0, 0.1));

    // Playing again starts from nothing
    frame = play (quiet, playing);
    CHECK_THAT (frame[MeterFrame::integrated], WithinAbs (-23.0, 0.1));
    CHECK_THAT (frame[MeterFrame::maxTruePeak], WithinAbs (-23.0, 0.1));
}

TEST_CASE_METHOD (EngineFixture, "A host whose transport has never played counts, through a prepare too", "[engine][transport]")
{
    auto frame = play (makeTone (2, 5.0, 1000.0, dbToGain (-23.0f)), stopped);
    CHECK_THAT (frame[MeterFrame::integrated], WithinAbs (-23.0, 0.1));
    CHECK_THAT (frame[MeterFrame::integrating], WithinAbs (1.0, 0.0));

    // One that has played still waits after preparing again
    play (makeTone (2, 1.0, 1000.0, dbToGain (-23.0f)), playing);
    engine.prepare (sampleRate, 2);
    frame = play (makeTone (2, 1.0, 1000.0, dbToGain (-23.0f)), stopped);
    CHECK_THAT (frame[MeterFrame::integrating], WithinAbs (0.0, 0.0));
}

TEST_CASE_METHOD (EngineFixture, "A host that plays without a beat position still counts", "[engine][transport]")
{
    const auto frame = play (makeTone (2, 5.0, 1000.0, dbToGain (-23.0f)), playingWithNoPosition);

    CHECK_THAT (frame[MeterFrame::integrated], WithinAbs (-23.0, 0.1));
    CHECK_THAT (frame[MeterFrame::integrating], WithinAbs (1.0, 0.0));
}

TEST_CASE_METHOD (EngineFixture, "A reset starts the integrated loudness, the held peaks and the clip count again", "[engine]")
{
    play (makeTone (2, 3.0, 100.0, 1.5f));
    auto frame = play (makeTone (2, 3.0, 1000.0, dbToGain (-30.0f)));

    CHECK (frame[MeterFrame::maxTruePeak] > 3.0f);
    CHECK_THAT (frame[MeterFrame::clipCount], WithinAbs (1.0, 0.0));
    CHECK (frame[MeterFrame::integrated] > -10.0f);

    const auto low = static_cast<size_t> (MeterFrame::peaksOffset + bandOf (100.0f));
    CHECK (frame[low] > -20.0f);

    engine.requestReset();
    frame = play (makeTone (2, 3.0, 1000.0, dbToGain (-30.0f)));

    CHECK_THAT (frame[MeterFrame::maxTruePeak], WithinAbs (-30.0, 0.1));
    CHECK_THAT (frame[MeterFrame::clipCount], WithinAbs (0.0, 0.0));
    CHECK (frame[low] < -60.0f);
    CHECK_THAT (frame[MeterFrame::integrated], WithinAbs (-30.0, 0.1));
}

TEST_CASE_METHOD (EngineFixture, "When the host stops sending audio, the display falls away", "[engine]")
{
    play (makeTone (2, 2.0, 1000.0, 0.5f));

    for (int i = 0; i < 600; ++i)
        engine.updateFrame (frameSeconds);

    const auto& frame = engine.updateFrame (frameSeconds);
    CHECK_THAT (frame[MeterFrame::truePeakLeft], WithinAbs (MeterFrame::peakFloorDb, 0.0));
    CHECK_THAT (frame[static_cast<size_t> (MeterFrame::bandsOffset + bandOf (1000.0f))], WithinAbs (MeterFrame::bandFloorDb, 0.0));

    // The first point still off the centre, if there is one
    const auto pointsEnd = frame.begin() + MeterFrame::waveformOffset;
    const auto moving = std::find_if (frame.begin() + MeterFrame::pointsOffset, pointsEnd, [] (float point) { return ! juce::exactlyEqual (point, 0.0f); });
    const auto movingIndex = moving - (frame.begin() + MeterFrame::pointsOffset);

    CAPTURE (movingIndex, moving != pointsEnd ? *moving : 0.0f);
    CHECK (moving == pointsEnd);

    // The waveform has scrolled out to flat
    CHECK (*std::max_element (frame.begin() + MeterFrame::waveformOffset, frame.end()) == 0.0f);

    // The live loudness goes too, though only process() measures it
    CHECK_THAT (frame[MeterFrame::momentary], WithinAbs (MeterFrame::noLoudness, 0.0));
    CHECK_THAT (frame[MeterFrame::shortTerm], WithinAbs (MeterFrame::noLoudness, 0.0));

    // What was measured stays
    CHECK_THAT (frame[MeterFrame::maxTruePeak], WithinAbs (-6.02, 0.1));
    CHECK_THAT (frame[MeterFrame::integrated], WithinAbs (-6.02, 0.1));

    // And the frame has stopped changing, so the editors stop drawing. Part of a waveform column later
    const auto settled = frame;
    CHECK (engine.updateFrame (0.01) == settled);
}

TEST_CASE_METHOD (EngineFixture, "The live loudness falls as it would through silence once the audio stops, and starts again when it's back", "[engine]")
{
    const auto tone = makeTone (2, 4.0, 1000.0, dbToGain (-20.0f));
    play (tone);

    // A second of nothing: the momentary loudness's 400 ms has gone, and a third of the short-term's 3 s
    for (int i = 0; i < 60; ++i)
        engine.updateFrame (frameSeconds);

    auto frame = engine.updateFrame (0.0);
    CHECK_THAT (frame[MeterFrame::momentary], WithinAbs (MeterFrame::noLoudness, 0.0));
    CHECK_THAT (frame[MeterFrame::shortTerm], WithinAbs (-20.0 + 10.0 * std::log10 (2.0 / 3.0), 0.2));

    // Back again: nothing from before the gap is in the new readings
    frame = play (makeTone (2, 0.25, 1000.0, dbToGain (-20.0f)));
    CHECK_THAT (frame[MeterFrame::momentary], WithinAbs (-23.0, 0.3));
}

TEST_CASE_METHOD (EngineFixture, "A reset shows at once while no audio is arriving", "[engine]")
{
    play (makeTone (2, 4.0, 100.0, 1.5f));

    for (int i = 0; i < 30; ++i)
        engine.updateFrame (frameSeconds);

    const auto low = static_cast<size_t> (MeterFrame::peaksOffset + bandOf (100.0f));
    auto frame = engine.updateFrame (frameSeconds);
    CHECK (frame[MeterFrame::integrated] > -10.0f);
    CHECK (frame[low] > -20.0f);

    engine.requestReset();
    frame = engine.updateFrame (frameSeconds);

    CHECK_THAT (frame[MeterFrame::integrated], WithinAbs (MeterFrame::noLoudness, 0.0));
    CHECK_THAT (frame[MeterFrame::range], WithinAbs (MeterFrame::noLoudness, 0.0));
    CHECK_THAT (frame[MeterFrame::maxTruePeak], WithinAbs (MeterFrame::noLoudness, 0.0));
    CHECK_THAT (frame[MeterFrame::clipCount], WithinAbs (0.0, 0.0));
    CHECK (frame[low] < frame[low - MeterFrame::numBands] + 0.01f);

    // And it's made for real once the audio is back
    frame = play (makeTone (2, 3.0, 1000.0, dbToGain (-30.0f)));
    CHECK_THAT (frame[MeterFrame::integrated], WithinAbs (-30.0, 0.1));
    CHECK_THAT (frame[MeterFrame::clipCount], WithinAbs (0.0, 0.0));
}

TEST_CASE_METHOD (EngineFixture, "The gaps between a host's large blocks aren't taken for the audio stopping", "[engine]")
{
    // 16384 samples at 48 kHz: a third of a second between blocks
    const auto tone = makeTone (2, 4.0, 1000.0, 0.5f);
    const auto largeBlock = 16384;
    const auto framesPerBlock = juce::roundToInt (largeBlock / sampleRate / frameSeconds);
    const auto band = static_cast<size_t> (MeterFrame::bandsOffset + bandOf (1000.0f));
    auto lowest = 0.0f;

    for (int start = 0; start + largeBlock <= tone.getNumSamples(); start += largeBlock)
    {
        juce::AudioBuffer<float> block (2, largeBlock);

        for (int channel = 0; channel < 2; ++channel)
            block.copyFrom (channel, 0, tone, channel, start, largeBlock);

        engine.process (block, {});

        for (int i = 0; i < framesPerBlock; ++i)
        {
            const auto& frame = engine.updateFrame (frameSeconds);

            if (start > 0)
                lowest = juce::jmin (lowest, frame[band], frame[MeterFrame::momentary]);
        }
    }

    // The band and the loudness hold between the blocks
    CHECK (lowest > -10.0f);
}

TEST_CASE_METHOD (EngineFixture, "With no editor reading, audio is still measured, and the backlog can be dropped", "[engine]")
{
    // Far more than the engine keeps for a reader
    inBlocks (makeTone (2, 5.0, 1000.0, 0.5f), [this] (juce::AudioBuffer<float>& block) { engine.process (block, {}); });

    engine.discardPending();
    const auto& frame = engine.updateFrame (frameSeconds);

    CHECK_THAT (frame[MeterFrame::integrated], WithinAbs (-6.02, 0.1));
    CHECK_THAT (frame[static_cast<size_t> (MeterFrame::bandsOffset + bandOf (1000.0f))], WithinAbs (MeterFrame::bandFloorDb, 0.0));
}
