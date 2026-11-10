#pragma once

#include "MeterPanel.h"

//==============================================================================
/**
 * Loudness and true peak: bars for the momentary and short-term loudness and each channel's
 * true peak, beside the readings as numbers.
 */
class LoudnessView : public MeterPanel
{
public:
    enum Cell { momentary, shortTerm, range, truePeak, clips, numCells };

    struct Layout
    {
        std::array<juce::Rectangle<float>, 4> bars;   // momentary, short-term, left peak, right peak
        juce::Rectangle<float> scale;                 // the bars' numbers, to their left
        juce::Rectangle<float> integrated;
        std::array<juce::Rectangle<float>, numCells> cells;
    };

    static constexpr float barLabelHeight = 14.0f, scaleWidth = 22.0f;

    explicit LoudnessView (const MeterLook& lookToUse) : MeterPanel ("LOUDNESS", lookToUse) {}

    static Layout layOut (juce::Rectangle<float> area);

private:
    void paintContent (juce::Graphics&, juce::Rectangle<float> area) override;
    void paintBar (juce::Graphics&, juce::Rectangle<float> bounds, const char* name, float value, bool hotOverZero) const;
    void paintReading (juce::Graphics&, juce::Rectangle<float> bounds, const juce::String& name, const juce::String& value,
                       juce::Colour colour, float largest) const;
};
