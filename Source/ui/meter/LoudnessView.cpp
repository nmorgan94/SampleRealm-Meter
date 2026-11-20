#include "LoudnessView.h"
#include "MeterText.h"
#include "../../shared/MeterScales.h"

LoudnessView::Layout LoudnessView::layOut (juce::Rectangle<float> area)
{
    constexpr auto smallGap = 5.0f, groupGap = 14.0f;

    Layout layout;
    layout.compact = area.getWidth() < compactBelowWidth || area.getHeight() < compactBelowHeight;

    const auto narrowest = layout.compact ? 96.0f : 130.0f;
    const auto widest = layout.compact ? 130.0f : 210.0f;

    auto barArea = area.removeFromLeft (std::floor (juce::jlimit (narrowest, widest, area.getWidth() * 0.36f)));
    area.removeFromLeft (layout.compact ? 12.0f : 16.0f);

    layout.scale = barArea.removeFromLeft (scaleWidth).withTrimmedBottom (barLabelHeight);
    barArea.removeFromLeft (smallGap);

    const auto barWidth = std::floor ((barArea.getWidth() - 2.0f * smallGap - groupGap) / 4.0f);
    auto x = barArea.getX();

    for (size_t i = 0; i < layout.bars.size(); ++i)
    {
        layout.bars[i] = { x, barArea.getY(), barWidth, barArea.getHeight() };
        x += barWidth + (i == 1 ? groupGap : smallGap);
    }

    if (layout.compact)
    {
        // Side by side where each has the width for its number, and otherwise one over the next
        const auto across = area.getWidth() >= compactAcrossWidth;
        const auto size = std::floor ((across ? area.getWidth() : area.getHeight()) / 3.0f);

        for (auto* reading : { &layout.integrated, &layout.cells[shortTerm], &layout.cells[truePeak] })
            *reading = across ? area.removeFromLeft (size) : area.removeFromTop (size);

        return layout;
    }

    layout.integrated = area.removeFromTop (std::floor (area.getHeight() * 0.46f));

    const auto cellWidth = std::floor (area.getWidth() / 3.0f);
    const auto cellHeight = std::floor (area.getHeight() / 2.0f);

    for (size_t i = 0; i < numCells; ++i)
        layout.cells[i] = { area.getX() + cellWidth * static_cast<float> (i % 3), area.getY() + cellHeight * static_cast<float> (i / 3),
                            cellWidth, cellHeight };

    return layout;
}

//==============================================================================
void LoudnessView::paintContent (juce::Graphics& g, juce::Rectangle<float> area)
{
    using namespace MeterScales;

    const auto& theme = look.theme;
    const auto layout = layOut (area);

    const auto yOf = [&layout] (float level)
    {
        const auto track = layout.bars[0].withTrimmedBottom (barLabelHeight);
        return track.getBottom() - track.getHeight() * proportion (level, levelBottom, levelTop);
    };

    if (! look.capture)
    {
        g.setColour (theme.dim);
        g.setFont (font (9.0f));

        for (const auto tick : levelTicks)
            g.drawText (juce::String (tick), juce::Rectangle<float> (layout.scale.getX(), yOf (static_cast<float> (tick)) - 6.0f, layout.scale.getWidth(), 12.0f),
                        juce::Justification::centredRight, false);
    }

    paintBar (g, layout.bars[0], layout.compact ? "M" : "MOM", frame[MeterFrame::momentary], false);
    paintBar (g, layout.bars[1], layout.compact ? "S" : "ST", frame[MeterFrame::shortTerm], false);
    paintBar (g, layout.bars[2], "L", frame[MeterFrame::truePeakLeft], true);
    paintBar (g, layout.bars[3], "R", frame[MeterFrame::truePeakRight], true);

    // The integrated loudness as a line across the loudness bars
    if (const auto integrated = frame[MeterFrame::integrated]; MeterText::hasReading (integrated))
    {
        g.setColour (theme.text);
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (layout.bars[0].getX() - 2.0f, yOf (integrated) - 1.0f,
                                                                layout.bars[1].getRight() + 2.0f, yOf (integrated) + 1.0f));
    }

    const auto paused = frame[MeterFrame::integrating] < 0.5f;
    const auto over = frame[MeterFrame::maxTruePeak] > 0.0f;
    const auto clipped = frame[MeterFrame::clipCount] > 0.0f;

    if (layout.compact)
    {
        paintReading (g, layout.integrated, paused ? "INT - PAUSED" : "INT LUFS", MeterText::loudness (frame[MeterFrame::integrated]), theme.primary, 40.0f);
        paintReading (g, layout.cells[shortTerm], "SHORT", MeterText::loudness (frame[MeterFrame::shortTerm]), theme.text, 40.0f);
        paintReading (g, layout.cells[truePeak], "TP MAX", MeterText::truePeak (frame[MeterFrame::maxTruePeak]), over ? theme.hot : theme.text, 40.0f);
        return;
    }

    paintReading (g, layout.integrated, paused ? "INTEGRATED LUFS - PAUSED" : "INTEGRATED LUFS",
                  MeterText::loudness (frame[MeterFrame::integrated]), theme.primary, 96.0f);

    paintReading (g, layout.cells[momentary], "MOMENTARY", MeterText::loudness (frame[MeterFrame::momentary]), theme.text, 40.0f);
    paintReading (g, layout.cells[shortTerm], "SHORT-TERM", MeterText::loudness (frame[MeterFrame::shortTerm]), theme.text, 40.0f);
    paintReading (g, layout.cells[range], "RANGE LU", MeterText::loudness (frame[MeterFrame::range]), theme.text, 40.0f);
    paintReading (g, layout.cells[truePeak], "TRUE PEAK MAX", MeterText::truePeak (frame[MeterFrame::maxTruePeak]), over ? theme.hot : theme.text, 40.0f);
    paintReading (g, layout.cells[clips], "CLIPS", MeterText::clips (frame[MeterFrame::clipCount]), clipped ? theme.hot : theme.text, 40.0f);
}

void LoudnessView::paintBar (juce::Graphics& g, juce::Rectangle<float> bounds, const char* name, float value, bool hotOverZero) const
{
    using namespace MeterScales;

    const auto& theme = look.theme;
    const auto label = bounds.removeFromBottom (barLabelHeight);
    const auto track = bounds;

    if (! look.capture)
    {
        g.setColour (theme.dim);
        g.setFont (font (9.0f));
        g.drawText (name, label, juce::Justification::centredBottom, false);
    }

    g.setColour (theme.line.withMultipliedAlpha (0.45f));
    g.fillRoundedRectangle (track, 2.0f);

    const auto top = track.getBottom() - track.getHeight() * proportion (value, levelBottom, levelTop);

    if (top < track.getBottom())
    {
        // Brighter the higher it goes, and stronger with the beat
        g.setGradientFill (juce::ColourGradient::vertical (theme.secondary, track.getY(), theme.primary, track.getBottom()));
        g.setOpacity (0.8f + 0.2f * glowAmount());
        g.fillRect (track.withTop (top));
        g.setOpacity (1.0f);
    }

    // Over full scale
    const auto zero = track.getBottom() - track.getHeight() * proportion (0.0f, levelBottom, levelTop);

    if (hotOverZero && top < zero)
    {
        g.setColour (theme.hot);
        g.fillRect (track.withTop (top).withBottom (zero));
    }
}

void LoudnessView::paintReading (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& name, const juce::String& value,
                                 juce::Colour colour, float largest) const
{
    const auto label = bounds.removeFromTop (14.0f);

    g.setColour (look.theme.dim);
    g.setFont (font (9.0f));
    g.drawText (name, label, juce::Justification::centredLeft, false);

    // As big as fits, for five characters or so
    const auto height = juce::jlimit (12.0f, largest, juce::jmin (bounds.getHeight() - 4.0f, bounds.getWidth() / 3.6f));

    g.setColour (colour);
    g.setFont (font (height, true));
    g.drawText (value, bounds.withHeight (juce::jmin (bounds.getHeight(), height + 4.0f)), juce::Justification::centredLeft, false);
}
