#pragma once

#include <PluginTestHelpers.h>

#include "PluginEditor.h"

//==============================================================================
namespace TestHelpers
{
    using ProcessorFixture = PreparedProcessor<AudioPluginAudioProcessor>;

    // A sine in every channel, at any sample rate
    inline juce::AudioBuffer<float> makeTone (int numChannels, double seconds, double frequency, float amplitude, double rate = sampleRate)
    {
        juce::AudioBuffer<float> buffer (numChannels, juce::roundToInt (seconds * rate));
        const auto increment = juce::MathConstants<double>::twoPi * frequency / rate;

        for (int channel = 0; channel < numChannels; ++channel)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                buffer.setSample (channel, i, amplitude * static_cast<float> (std::sin (increment * i)));

        return buffer;
    }

    inline float dbToGain (float db)   { return juce::Decibels::decibelsToGain (db); }

    // Feeds audio a block at a time, as a host does
    template <typename Process>
    void inBlocks (const juce::AudioBuffer<float>& audio, Process&& process)
    {
        for (int start = 0; start < audio.getNumSamples(); start += blockSize)
        {
            const auto length = juce::jmin (blockSize, audio.getNumSamples() - start);
            juce::AudioBuffer<float> block (const_cast<float* const*> (audio.getArrayOfReadPointers()), audio.getNumChannels(), start, length);
            process (block);
        }
    }
}
