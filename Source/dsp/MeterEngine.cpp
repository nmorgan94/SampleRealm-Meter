#include "MeterEngine.h"

//==============================================================================
MeterEngine::MeterEngine()
{
    prepare (sampleRate.load(), maxChannels);
    updateFrame (0.0);
}

void MeterEngine::prepare (double newSampleRate, int numChannelsToMeter)
{
    const auto rate = newSampleRate > 0.0 ? newSampleRate : 48000.0;

    loudness.prepare (rate, numChannelsToMeter);
    truePeak.prepare (rate, numChannelsToMeter);
    wasPlaying = false;
    largestBlock = 0;
    audioStopped = false;

    // The reader picks the new rate up on its own thread
    sampleRate = rate;
    numChannels = juce::jlimit (1, maxChannels, numChannelsToMeter);
    readerReset = true;
}

void MeterEngine::requestReset() noexcept
{
    pendingReset = true;
}

void MeterEngine::discardPending() noexcept
{
    const auto ready = fifo.getNumReady();
    fifo.finishedRead (ready);

    // The time with no reader wasn't the audio stopping
    idleFor = 0.0;
}

//==============================================================================
void MeterEngine::process (const juce::AudioBuffer<float>& buffer, const Transport& transport) noexcept
{
    auto reset = pendingReset.exchange (false);

    const auto playing = transport.available && transport.playing;

    if (playing && ! wasPlaying)
        reset = true;

    wasPlaying = playing;

    if (reset)
    {
        loudness.resetIntegrated();
        truePeak.resetHeld();
        readerReset = true;
    }

    const auto counting = ! transport.available || transport.playing;
    loudness.setIntegrating (counting);
    integrating = counting;

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    if (buffer.getNumSamples() > largestBlock.load())
        largestBlock = buffer.getNumSamples();

    // What was heard before a gap isn't part of the last 400 ms or 3 s any more
    if (audioStopped.exchange (false))
        loudness.forgetRecent();

    loudness.process (buffer);
    truePeak.process (buffer);

    // For the reader's spectrum and stereo field. With no reader this fills up, and the rest is dropped
    const auto scope = fifo.write (juce::jmin (buffer.getNumSamples(), fifo.getFreeSpace()));

    for (int channel = 0; channel < maxChannels; ++channel)
    {
        // A mono signal is the same in both channels
        const auto* source = buffer.getReadPointer (juce::jmin (channel, buffer.getNumChannels() - 1));
        auto* destination = fifoData[static_cast<size_t> (channel)].data();

        std::copy_n (source, scope.blockSize1, destination + scope.startIndex1);
        std::copy_n (source + scope.blockSize1, scope.blockSize2, destination + scope.startIndex2);
    }
}

//==============================================================================
const MeterFrame::Data& MeterEngine::updateFrame (double seconds) noexcept
{
    using namespace MeterFrame;

    seconds = juce::jlimit (0.0, maxFrameSeconds, seconds);

    if (const auto rate = sampleRate.load(); ! juce::exactlyEqual (rate, readerRate))
    {
        readerRate = rate;
        spectrum.setSampleRate (rate);
        stereo.setSampleRate (rate);
        beat.setSampleRate (rate);
    }

    auto numSamples = 0;

    {
        const auto scope = fifo.read (fifo.getNumReady());
        numSamples = scope.blockSize1 + scope.blockSize2;

        for (size_t channel = 0; channel < maxChannels; ++channel)
        {
            std::copy_n (fifoData[channel].data() + scope.startIndex1, scope.blockSize1, incoming[channel].data());
            std::copy_n (fifoData[channel].data() + scope.startIndex2, scope.blockSize2, incoming[channel].data() + scope.blockSize1);
        }
    }

    idleFor = numSamples > 0 ? 0.0 : idleFor + seconds;

    const auto wasIdle = std::exchange (idle, idleFor > juce::jmax (idleSeconds, 2.0 * largestBlock.load() / readerRate));

    if (idle)
    {
        // The host has stopped sending audio, so everything falls away as if through silence
        numSamples = juce::jmin (fifoSize, juce::roundToInt (seconds * readerRate));

        for (auto& channel : incoming)
            std::fill_n (channel.data(), numSamples, 0.0f);

        if (! wasIdle)
            audioStopped = true;
    }

    for (int i = 0; i < numSamples; ++i)
        mid[static_cast<size_t> (i)] = 0.5f * (incoming[0][static_cast<size_t> (i)] + incoming[1][static_cast<size_t> (i)]);

    spectrum.push (mid.data(), numSamples);
    stereo.push (incoming[0].data(), incoming[1].data(), numSamples);
    beat.push (mid.data(), numSamples, seconds);
    spectrum.update (seconds);

    // A reset asked for while no audio is arriving waits for process(), so it's shown here meanwhile
    const auto resetWaiting = idle && pendingReset.load();

    if (readerReset.exchange (false) || (resetWaiting && ! resetShown))
        spectrum.resetPeaks();

    resetShown = resetWaiting;

    // The live loudness as silence would leave it: a window's power falls in step with how much
    // of the window the silence has filled
    const auto fallen = [this] (float heard, double windowSeconds)
    {
        if (! idle || heard <= noLoudness)
            return heard;

        const auto left = 1.0 - idleFor / windowSeconds;
        return left > 0.0 ? juce::jmax (noLoudness, heard + static_cast<float> (10.0 * std::log10 (left))) : noLoudness;
    };

    for (size_t channel = 0; channel < maxChannels; ++channel)
    {
        const auto heard = juce::Decibels::gainToDecibels (truePeak.readAndReset (static_cast<int> (channel)), peakFloorDb);
        peakDb[channel] = juce::jmax (heard, peakDb[channel] - peakFallDbPerSecond * static_cast<float> (seconds), peakFloorDb);
    }

    // A mono signal is the same in both channels
    if (numChannels.load() == 1)
        peakDb[1] = peakDb[0];

    frame[momentary] = fallen (loudness.getMomentary(), LoudnessMeter::momentarySeconds);
    frame[shortTerm] = fallen (loudness.getShortTerm(), LoudnessMeter::shortTermSeconds);
    frame[integrated] = resetWaiting ? noLoudness : loudness.getIntegrated();
    frame[range] = resetWaiting ? noLoudness : loudness.getRange();
    frame[truePeakLeft] = peakDb[0];
    frame[truePeakRight] = peakDb[1];
    frame[maxTruePeak] = resetWaiting ? noLoudness : juce::Decibels::gainToDecibels (truePeak.getMax(), noLoudness);
    frame[clipCount] = resetWaiting ? 0.0f : static_cast<float> (truePeak.getClipCount());
    frame[correlation] = stereo.getCorrelation();
    frame[balance] = stereo.getBalance();
    frame[pulse] = beat.get();
    frame[MeterFrame::integrating] = integrating.load() ? 1.0f : 0.0f;

    std::copy (spectrum.getBands().begin(), spectrum.getBands().end(), frame.begin() + bandsOffset);
    std::copy (spectrum.getPeaks().begin(), spectrum.getPeaks().end(), frame.begin() + peaksOffset);
    stereo.copyPoints (frame.data() + pointsOffset);

    return frame;
}
