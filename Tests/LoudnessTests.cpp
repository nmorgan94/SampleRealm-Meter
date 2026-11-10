#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"
#include "dsp/LoudnessMeter.h"
#include "dsp/TruePeak.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

namespace
{
    template <typename Meter>
    void feed (Meter& meter, const juce::AudioBuffer<float>& audio)
    {
        inBlocks (audio, [&meter] (juce::AudioBuffer<float>& block) { meter.process (block); });
    }
}

//==============================================================================
// The signals and tolerances of EBU Tech 3341, made here rather than read from its files
TEST_CASE ("A stereo 1 kHz sine reads its level in LUFS on every loudness, at any sample rate", "[loudness]")
{
    const auto rate = GENERATE (44100.0, 48000.0, 96000.0);
    const auto level = GENERATE (-23.0f, -33.0f);
    CAPTURE (rate, level);

    LoudnessMeter meter;
    meter.prepare (rate, 2);
    feed (meter, makeTone (2, 20.0, 1000.0, dbToGain (level), rate));

    CHECK_THAT (meter.getMomentary(), WithinAbs (level, 0.1));
    CHECK_THAT (meter.getShortTerm(), WithinAbs (level, 0.1));
    CHECK_THAT (meter.getIntegrated(), WithinAbs (level, 0.1));
    CHECK_THAT (meter.getRange(), WithinAbs (0.0, 0.2));
}

TEST_CASE ("A mono signal reads 3 LU under the same signal in both channels of a stereo one", "[loudness]")
{
    LoudnessMeter meter;
    meter.prepare (sampleRate, 1);
    feed (meter, makeTone (1, 5.0, 1000.0, dbToGain (-23.0f)));

    CHECK_THAT (meter.getIntegrated(), WithinAbs (-26.01, 0.1));
}

TEST_CASE ("The integrated loudness leaves out the quiet parts", "[loudness]")
{
    LoudnessMeter meter;
    meter.prepare (sampleRate, 2);

    SECTION ("More than 10 LU under the rest")
    {
        feed (meter, makeTone (2, 10.0, 1000.0, dbToGain (-36.0f)));
        feed (meter, makeTone (2, 60.0, 1000.0, dbToGain (-23.0f)));
        feed (meter, makeTone (2, 10.0, 1000.0, dbToGain (-36.0f)));

        CHECK_THAT (meter.getIntegrated(), WithinAbs (-23.0, 0.1));
    }

    SECTION ("Under -70 LUFS")
    {
        feed (meter, makeTone (2, 10.0, 1000.0, dbToGain (-23.0f)));
        feed (meter, juce::AudioBuffer<float> (makeConstantBuffer (2, 48000 * 30, 0.0f)));

        CHECK_THAT (meter.getIntegrated(), WithinAbs (-23.0, 0.1));
        CHECK_THAT (meter.getMomentary(), WithinAbs (LoudnessMeter::noReading, 0.0));
    }
}

// EBU Tech 3342
TEST_CASE ("The loudness range is the spread of the short-term loudness", "[loudness]")
{
    const auto [first, second, expected] = GENERATE (table<float, float, float> ({ { -20.0f, -30.0f, 10.0f },
                                                                                   { -20.0f, -15.0f, 5.0f },
                                                                                   { -40.0f, -20.0f, 20.0f } }));
    CAPTURE (first, second);

    LoudnessMeter meter;
    meter.prepare (sampleRate, 2);

    // No reading until there has been 3 s to measure
    feed (meter, makeTone (2, 2.0, 1000.0, dbToGain (first)));
    CHECK_THAT (meter.getRange(), WithinAbs (LoudnessMeter::noReading, 0.0));

    feed (meter, makeTone (2, 18.0, 1000.0, dbToGain (first)));
    feed (meter, makeTone (2, 20.0, 1000.0, dbToGain (second)));

    CHECK_THAT (meter.getRange(), WithinAbs (expected, 1.0));
}

TEST_CASE ("The integrated loudness stops while it isn't counting, and starts again on a reset", "[loudness]")
{
    LoudnessMeter meter;
    meter.prepare (sampleRate, 2);
    CHECK_THAT (meter.getIntegrated(), WithinAbs (LoudnessMeter::noReading, 0.0));

    feed (meter, makeTone (2, 5.0, 1000.0, dbToGain (-23.0f)));
    CHECK_THAT (meter.getIntegrated(), WithinAbs (-23.0, 0.1));

    meter.setIntegrating (false);
    feed (meter, makeTone (2, 5.0, 1000.0, dbToGain (-10.0f)));
    CHECK_THAT (meter.getIntegrated(), WithinAbs (-23.0, 0.1));
    CHECK_THAT (meter.getShortTerm(), WithinAbs (-10.0, 0.1));   // the others carry on

    meter.setIntegrating (true);
    meter.resetIntegrated();
    CHECK_THAT (meter.getIntegrated(), WithinAbs (LoudnessMeter::noReading, 0.0));

    feed (meter, makeTone (2, 5.0, 1000.0, dbToGain (-10.0f)));
    CHECK_THAT (meter.getIntegrated(), WithinAbs (-10.0, 0.1));
}

//==============================================================================
TEST_CASE ("The true peak finds a peak between the samples", "[truepeak]")
{
    TruePeak meter;
    meter.prepare (sampleRate, 2);

    // A quarter of the sample rate, shifted so every sample is 3 dB under the sine's peak of -1 dB.
    // It fades in, as a sudden start has peaks of its own
    juce::AudioBuffer<float> audio (2, 4800);

    for (int i = 0; i < audio.getNumSamples(); ++i)
    {
        const auto level = dbToGain (-1.0f) * juce::jmin (1.0f, static_cast<float> (i) / 1000.0f);
        const auto sample = level * static_cast<float> (std::sin (juce::MathConstants<double>::halfPi * i + juce::MathConstants<double>::pi / 4.0));
        audio.setSample (0, i, sample);
        audio.setSample (1, i, 0.5f * sample);
    }

    CHECK_THAT (juce::Decibels::gainToDecibels (audio.getMagnitude (0, 0, audio.getNumSamples())), WithinAbs (-4.01, 0.01));
    feed (meter, audio);

    // Within EBU Tech 3341's +0.2 and -0.4 dB
    const auto left = juce::Decibels::gainToDecibels (meter.readAndReset (0));
    const auto right = juce::Decibels::gainToDecibels (meter.readAndReset (1));
    CHECK (left > -1.4f);
    CHECK (left < -0.8f);
    CHECK (right > -7.42f);
    CHECK (right < -6.82f);

    // The loudest of the channels is held, and each channel starts again once read
    CHECK_THAT (meter.getMax(), WithinAbs (juce::Decibels::decibelsToGain (left), 1.0e-6));
    CHECK_THAT (meter.readAndReset (0), WithinAbs (0.0, 0.0));
    CHECK (meter.getClipCount() == 0);
}

TEST_CASE ("Clips are counted once each, however long they last", "[truepeak]")
{
    TruePeak meter;
    meter.prepare (sampleRate, 2);

    const auto loud = makeTone (2, 0.3, 100.0, 1.5f);
    const auto quiet = makeTone (2, 0.3, 100.0, 0.5f);

    feed (meter, quiet);
    CHECK (meter.getClipCount() == 0);

    // The sine goes over many times in each burst
    for (int burst = 1; burst <= 3; ++burst)
    {
        feed (meter, loud);
        feed (meter, quiet);
        CHECK (meter.getClipCount() == burst);
    }

    CHECK_THAT (juce::Decibels::gainToDecibels (meter.getMax()), WithinAbs (3.52, 0.1));

    meter.resetHeld();
    CHECK (meter.getClipCount() == 0);
    CHECK_THAT (meter.getMax(), WithinAbs (0.0, 0.0));
}

TEST_CASE ("A signal sitting on full scale isn't a clip, and one that isn't a number is the loudest there is", "[truepeak]")
{
    TruePeak meter;
    meter.prepare (sampleRate, 2);

    // Rising gently to it: a sudden jump to full scale does go over between the samples
    auto rising = makeConstantBuffer (2, 9600, 1.0f);
    rising.applyGainRamp (0, 4800, 0.0f, 1.0f);
    feed (meter, rising);
    CHECK (meter.getClipCount() == 0);
    CHECK_THAT (meter.readAndReset (0), WithinAbs (1.0, 1.0e-4));

    auto broken = makeConstantBuffer (2, 64, 0.1f);
    broken.setSample (0, 10, std::numeric_limits<float>::infinity());
    broken.setSample (0, 11, std::numeric_limits<float>::quiet_NaN());
    feed (meter, broken);
    CHECK (meter.getClipCount() == 1);
    CHECK (meter.readAndReset (0) > 1.0e30f);

    // And it doesn't linger
    feed (meter, makeConstantBuffer (2, 4800, 0.1f));
    meter.readAndReset (0);
    feed (meter, makeConstantBuffer (2, 4800, 0.1f));
    CHECK_THAT (meter.readAndReset (0), WithinAbs (0.1, 1.0e-4));
}
