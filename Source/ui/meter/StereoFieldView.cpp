#include "StereoFieldView.h"
#include "Glow.h"
#include "../../shared/MeterScales.h"

StereoFieldView::Layout StereoFieldView::layOut (juce::Rectangle<float> area)
{
    Layout layout;

    if (area.getWidth() > area.getHeight() * wideRatio)
    {
        const auto side = std::floor (juce::jmax (0.0f, area.getHeight()));
        layout.scope = { area.getX(), area.getY(), side, side };

        auto bars = area.withTrimmedLeft (side + sideGap);
        bars = bars.withSizeKeepingCentre (bars.getWidth(), 2.0f * barHeight + barsGap);
        layout.correlation = bars.removeFromTop (barHeight);
        layout.balance = bars.removeFromBottom (barHeight);
        return layout;
    }

    layout.balance = area.removeFromBottom (barHeight);
    layout.correlation = area.removeFromBottom (barHeight);
    area.removeFromBottom (barsGap);

    const auto side = std::floor (juce::jmax (0.0f, juce::jmin (area.getWidth(), area.getHeight())));
    layout.scope = { std::floor (area.getCentreX() - side * 0.5f), std::floor (area.getCentreY() - side * 0.5f), side, side };
    return layout;
}

void StereoFieldView::advance (double seconds)
{
    const auto side = juce::roundToInt (layOut (getContentBounds().toFloat()).scope.getWidth());

    if (side < 24)
    {
        trace = {};
        traceLeft = 0.0f;
        return;
    }

    // Sharp on high-resolution screens while it's small, and cheap to fade while it's big
    const auto scale = side <= 400 ? 2 : 1;

    if (trace.getWidth() != side * scale)
    {
        // The trace comes along, so it doesn't go out while a panel grows or shrinks
        juce::Image resized (juce::Image::ARGB, side * scale, side * scale, true, juce::SoftwareImageType());

        if (traceLeft > 0.0f)
        {
            juce::Graphics g (resized);
            g.drawImage (trace, resized.getBounds().toFloat());
        }

        trace = resized;
    }

    // Nothing to fade once the trace has gone, as in silence
    if (traceLeft > 0.0f)
    {
        const auto fade = static_cast<float> (std::exp (-seconds / (MeterScales::traceFadeSeconds * (1.0 + look.reactivity))));
        traceLeft *= fade;

        // Under half a step of an 8-bit alpha, there's nothing left to see
        if (traceLeft < 1.0f / 512.0f)
        {
            trace.clear (trace.getBounds());
            traceLeft = 0.0f;
        }
        else
        {
            trace.multiplyAllAlphas (fade);
        }
    }

    const auto centre = static_cast<float> (side) * 0.5f;
    const auto radius = centre - scopeMargin;
    const auto* points = frame.data() + MeterFrame::pointsOffset;

    juce::Path path;
    auto furthest = 0.0f;

    for (int i = 0; i < MeterFrame::numPoints; ++i)
    {
        const auto x = points[2 * i], y = points[2 * i + 1];
        furthest = juce::jmax (furthest, std::abs (x), std::abs (y));

        if (i == 0)
            path.startNewSubPath (centre + x * radius, centre - y * radius);
        else
            path.lineTo (centre + x * radius, centre - y * radius);
    }

    // Silence leaves the trace to fade out
    if (furthest < 1.0e-4f)
        return;

    traceLeft = 1.0f;

    juce::Graphics g (trace);
    g.addTransform (juce::AffineTransform::scale (static_cast<float> (scale)));
    Glow::stroke (g, path, look.theme.primary, 1.2f, glowAmount());
}

void StereoFieldView::paintContent (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto& theme = look.theme;
    const auto layout = layOut (area);
    const auto& scope = layout.scope;

    if (scope.getWidth() >= 24.0f)
    {
        const auto centre = scope.getCentre();
        const auto radius = scope.getWidth() * 0.5f - scopeMargin;
        const auto diagonal = radius * 0.70710678f;

        g.setColour (theme.line);
        g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        g.setColour (theme.line.withMultipliedAlpha (0.7f));
        g.fillRect (juce::Rectangle<float> (centre.x - 0.5f, centre.y - radius, 1.0f, radius * 2.0f));
        g.fillRect (juce::Rectangle<float> (centre.x - radius, centre.y - 0.5f, radius * 2.0f, 1.0f));

        g.setColour (theme.line.withMultipliedAlpha (0.45f));
        g.drawLine (centre.x - diagonal, centre.y - diagonal, centre.x + diagonal, centre.y + diagonal, 1.0f);
        g.drawLine (centre.x - diagonal, centre.y + diagonal, centre.x + diagonal, centre.y - diagonal, 1.0f);

        if (! look.capture)
        {
            g.setColour (theme.dim);
            g.setFont (font (10.0f));

            const auto label = [&] (const char* text, float x, float y)
            {
                g.drawText (text, juce::Rectangle<float> (x - 8.0f, y - 6.0f, 16.0f, 12.0f), juce::Justification::centred, false);
            };

            label ("L", centre.x - diagonal - 9.0f, centre.y - diagonal - 7.0f);
            label ("R", centre.x + diagonal + 9.0f, centre.y - diagonal - 7.0f);
        }

        if (traceLeft > 0.0f)
            g.drawImage (trace, scope);
    }

    paintBar (g, layout.correlation, frame[MeterFrame::correlation], "-1", "CORRELATION", "+1", true);
    paintBar (g, layout.balance, frame[MeterFrame::balance], "L", "BALANCE", "R", false);
}

void StereoFieldView::paintBar (juce::Graphics& g, juce::Rectangle<float> bounds, float value, const juce::String& left,
                                const juce::String& name, const juce::String& right, bool hotBelowZero) const
{
    const auto& theme = look.theme;
    const auto labels = bounds.removeFromTop (14.0f);
    const auto track = bounds.removeFromTop (6.0f);

    if (! look.capture)
    {
        g.setColour (theme.dim);
        g.setFont (font (9.0f));
        g.drawText (left, labels, juce::Justification::centredLeft, false);
        g.drawText (name, labels, juce::Justification::centred, false);
        g.drawText (right, labels, juce::Justification::centredRight, false);
    }

    g.setColour (theme.line.withMultipliedAlpha (0.6f));
    g.fillRoundedRectangle (track, 3.0f);

    const auto centre = track.getCentreX();
    const auto x = centre + juce::jlimit (-1.0f, 1.0f, value) * track.getWidth() * 0.5f;

    g.setColour (hotBelowZero && value < 0.0f ? theme.hot : theme.primary);
    g.fillRect (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (centre, x), track.getY(), juce::jmax (centre, x), track.getBottom()));

    g.setColour (theme.text);
    g.fillRoundedRectangle (x - 1.5f, track.getY() - 2.0f, 3.0f, track.getHeight() + 4.0f, 1.5f);
}
