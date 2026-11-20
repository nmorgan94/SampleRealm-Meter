#include "WaveformView.h"

juce::Path WaveformView::shape (juce::Rectangle<float> plot, int band) const
{
    using namespace MeterFrame;

    const auto phase = frame[waveformPhase];
    const auto half = plot.getHeight() * 0.5f;

    const auto reach = [&] (int column)
    {
        return half * juce::jmin (1.0f, frame[static_cast<size_t> (waveformOffset + waveformBands * column + band)]);
    };

    // The first and last columns reach out to the plot's edges, which the phase leaves them short of
    juce::Path path;
    path.startNewSubPath (plot.getX(), plot.getCentreY() - reach (0));

    for (int column = 0; column < numColumns; ++column)
        path.lineTo (columnX (plot, column, phase), plot.getCentreY() - reach (column));

    path.lineTo (plot.getRight(), plot.getCentreY() - reach (numColumns - 1));
    path.lineTo (plot.getRight(), plot.getCentreY() + reach (numColumns - 1));

    for (int column = numColumns; --column >= 0;)
        path.lineTo (columnX (plot, column, phase), plot.getCentreY() + reach (column));

    path.lineTo (plot.getX(), plot.getCentreY() + reach (0));
    path.closeSubPath();
    return path;
}

void WaveformView::paintContent (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto& theme = look.theme;
    const auto amount = glowAmount();

    // The middle line brightens with the beat
    g.setColour (theme.line.withMultipliedAlpha (0.55f + 0.45f * amount));
    g.fillRect (juce::Rectangle<float> (area.getX(), area.getCentreY() - 0.5f, area.getWidth(), 1.0f));

    const juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (area.getSmallestIntegerContainer());

    // The lows at the back, where they're the tallest, and the highs in front
    g.setColour (theme.primary.withAlpha (0.7f + 0.3f * amount));
    g.fillPath (shape (area, 0));
    g.setColour (theme.secondary.withAlpha (0.85f));
    g.fillPath (shape (area, 1));
    g.setColour (theme.text.withAlpha (0.9f));
    g.fillPath (shape (area, 2));
}
