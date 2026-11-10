#include <juce_audio_basics/juce_audio_basics.h>
#include "shared/ParameterSpecs.h"
#include "shared/UiInfoJson.h"
#include "dsp/MeterEngine.h"

// BuildCommit.cpp
extern const char* const buildCommit;

//==============================================================================
namespace
{
    constexpr int maxBlockSize = 128;
    constexpr int maxChannels = MeterEngine::maxChannels;

    constexpr auto numParams = static_cast<int> (ParameterSpecs::all.size());

    auto defaultValues()
    {
        std::array<float, ParameterSpecs::all.size()> values;

        for (size_t i = 0; i < values.size(); ++i)
            values[i] = ParameterSpecs::all[i]->defaultValue;

        return values;
    }

    auto values = defaultValues();

    MeterEngine meter;
    juce::AudioBuffer<float> buffer { maxChannels, maxBlockSize };
    int numChannels = maxChannels;
    double currentSampleRate = 48000.0;
    Transport transport;
}

#define EXPORT(name) extern "C" __attribute__ ((export_name (#name)))

EXPORT (uiInfo) const char* uiInfo()
{
    static const auto json = uiInfoJson (PLUGIN_VERSION, buildCommit);
    return json.c_str();
}

// As isBusesLayoutSupported: mono or stereo, the same in and out
EXPORT (prepare) void prepare (double sampleRate, int channels)
{
    numChannels = std::clamp (channels, 1, maxChannels);
    currentSampleRate = sampleRate;
    buffer.clear();
    meter.prepare (sampleRate, numChannels);
}

// The parameters only change how the editor looks, so the engine just keeps them
EXPORT (setParam) void setParam (int index, float value)
{
    if (index >= 0 && index < numParams)
        values[static_cast<size_t> (index)] = value;
}

// The page plays the host: render moves the position on while playing
EXPORT (setTransport) void setTransport (double bpm, bool playing, double ppq)
{
    transport = { bpm, playing, ppq, true };
}

// Write the input here, and read the output back after render
EXPORT (channel) float* channel (int index)
{
    return buffer.getWritePointer (std::clamp (index, 0, maxChannels - 1));
}

EXPORT (render) void render (int numSamples)
{
    juce::AudioBuffer<float> block { buffer.getArrayOfWritePointers(), numChannels, std::clamp (numSamples, 0, maxBlockSize) };

    // A meter only listens: the block is left as it was written
    meter.process (block, transport);
    transport.advance (block.getNumSamples(), currentSampleRate);
}

// The meter now as MeterFrame::size floats, given how long it's been since the last call
EXPORT (frame) const float* frame (double seconds)
{
    return meter.updateFrame (seconds).data();
}

EXPORT (frameSize) int frameSize()
{
    return MeterFrame::size;
}

// Starts the integrated loudness and the held peaks again, on the next render
EXPORT (resetMeters) void resetMeters()
{
    meter.requestReset();
}
