#pragma once

#include "DashboardLayout.h"
#include "LoudnessView.h"
#include "SpectrumView.h"
#include "StereoFieldView.h"
#include "WaveformView.h"

//==============================================================================
/**
 * The meter's panels. One can grow to fill the dashboard while the others fade out.
 * Mirrored by webui/src/meter/Dashboard.tsx.
 */
class Dashboard : public juce::Component
{
public:
    // The same numbers as ParameterSpecs::viewNames: the dashboard, then each panel expanded
    static constexpr int dashboardView = 0;
    static_assert (ParameterSpecs::viewNames.size() == DashboardLayout::numPanels + 1);

    explicit Dashboard (const MeterLook& lookToUse);

    /** Called with the view a click asks for. */
    std::function<void (int view)> onViewChosen;

    /** Animates to a view, or jumps there. */
    void setView (int newView, bool animate);

    /** Which panels the dashboard has. An expanded panel shows whether it's one of them or not. */
    void setShown (const DashboardLayout::Shown& newShown);

    /** How the panels share the space. */
    void setSizes (const DashboardLayout::Sizes& newSizes);

    /** Called with the sizes a drag on the gap between two panels asks for, or a double-click there, which puts them back. */
    std::function<void (const DashboardLayout::Sizes&)> onSizesChosen;

    void setFrame (const MeterFrame::Data& frame, double seconds);

    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    /** The gaps that can be dragged: none while a panel is expanded. */
    std::vector<DashboardLayout::Divider> getDividers() const;

    MeterPanel& getPanel (DashboardLayout::Panel panel)   { return *panels[panel]; }

private:
    const MeterLook& look;

    SpectrumView spectrum { look };
    StereoFieldView stereo { look };
    LoudnessView loudness { look };
    WaveformView waveform { look };
    const std::array<MeterPanel*, DashboardLayout::numPanels> panels { &spectrum, &stereo, &loudness, &waveform };

    DashboardLayout::Shown shown = DashboardLayout::allShown;
    DashboardLayout::Sizes sizes;

    // The gap being dragged, and the sizes as the drag began
    std::optional<DashboardLayout::Divider> draggedDivider;
    DashboardLayout::Sizes sizesBeforeDrag;

    std::optional<DashboardLayout::Divider> dividerAt (juce::Point<int> position) const;

    int view = dashboardView;
    DashboardLayout::Panel expanded = DashboardLayout::spectrum;   // the panel growing, shrinking or full
    float expansion = 0.0f;                                        // from 0, the dashboard, to 1

    void layOut();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dashboard)
};
