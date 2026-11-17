#include <juce_audio_basics/juce_audio_basics.h>
#include <samplerealm/BuildCommit.h>
#include <samplerealm/ParameterValues.h>
#include <samplerealm/UiInfoJson.h>
#include <samplerealm/WasmExport.h>
#include "shared/ParameterSpecs.h"
#include "shared/MeterInfoJson.h"
#include "dsp/MeterEngine.h"

//==============================================================================
namespace
{
    constexpr int maxBlockSize = 128;
    constexpr int maxChannels = MeterEngine::maxChannels;

    ParameterValues values { ParameterSpecs::all };

    MeterEngine meter;
    juce::AudioBuffer<float> buffer { maxChannels, maxBlockSize };
    int numChannels = maxChannels;
    Transport transport;
}

WASM_EXPORT (uiInfo) const char* uiInfo()
{
    static const auto json = uiInfoJson (ParameterSpecs::all, PLUGIN_VERSION, buildCommit);
    return json.c_str();
}

// The frame's layout, the scales and the themes, beside uiInfo
WASM_EXPORT (meterInfo) const char* meterInfo()
{
    static const auto json = meterInfoJson();
    return json.c_str();
}

// As isBusesLayoutSupported: mono or stereo, the same in and out
WASM_EXPORT (prepare) void prepare (double sampleRate, int channels)
{
    numChannels = std::clamp (channels, 1, maxChannels);
    buffer.clear();
    meter.prepare (sampleRate, numChannels);
}

// The parameters only change how the editor looks, so the engine just keeps them
WASM_EXPORT (setParam) void setParam (int index, float value)
{
    values.set (index, value);
}

// The page plays the host. The meter only reads whether it's playing
WASM_EXPORT (setTransport) void setTransport (double bpm, bool playing, double ppq)
{
    transport = { bpm, playing, ppq, true, playing };
}

// Write the input here, and read the output back after render
WASM_EXPORT (channel) float* channel (int index)
{
    return buffer.getWritePointer (std::clamp (index, 0, maxChannels - 1));
}

WASM_EXPORT (render) void render (int numSamples)
{
    juce::AudioBuffer<float> block { buffer.getArrayOfWritePointers(), numChannels, std::clamp (numSamples, 0, maxBlockSize) };

    // A meter only listens: the block is left as it was written
    meter.process (block, transport);
}

// The meter now as MeterFrame::size floats, given how long it's been since the last call
WASM_EXPORT (frame) const float* frame (double seconds)
{
    return meter.updateFrame (seconds).data();
}

WASM_EXPORT (frameSize) int frameSize()
{
    return MeterFrame::size;
}

// Starts the integrated loudness and the held peaks again, on the next render
WASM_EXPORT (resetMeters) void resetMeters()
{
    meter.requestReset();
}
