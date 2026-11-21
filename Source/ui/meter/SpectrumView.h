#pragma once

#include "MeterPanel.h"

//==============================================================================
/** The spectrum as a glowing curve over its fill, with the held peaks as a thin line above. */
class SpectrumView : public MeterPanel
{
public:
    static constexpr float labelHeight = 16.0f;

    // The widest frequency label, "10k", and a gap
    static constexpr float labelSpacing = 30.0f;

    // "12.5 kHz  -100.0 dB" at the labels' size, a line of it, and its distance from the line
    static constexpr float readoutWidth = 150.0f, readoutHeight = 14.0f, readoutGap = 6.0f;

    /** What the mouse is over: its frequency, the band there, a line down the plot and where the reading is written. */
    struct Readout
    {
        float hz = 0.0f;
        int band = 0;
        juce::Rectangle<float> line, label;
        bool labelOnRight = true;
    };

    explicit SpectrumView (const MeterLook& lookToUse) : MeterPanel ("SPECTRUM", lookToUse) {}

    /** Where the curve is drawn within the content. */
    static juce::Rectangle<float> plotIn (juce::Rectangle<float> area)   { return area.withTrimmedBottom (labelHeight); }

    /** The readout for a point: none outside the plot, or where the plot is narrower than the label. The label is
        right of the line, or left of it near the plot's right edge, and kept inside the plot. */
    static std::optional<Readout> readoutAt (juce::Rectangle<float> plot, juce::Point<float> point);

    /** The readout showing: none unless the mouse is over the plot, and none in capture mode. */
    std::optional<Readout> getReadout() const;

    void mouseMove (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    std::optional<juce::Point<float>> hover;
    void setHover (std::optional<juce::Point<float>> to);

    void paintContent (juce::Graphics&, juce::Rectangle<float> area) override;

    /** A smooth line through one level per band, starting at a frame offset. */
    juce::Path curve (juce::Rectangle<float> plot, int offset) const;
};
