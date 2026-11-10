#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

//==============================================================================
/**
 * A level from 0 to 1 that jumps up on each low hit, such as a kick, and dies away, for
 * the editor to move with the beat. Not thread safe: one thread uses it.
 */
class BeatPulse
{
public:
    void setSampleRate (double sampleRate) noexcept
    {
        const auto coefficient = [sampleRate] (double seconds)
        {
            return static_cast<float> (1.0 - std::exp (-1.0 / (seconds * sampleRate)));
        };

        lowPass = static_cast<float> (1.0 - std::exp (-2.0 * std::numbers::pi * cutoffHz / sampleRate));
        // The fast level catches a kick's start in 2 ms and lets go in 60, about a kick's body.
        // The slow one is where the low end has been sitting for the last 400 ms
        fastAttack = coefficient (0.002);
        fastRelease = coefficient (0.06);
        slowFollow = coefficient (0.4);

        // The loudest the low end has been lately, forgotten over 3 seconds, or a bar or two
        loudestRelease = static_cast<float> (std::exp (-1.0 / (3.0 * sampleRate)));
        reset();
    }

    void reset() noexcept
    {
        low = fast = slow = loudest = pulse = 0.0f;
    }

    /** Takes the next samples of a mono signal, and how long it's been since the last call. */
    void push (const float* samples, int numSamples, double seconds) noexcept
    {
        pulse *= static_cast<float> (std::exp (-seconds / decaySeconds));

        for (int i = 0; i < numSamples; ++i)
        {
            const auto sample = std::isfinite (samples[i]) ? samples[i] : 0.0f;

            low += (sample - low) * lowPass;
            const auto level = std::abs (low);

            fast += (level - fast) * (level > fast ? fastAttack : fastRelease);
            slow += (fast - slow) * slowFollow;
            loudest = std::max (fast, loudest * loudestRelease);

            // How far the low end has jumped over where it's been sitting, against its recent loudest
            const auto jump = (fast - threshold * slow) / (0.5f * std::max (loudest, quietest));
            pulse = std::max (pulse, std::clamp (jump, 0.0f, 1.0f));
        }

        // Out of denormals
        if (low > -tiny && low < tiny)   low = 0.0f;
        if (fast < tiny)                 fast = 0.0f;
        if (slow < tiny)                 slow = 0.0f;
        if (loudest < tiny)              loudest = 0.0f;
        if (pulse < 1.0e-4f)             pulse = 0.0f;
    }

    float get() const noexcept   { return pulse; }

private:
    // Kicks and bass notes are under this, and most else is over it
    static constexpr double cutoffHz = 120.0;

    // The pulse is over before the next hit at any usual tempo: a 16th note at 85 BPM
    static constexpr double decaySeconds = 0.18;

    // A hit is the low end jumping 20% over where it's been sitting, so a held note isn't one
    static constexpr float threshold = 1.2f;

    // -50 dB: under this there's no beat to follow
    static constexpr float quietest = 0.003f;
    static constexpr float tiny = 1.0e-20f;

    float lowPass = 0.0f, fastAttack = 0.0f, fastRelease = 0.0f, slowFollow = 0.0f, loudestRelease = 0.0f;
    float low = 0.0f, fast = 0.0f, slow = 0.0f, loudest = 0.0f, pulse = 0.0f;
};
