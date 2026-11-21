#include "SpectrumView.h"
#include "Glow.h"
#include "MeterText.h"
#include "../../shared/MeterScales.h"

juce::Path SpectrumView::curve (juce::Rectangle<float> plot, int offset) const
{
    using namespace MeterScales;

    const auto pointAt = [&] (int band)
    {
        const auto level = frame[static_cast<size_t> (offset + band)];
        return juce::Point<float> (plot.getX() + plot.getWidth() * (static_cast<float> (band) + 0.5f) / MeterFrame::numBands,
                                   plot.getY() + plot.getHeight() * (1.0f - proportion (level, spectrumBottomDb, spectrumTopDb)));
    };

    // Through the middle of each pair of points, bending at the points themselves
    juce::Path path;
    auto previous = pointAt (0);
    path.startNewSubPath (plot.getX(), previous.y);
    path.lineTo (previous);

    for (int band = 1; band < MeterFrame::numBands; ++band)
    {
        const auto next = pointAt (band);
        path.quadraticTo (previous, (previous + next) * 0.5f);
        previous = next;
    }

    path.lineTo (previous);
    path.lineTo (plot.getRight(), previous.y);
    return path;
}

std::optional<SpectrumView::Readout> SpectrumView::readoutAt (juce::Rectangle<float> plot, juce::Point<float> point)
{
    if (plot.getWidth() < readoutWidth || ! plot.contains (point))
        return {};

    const auto x = point.x;
    const auto across = MeterScales::proportion (x, plot.getX(), plot.getRight());
    const auto fitsRight = x + readoutGap + readoutWidth <= plot.getRight();
    const auto labelX = juce::jlimit (plot.getX(), plot.getRight() - readoutWidth, fitsRight ? x + readoutGap : x - readoutGap - readoutWidth);

    return Readout { MeterScales::hzAcross (across),
                     juce::jmin (MeterFrame::numBands - 1, static_cast<int> (across * MeterFrame::numBands)),
                     { std::floor (x), plot.getY(), 1.0f, plot.getHeight() },
                     { labelX, plot.getY() + 2.0f, readoutWidth, readoutHeight },
                     fitsRight };
}

std::optional<SpectrumView::Readout> SpectrumView::getReadout() const
{
    if (look.capture || ! hover.has_value())
        return {};

    return readoutAt (plotIn (getContentBounds().toFloat()), *hover);
}

void SpectrumView::setHover (std::optional<juce::Point<float>> to)
{
    const auto shown = getReadout().has_value();
    hover = to;

    if (shown || getReadout().has_value())
        repaint();
}

void SpectrumView::mouseMove (const juce::MouseEvent& event)   { setHover (event.position); }
void SpectrumView::mouseDrag (const juce::MouseEvent& event)   { setHover (event.position); }
void SpectrumView::mouseExit (const juce::MouseEvent&)         { setHover ({}); }

void SpectrumView::paintContent (juce::Graphics& g, juce::Rectangle<float> area)
{
    using namespace MeterScales;

    const auto& theme = look.theme;
    const auto amount = glowAmount();
    const auto plot = plotIn (area);
    const auto labels = area.withTop (plot.getBottom());

    // The grid brightens with the beat
    g.setColour (theme.line.withMultipliedAlpha (0.55f + 0.45f * amount));

    for (const auto hz : spectrumGridHz)
        g.fillRect (juce::Rectangle<float> (plot.getX() + plot.getWidth() * acrossSpectrum (hz) - 0.5f, plot.getY(), 1.0f, plot.getHeight()));

    const auto yOf = [&plot] (float db)
    {
        return plot.getY() + plot.getHeight() * (1.0f - proportion (db, spectrumBottomDb, spectrumTopDb));
    };

    for (const auto db : spectrumGridDb)
        g.fillRect (juce::Rectangle<float> (plot.getX(), yOf (static_cast<float> (db)) - 0.5f, plot.getWidth(), 1.0f));

    if (! look.capture)
    {
        g.setColour (theme.dim);
        g.setFont (font (10.0f));

        // In a narrow plot, only the labels that don't run into the one before
        auto lastLabelX = plot.getX() - labelSpacing;

        for (size_t i = 0; i < spectrumGridHz.size(); ++i)
        {
            const auto x = plot.getX() + plot.getWidth() * acrossSpectrum (spectrumGridHz[i]);

            if (x - lastLabelX < labelSpacing)
                continue;

            lastLabelX = x;
            g.drawText (spectrumGridLabels[i], juce::Rectangle<float> (x - 20.0f, labels.getY(), 40.0f, labels.getHeight()),
                        juce::Justification::centred, false);
        }

        for (const auto db : spectrumGridDb)
            g.drawText (juce::String (db), juce::Rectangle<float> (plot.getX() + 4.0f, yOf (static_cast<float> (db)) + 1.0f, 40.0f, 12.0f),
                        juce::Justification::centredLeft, false);
    }

    const auto trace = curve (plot, MeterFrame::bandsOffset);

    auto fill = trace;
    fill.lineTo (plot.getBottomRight());
    fill.lineTo (plot.getBottomLeft());
    fill.closeSubPath();

    const juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (plot.getSmallestIntegerContainer());

    g.setGradientFill (juce::ColourGradient::vertical (theme.primary.withAlpha (0.42f + 0.2f * amount), plot.getY(),
                                                       theme.primary.withAlpha (0.02f), plot.getBottom()));
    g.fillPath (fill);

    g.setColour (theme.secondary.withAlpha (0.6f));
    g.strokePath (curve (plot, MeterFrame::peaksOffset), juce::PathStrokeType (1.0f));

    Glow::stroke (g, trace, juce::ColourGradient::horizontal (theme.primary, plot.getX(), theme.secondary, plot.getRight()), 1.8f, amount);

    if (const auto readout = getReadout())
    {
        g.setColour (theme.dim);
        g.fillRect (readout->line);

        g.setColour (theme.text);
        g.setFont (font (10.0f));
        g.drawText (MeterText::frequency (readout->hz) + "  " + MeterText::level (frame[static_cast<size_t> (MeterFrame::bandsOffset + readout->band)]),
                    readout->label, readout->labelOnRight ? juce::Justification::centredLeft : juce::Justification::centredRight, false);
    }
}
