#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include "../shared/MeterFrame.h"

//==============================================================================
/**
 * How a stereo signal sits between its channels: the correlation, the balance, and the
 * goniometer's points. Not thread safe: one thread uses it.
 */
class StereoField
{
public:
    static constexpr int numPoints = MeterFrame::numPoints;

    void setSampleRate (double sampleRate) noexcept
    {
        // The correlation and balance settle over a quarter of a second: steady enough to read,
        // quick enough to follow a phrase
        smoothing = static_cast<float> (1.0 - std::exp (-1.0 / (0.25 * sampleRate)));

        // The scope's scale lets go over a second and a half, so it doesn't pump with every hit
        scaleRelease = static_cast<float> (std::exp (-1.0 / (1.5 * sampleRate)));

        // The points span about one editor frame
        stride = std::max (1, static_cast<int> (std::lround (sampleRate * 0.016 / numPoints)));
        reset();
    }

    void reset() noexcept
    {
        points.fill (0.0f);
        pointIndex = untilNextPoint = 0;
        leftRight = leftSquared = rightSquared = 0.0f;
        scale = 0.0f;
    }

    void push (const float* left, const float* right, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto l = std::isfinite (left[i]) ? left[i] : 0.0f;
            const auto r = std::isfinite (right[i]) ? right[i] : 0.0f;

            leftRight += (l * r - leftRight) * smoothing;
            leftSquared += (l * l - leftSquared) * smoothing;
            rightSquared += (r * r - rightSquared) * smoothing;

            // The signal's recent peak, which the points are drawn against
            scale = std::max (std::max (std::abs (l), std::abs (r)), scale * scaleRelease);

            if (--untilNextPoint > 0)
                continue;

            untilNextPoint = stride;

            // Quiet signals fill the scope too, but not the noise floor. Halved, so a mono signal
            // at its peak reaches the edge and nothing goes past it
            const auto gain = pointRadius / std::max (scale, quietest) * 0.5f;
            const auto index = static_cast<size_t> (2 * pointIndex);

            // Mid goes up and side across, so the left channel alone leans left
            points[index] = std::clamp ((r - l) * gain, -1.0f, 1.0f);
            points[index + 1] = std::clamp ((l + r) * gain, -1.0f, 1.0f);
            pointIndex = (pointIndex + 1) % numPoints;
        }

        // Out of denormals, which silence would otherwise leave these in for good
        for (auto* value : { &leftRight, &leftSquared, &rightSquared, &scale })
            if (std::abs (*value) < tiny)
                *value = 0.0f;
    }

    /** 1 where the channels are the same, 0 where they're unrelated, and -1 where one is the other inverted. */
    float getCorrelation() const noexcept
    {
        const auto power = std::sqrt (leftSquared * rightSquared);
        return power > silent ? std::clamp (leftRight / power, -1.0f, 1.0f) : 0.0f;
    }

    /** From -1, all in the left channel, to 1. */
    float getBalance() const noexcept
    {
        const auto l = std::sqrt (leftSquared);
        const auto r = std::sqrt (rightSquared);
        return l + r > 1.0e-5f ? (r - l) / (l + r) : 0.0f;
    }

    /** The points as x, y pairs within -1 to 1, the oldest first. */
    void copyPoints (float* destination) const noexcept
    {
        const auto first = static_cast<size_t> (2 * pointIndex);
        std::copy (points.begin() + static_cast<std::ptrdiff_t> (first), points.end(), destination);
        std::copy (points.begin(), points.begin() + static_cast<std::ptrdiff_t> (first), destination + (points.size() - first));
    }

private:
    // Just inside the scope's edge
    static constexpr float pointRadius = 0.98f;

    // -40 dB: quieter than this the points shrink, so the noise floor doesn't fill the scope
    static constexpr float quietest = 0.01f;

    // -60 dB of power in both channels: no correlation to speak of, and reached within a few
    // seconds of the signal stopping, so the bar goes back to the middle with everything else
    static constexpr float silent = 1.0e-6f;

    static constexpr float tiny = 1.0e-20f;

    float smoothing = 0.0f, scaleRelease = 0.0f;
    int stride = 1, untilNextPoint = 0;

    float leftRight = 0.0f, leftSquared = 0.0f, rightSquared = 0.0f;
    float scale = 0.0f;

    std::array<float, 2 * numPoints> points {};
    int pointIndex = 0;
};
