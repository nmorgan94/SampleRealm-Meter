#pragma once

#include "MeterLook.h"
#include "../../shared/MeterFrame.h"

//==============================================================================
/**
 * One of the dashboard's panels: a rounded box with a title and a button that expands it to
 * fill the editor. Each view draws its part of the frame inside. Mirrored by webui/src/meter/MeterPanel.tsx.
 */
class MeterPanel : public juce::Component
{
public:
    static constexpr int inset = 12, titleHeight = 22;
    static constexpr float cornerSize = 8.0f;
    static constexpr int expandButtonSize = 22, expandButtonInset = 5;

    MeterPanel (const juce::String& title, const MeterLook& lookToUse);

    /** Clicking the expand button, or double-clicking the panel. */
    std::function<void()> onToggleExpand;

    void setExpanded (bool shouldBeExpanded);

    /** Whether there's anything to expand over: not for the dashboard's only panel. */
    void setExpandable (bool shouldBeExpandable);

    /** The meter now, and how long it's been since the last frame. */
    void setFrame (const MeterFrame::Data& newFrame, double seconds);

    void paint (juce::Graphics&) final;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    /** Inside the box, under the title. */
    juce::Rectangle<int> getContentBounds() const;
    juce::Rectangle<int> getExpandButtonBounds() const;

protected:
    const MeterLook& look;
    MeterFrame::Data frame = MeterFrame::silent();

    /** Called with each new frame, before the repaint. */
    virtual void advance (double seconds)   { juce::ignoreUnused (seconds); }

    /** Whether the panel still has something to draw though the frame hasn't changed. */
    virtual bool isMoving() const           { return false; }
    virtual void paintContent (juce::Graphics&, juce::Rectangle<float> area) = 0;

    /** How much to glow right now, from 0 to 1: the beat's pulse, as far as the reactivity lets it. */
    float glowAmount() const noexcept   { return look.reactivity * frame[MeterFrame::pulse]; }

    static juce::Font font (float height, bool bold = false);

private:
    bool expanded = false, expandable = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterPanel)
};
