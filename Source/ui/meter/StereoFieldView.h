#pragma once

#include "MeterPanel.h"

//==============================================================================
/**
 * The goniometer, whose trace fades like a scope's phosphor, over the correlation and
 * balance bars.
 */
class StereoFieldView : public MeterPanel
{
public:
    static constexpr float barHeight = 26.0f, barsGap = 6.0f, scopeMargin = 12.0f;

    // Wider than this for its height, the bars go beside the scope rather than under it
    static constexpr float wideRatio = 1.4f;
    static constexpr float sideGap = 14.0f;

    struct Layout
    {
        juce::Rectangle<float> scope, correlation, balance;
    };

    explicit StereoFieldView (const MeterLook& lookToUse) : MeterPanel ("STEREO FIELD", lookToUse) {}

    /** A square scope with the two bars under it, or beside it in a wide panel. */
    static Layout layOut (juce::Rectangle<float> area);

private:
    juce::Image trace;

    // How bright the trace's last stroke still is, from 1 down to 0 once it has gone
    float traceLeft = 0.0f;

    void advance (double seconds) override;
    bool isMoving() const override   { return traceLeft > 0.0f; }
    void paintContent (juce::Graphics&, juce::Rectangle<float> area) override;
    void paintBar (juce::Graphics&, juce::Rectangle<float> bounds, float value, const juce::String& left,
                   const juce::String& name, const juce::String& right, bool hotBelowZero) const;
};
