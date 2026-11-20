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

    explicit SpectrumView (const MeterLook& lookToUse) : MeterPanel ("SPECTRUM", lookToUse) {}

    /** Where the curve is drawn within the content. */
    static juce::Rectangle<float> plotIn (juce::Rectangle<float> area)   { return area.withTrimmedBottom (labelHeight); }

private:
    void paintContent (juce::Graphics&, juce::Rectangle<float> area) override;

    /** A smooth line through one level per band, starting at a frame offset. */
    juce::Path curve (juce::Rectangle<float> plot, int offset) const;
};
