#pragma once

#include "MeterPanel.h"

//==============================================================================
/**
 * The last few seconds scrolling by as DJ software draws a track: the lows, the mids and the
 * highs as three waveforms, one over the other.
 */
class WaveformView : public MeterPanel
{
public:
    explicit WaveformView (const MeterLook& lookToUse) : MeterPanel ("WAVEFORM", lookToUse) {}

    /**
     * Where a column is across the plot, the newest at its right edge. The phase, how much of
     * the next column has been heard, moves them all left by up to a column.
     */
    static float columnX (juce::Rectangle<float> plot, int column, float phase)
    {
        return plot.getX() + plot.getWidth() * (static_cast<float> (column + 1) - phase) / MeterFrame::numColumns;
    }

private:
    void paintContent (juce::Graphics&, juce::Rectangle<float> area) override;

    /** One band's peaks, mirrored about the middle. */
    juce::Path shape (juce::Rectangle<float> plot, int band) const;
};
