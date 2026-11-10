#pragma once

#include "DashboardLayout.h"
#include "LoudnessView.h"
#include "SpectrumView.h"
#include "StereoFieldView.h"

//==============================================================================
/**
 * The meter's three panels. One can grow to fill the dashboard while the others fade out.
 * Mirrored by webui/src/meter/Dashboard.tsx.
 */
class Dashboard : public juce::Component
{
public:
    // The same numbers as ParameterSpecs::viewNames: the dashboard, then each panel expanded
    static constexpr int dashboardView = 0;

    explicit Dashboard (const MeterLook& lookToUse);

    /** Called with the view a click asks for. */
    std::function<void (int view)> onViewChosen;

    /** Animates to a view, or jumps there. */
    void setView (int newView, bool animate);

    void setFrame (const MeterFrame::Data& frame, double seconds);

    void resized() override;

    MeterPanel& getPanel (DashboardLayout::Panel panel)   { return *panels[panel]; }

private:
    const MeterLook& look;

    SpectrumView spectrum { look };
    StereoFieldView stereo { look };
    LoudnessView loudness { look };
    const std::array<MeterPanel*, DashboardLayout::numPanels> panels { &spectrum, &stereo, &loudness };

    int view = dashboardView;
    DashboardLayout::Panel expanded = DashboardLayout::spectrum;   // the panel growing, shrinking or full
    float expansion = 0.0f;                                        // from 0, the dashboard, to 1

    void layOut();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dashboard)
};
