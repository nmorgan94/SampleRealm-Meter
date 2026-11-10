#include "MeterPanel.h"
#include "../CustomLookAndFeel.h"

MeterPanel::MeterPanel (const juce::String& title, const MeterLook& lookToUse) : look (lookToUse)
{
    setTitle (title);
    setOpaque (false);
}

juce::Font MeterPanel::font (float height, bool bold)
{
    return CustomLookAndFeel::orbitron (bold ? CustomLookAndFeel::boldWeight : CustomLookAndFeel::regularWeight).withHeight (height);
}

void MeterPanel::setExpanded (bool shouldBeExpanded)
{
    if (std::exchange (expanded, shouldBeExpanded) != shouldBeExpanded)
        repaint();
}

void MeterPanel::setFrame (const MeterFrame::Data& newFrame, double seconds)
{
    // A stopped or silent meter soon shows the same frame every time
    if (std::exchange (frame, newFrame) == newFrame && ! isMoving())
        return;

    advance (seconds);
    repaint();
}

juce::Rectangle<int> MeterPanel::getContentBounds() const
{
    return getLocalBounds().reduced (inset).withTrimmedTop (titleHeight - inset / 2);
}

juce::Rectangle<int> MeterPanel::getExpandButtonBounds() const
{
    return getLocalBounds().reduced (expandButtonInset).removeFromTop (expandButtonSize).removeFromRight (expandButtonSize);
}

//==============================================================================
void MeterPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour (look.theme.panel);
    g.fillRoundedRectangle (bounds, cornerSize);
    g.setColour (look.theme.line);
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);

    if (! look.capture)
    {
        g.setColour (look.theme.dim);
        g.setFont (font (11.0f, true));
        g.drawText (getTitle(), getLocalBounds().reduced (inset, 0).withHeight (titleHeight + 4), juce::Justification::centredLeft);

        // Two corners, pointing out to expand and in to go back
        const auto button = getExpandButtonBounds().toFloat();
        const auto centre = button.getCentre();
        juce::Path corners;

        if (expanded)
        {
            corners.startNewSubPath (centre.x + 2.0f, centre.y - 7.0f);
            corners.lineTo (centre.x + 2.0f, centre.y - 2.0f);
            corners.lineTo (centre.x + 7.0f, centre.y - 2.0f);
            corners.startNewSubPath (centre.x - 7.0f, centre.y + 2.0f);
            corners.lineTo (centre.x - 2.0f, centre.y + 2.0f);
            corners.lineTo (centre.x - 2.0f, centre.y + 7.0f);
        }
        else
        {
            corners.startNewSubPath (centre.x + 2.0f, centre.y - 7.0f);
            corners.lineTo (centre.x + 7.0f, centre.y - 7.0f);
            corners.lineTo (centre.x + 7.0f, centre.y - 2.0f);
            corners.startNewSubPath (centre.x - 7.0f, centre.y + 2.0f);
            corners.lineTo (centre.x - 7.0f, centre.y + 7.0f);
            corners.lineTo (centre.x - 2.0f, centre.y + 7.0f);
        }

        g.strokePath (corners, juce::PathStrokeType (1.5f));
    }

    const auto content = getContentBounds();

    // Too small to draw in, as while another panel takes over the dashboard
    if (content.getWidth() < 24 || content.getHeight() < 24)
        return;

    paintContent (g, content.toFloat());
}

void MeterPanel::mouseUp (const juce::MouseEvent& event)
{
    if (! look.capture && event.mouseWasClicked() && getExpandButtonBounds().contains (event.getPosition()) && onToggleExpand != nullptr)
        onToggleExpand();
}

void MeterPanel::mouseDoubleClick (const juce::MouseEvent& event)
{
    if (! getExpandButtonBounds().contains (event.getPosition()) && onToggleExpand != nullptr)
        onToggleExpand();
}
