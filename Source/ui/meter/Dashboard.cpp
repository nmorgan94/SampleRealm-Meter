#include "Dashboard.h"
#include "../../shared/MeterScales.h"

Dashboard::Dashboard (const MeterLook& lookToUse) : look (lookToUse)
{
    for (size_t i = 0; i < panels.size(); ++i)
    {
        addAndMakeVisible (panels[i]);

        panels[i]->onToggleExpand = [this, panelView = static_cast<int> (i) + 1]
        {
            if (onViewChosen != nullptr)
                onViewChosen (view == panelView ? dashboardView : panelView);
        };
    }
}

void Dashboard::setView (int newView, bool animate)
{
    newView = juce::jlimit (0, static_cast<int> (panels.size()), newView);

    if (newView != dashboardView)
    {
        const auto panel = static_cast<DashboardLayout::Panel> (newView - 1);

        // Straight from one panel to another, there's nothing to grow from
        if (panel != expanded && expansion > 0.0f)
            animate = false;

        expanded = panel;
    }

    view = newView;

    if (! animate)
        expansion = view == dashboardView ? 0.0f : 1.0f;

    for (size_t i = 0; i < panels.size(); ++i)
        panels[i]->setExpanded (view == static_cast<int> (i) + 1);

    layOut();
}

void Dashboard::setShown (const DashboardLayout::Shown& newShown)
{
    if (std::exchange (shown, newShown) != newShown)
        layOut();
}

void Dashboard::setFrame (const MeterFrame::Data& frame, double seconds)
{
    const auto target = view == dashboardView ? 0.0f : 1.0f;

    if (! juce::exactlyEqual (expansion, target))
    {
        const auto step = static_cast<float> (seconds / MeterScales::expandSeconds);
        expansion = target > expansion ? juce::jmin (target, expansion + step) : juce::jmax (target, expansion - step);
        layOut();
    }

    for (auto* panel : panels)
        if (panel->isVisible())
            panel->setFrame (frame, seconds);
}

void Dashboard::resized()
{
    layOut();
}

void Dashboard::layOut()
{
    const auto eased = DashboardLayout::ease (expansion);
    const auto bounds = DashboardLayout::expanding (getLocalBounds(), shown, expanded, eased);
    const auto expandable = view != dashboardView || std::count (shown.begin(), shown.end(), true) > 1;

    for (size_t i = 0; i < panels.size(); ++i)
    {
        auto& panel = *panels[i];
        const auto isExpanded = i == static_cast<size_t> (expanded) && expansion > 0.0f;

        panel.setBounds (bounds[i]);
        panel.setExpandable (expandable);
        panel.setAlpha (isExpanded ? 1.0f : 1.0f - eased);
        panel.setVisible (isExpanded || (shown[i] && expansion < 1.0f));

        if (isExpanded)
            panel.toFront (false);
    }
}
