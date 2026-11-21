#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"
#include "PluginEditor.h"
#include "shared/MeterScales.h"
#include "ui/meter/MeterText.h"
#include <EditorFixtures.h>

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;
using EditorFixtures::descendantsOf;

static juce::TextButton& buttonNamed (juce::Component& parent, const juce::String& text)
{
    for (auto* button : descendantsOf<juce::TextButton> (parent))
        if (button->getButtonText() == text)
            return *button;

    FAIL ("No button " << text);
    std::abort();
}

static juce::MouseEvent mouseEvent (juce::Component& component, juce::Point<int> position, juce::Point<int> pressedAt,
                                    juce::ModifierKeys button = juce::ModifierKeys::leftButtonModifier)
{
    return juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), position.toFloat(), button, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                             &component, &component, juce::Time::getCurrentTime(), pressedAt.toFloat(), juce::Time::getCurrentTime(), 1, false);
}

static void click (juce::Component& component, juce::Point<int> position)
{
    component.mouseUp (mouseEvent (component, position, position, {}));
}

static void drag (juce::Component& component, juce::Point<int> from, juce::Point<int> to,
                  juce::ModifierKeys button = juce::ModifierKeys::leftButtonModifier)
{
    component.mouseDown (mouseEvent (component, from, from, button));
    component.mouseDrag (mouseEvent (component, to, from, button));
}

//==============================================================================
// In the order of DashboardLayout::Panel: spectrum, stereo, loudness, waveform
static constexpr DashboardLayout::Shown noWaveform { true, true, true, false };

TEST_CASE ("The dashboard puts the spectrum over the waveform, over the stereo field and the loudness", "[layout]")
{
    using namespace DashboardLayout;

    // The same cases as webui/test/dashboardLayout.test.ts
    const auto panels = dashboard ({ 0, 44, 960, 556 });
    CHECK (panels[spectrum] == juce::Rectangle<int> (10, 54, 940, 239));
    CHECK (panels[waveform] == juce::Rectangle<int> (10, 303, 940, 103));
    CHECK (panels[stereo] == juce::Rectangle<int> (10, 416, 372, 174));
    CHECK (panels[loudness] == juce::Rectangle<int> (392, 416, 558, 174));
}

TEST_CASE ("The panels that are shown fill the dashboard", "[layout]")
{
    using namespace DashboardLayout;

    const juce::Rectangle<int> bounds { 0, 44, 960, 556 };

    const auto without = dashboard (bounds, noWaveform);
    CHECK (without[spectrum] == juce::Rectangle<int> (10, 54, 940, 305));
    CHECK (without[stereo] == juce::Rectangle<int> (10, 369, 372, 221));
    CHECK (without[loudness] == juce::Rectangle<int> (392, 369, 558, 221));
    CHECK (without[waveform].isEmpty());

    const auto upper = dashboard (bounds, { true, false, false, true });
    CHECK (upper[spectrum] == juce::Rectangle<int> (10, 54, 940, 367));
    CHECK (upper[waveform] == juce::Rectangle<int> (10, 431, 940, 159));

    CHECK (dashboard (bounds, { false, false, true, false })[loudness] == juce::Rectangle<int> (10, 54, 940, 536));
}

TEST_CASE ("A short dashboard is one row", "[layout]")
{
    using namespace DashboardLayout;

    // The editor at its smallest
    const juce::Rectangle<int> bounds { 0, 44, 800, 176 };

    const auto panels = dashboard (bounds);
    CHECK (panels[spectrum] == juce::Rectangle<int> (10, 54, 204, 156));
    CHECK (panels[waveform] == juce::Rectangle<int> (224, 54, 204, 156));
    CHECK (panels[stereo] == juce::Rectangle<int> (438, 54, 136, 156));
    CHECK (panels[loudness] == juce::Rectangle<int> (584, 54, 206, 156));

    const auto two = dashboard (bounds, { false, true, true, false });
    CHECK (two[stereo] == juce::Rectangle<int> (10, 54, 308, 156));
    CHECK (two[loudness] == juce::Rectangle<int> (328, 54, 462, 156));

    // Two rows from the height where the lower one has room
    CHECK (dashboard ({ 0, 44, 800, rowBelowHeight - 1 })[spectrum].getWidth() == 204);
    CHECK (dashboard ({ 0, 44, 800, rowBelowHeight })[spectrum].getWidth() == 780);
}

TEST_CASE ("The gaps between the dashboard's panels are its dividers", "[layout]")
{
    using namespace DashboardLayout;

    // The same cases as webui/test/dashboardLayout.test.ts
    const juce::Rectangle<int> bounds { 0, 44, 960, 556 };

    CHECK (dividers (bounds) == std::vector<Divider> { { Group::rows, 0, 1, false, { 10, 293, 940, 10 } },
                                                       { Group::rows, 1, 2, false, { 10, 406, 940, 10 } },
                                                       { Group::lower, 0, 1, true, { 382, 416, 10, 174 } } });

    // Between the neighbours that are shown
    CHECK (dividers (bounds, noWaveform) == std::vector<Divider> { { Group::rows, 0, 2, false, { 10, 359, 940, 10 } },
                                                                   { Group::lower, 0, 1, true, { 382, 369, 10, 221 } } });

    CHECK (dividers ({ 0, 44, 800, 176 }) == std::vector<Divider> { { Group::oneRow, 0, 1, true, { 214, 54, 10, 156 } },
                                                                    { Group::oneRow, 1, 2, true, { 428, 54, 10, 156 } },
                                                                    { Group::oneRow, 2, 3, true, { 574, 54, 10, 156 } } });

    CHECK (dividers (bounds, { false, false, true, false }).empty());
}

TEST_CASE ("Dragging a divider shares its two neighbours' space again, and moves no other panel", "[layout]")
{
    using namespace DashboardLayout;

    const juce::Rectangle<int> bounds { 0, 44, 960, 556 };
    const auto panels = dashboard (bounds, allShown, dragged (bounds, allShown, {}, dividers (bounds)[0], 30));

    CHECK (panels[spectrum] == juce::Rectangle<int> (10, 54, 940, 269));
    CHECK (panels[waveform] == juce::Rectangle<int> (10, 333, 940, 73));
    CHECK (panels[stereo] == juce::Rectangle<int> (10, 416, 372, 174));
    CHECK (panels[loudness] == juce::Rectangle<int> (392, 416, 558, 174));
}

TEST_CASE ("A drag that moves nothing leaves the sizes as they were", "[layout]")
{
    using namespace DashboardLayout;

    const juce::Rectangle<int> bounds { 0, 44, 960, 556 };

    CHECK (dragged (bounds, allShown, {}, dividers (bounds)[0], 0) == Sizes {});
}

TEST_CASE ("A drag stops at the smallest a panel can be", "[layout]")
{
    using namespace DashboardLayout;

    const juce::Rectangle<int> bounds { 0, 44, 960, 556 };
    const auto underSpectrum = dividers (bounds)[0];

    CHECK (dashboard (bounds, allShown, dragged (bounds, allShown, {}, underSpectrum, 500))[waveform] == juce::Rectangle<int> (10, 342, 940, minPanelHeight));
    CHECK (dashboard (bounds, allShown, dragged (bounds, allShown, {}, underSpectrum, -500))[spectrum] == juce::Rectangle<int> (10, 54, 940, minPanelHeight));

    const auto wider = dashboard (bounds, allShown, dragged (bounds, allShown, {}, dividers (bounds)[2], 500));
    CHECK (wider[stereo] == juce::Rectangle<int> (10, 416, 810, 174));
    CHECK (wider[loudness] == juce::Rectangle<int> (830, 416, minPanelWidth, 174));
}

TEST_CASE ("A panel that isn't showing keeps its share through a drag, and a reset puts one group back", "[layout]")
{
    using namespace DashboardLayout;

    const juce::Rectangle<int> bounds { 0, 44, 960, 556 };

    const auto withoutWaveform = dragged (bounds, noWaveform, {}, dividers (bounds, noWaveform)[0], -5);
    CHECK (dashboard (bounds, noWaveform, withoutWaveform)[spectrum].getHeight() == 300);
    CHECK (dashboard (bounds, allShown, withoutWaveform)[waveform].getHeight() == 103);

    const auto lowerDragged = dragged (bounds, allShown, {}, dividers (bounds)[2], 500);
    const auto both = dragged (bounds, allShown, lowerDragged, dividers (bounds)[0], 30);
    CHECK (reset (both, Group::rows) == lowerDragged);
    CHECK (reset (lowerDragged, Group::lower) == Sizes {});
}

TEST_CASE ("An expanding panel grows from its place to fill the dashboard", "[layout]")
{
    using namespace DashboardLayout;

    const juce::Rectangle<int> bounds { 0, 44, 960, 556 };

    CHECK (expanding (bounds, noWaveform, stereo, 0.0f) == dashboard (bounds, noWaveform));
    CHECK (expanding (bounds, noWaveform, stereo, 1.0f)[stereo] == juce::Rectangle<int> (10, 54, 940, 536));
    CHECK (expanding (bounds, noWaveform, stereo, 0.25f)[stereo] == juce::Rectangle<int> (10, 290, 514, 300));

    // The others stay where they were, to fade out
    CHECK (expanding (bounds, noWaveform, stereo, 1.0f)[spectrum] == dashboard (bounds, noWaveform)[spectrum]);

    // One the dashboard doesn't show grows from the middle
    CHECK (expanding (bounds, noWaveform, waveform, 0.5f)[waveform] == juce::Rectangle<int> (245, 188, 470, 268));

    CHECK_THAT (ease (0.0f), WithinAbs (0.0, 0.0));
    CHECK_THAT (ease (0.25f), WithinAbs (0.15625, 1.0e-6));
    CHECK_THAT (ease (0.5f), WithinAbs (0.5, 1.0e-6));
    CHECK_THAT (ease (1.0f), WithinAbs (1.0, 0.0));
}

TEST_CASE ("Values sit on the scales where the web UI puts them", "[layout]")
{
    using namespace MeterScales;

    // The same cases as webui/test/scales.test.ts
    CHECK_THAT (proportion (-45.0f, spectrumBottomDb, spectrumTopDb), WithinAbs (0.5, 1.0e-6));
    CHECK_THAT (proportion (40.0f, spectrumBottomDb, spectrumTopDb), WithinAbs (1.0, 0.0));
    CHECK_THAT (proportion (-200.0f, levelBottom, levelTop), WithinAbs (0.0, 0.0));
    CHECK_THAT (proportion (-14.0f, levelBottom, levelTop), WithinAbs (0.666667, 1.0e-5));

    CHECK_THAT (acrossSpectrum (20.0f), WithinAbs (0.0, 1.0e-6));
    CHECK_THAT (acrossSpectrum (1000.0f), WithinAbs (0.566323, 1.0e-5));
    CHECK_THAT (acrossSpectrum (20000.0f), WithinAbs (1.0, 1.0e-6));
}

TEST_CASE ("The waveform's newest column is at the right, and they all move left as the next one fills", "[layout]")
{
    // The same cases as webui/test/viewLayout.test.ts: a plot a pixel wide for each column
    const juce::Rectangle<float> plot { 10.0f, 0.0f, static_cast<float> (MeterFrame::numColumns), 100.0f };
    const auto newest = MeterFrame::numColumns - 1;

    CHECK_THAT (WaveformView::columnX (plot, newest, 0.0f), WithinAbs (plot.getRight(), 1.0e-4));
    CHECK_THAT (WaveformView::columnX (plot, newest, 0.5f), WithinAbs (plot.getRight() - 0.5, 1.0e-4));
    CHECK_THAT (WaveformView::columnX (plot, 0, 0.0f), WithinAbs (11.0, 1.0e-4));
    CHECK_THAT (WaveformView::columnX (plot, 0, 1.0f), WithinAbs (10.0, 1.0e-4));
}

TEST_CASE ("The stereo field's scope is square, with its bars under it or beside it in a wide panel", "[layout]")
{
    // The same cases as webui/test/viewLayout.test.ts
    const auto tall = StereoFieldView::layOut ({ 12.0f, 28.0f, 300.0f, 400.0f });
    CHECK (tall.scope == juce::Rectangle<float> (12.0f, 49.0f, 300.0f, 300.0f));
    CHECK (tall.correlation == juce::Rectangle<float> (12.0f, 376.0f, 300.0f, 26.0f));
    CHECK (tall.balance == juce::Rectangle<float> (12.0f, 402.0f, 300.0f, 26.0f));

    const auto wide = StereoFieldView::layOut ({ 12.0f, 28.0f, 348.0f, 181.0f });
    CHECK (wide.scope == juce::Rectangle<float> (12.0f, 28.0f, 181.0f, 181.0f));
    CHECK (wide.correlation == juce::Rectangle<float> (207.0f, 89.5f, 153.0f, 26.0f));
    CHECK (wide.balance == juce::Rectangle<float> (207.0f, 121.5f, 153.0f, 26.0f));
}

TEST_CASE ("The loudness panel has its bars at the left and its readings in a grid", "[layout]")
{
    // The same cases as webui/test/viewLayout.test.ts
    const auto layout = LoudnessView::layOut ({ 12.0f, 28.0f, 534.0f, 181.0f });

    CHECK (layout.scale == juce::Rectangle<float> (12.0f, 28.0f, 22.0f, 167.0f));
    CHECK (layout.bars[0] == juce::Rectangle<float> (39.0f, 28.0f, 35.0f, 181.0f));
    CHECK (layout.bars[1] == juce::Rectangle<float> (79.0f, 28.0f, 35.0f, 181.0f));
    CHECK (layout.bars[2] == juce::Rectangle<float> (128.0f, 28.0f, 35.0f, 181.0f));
    CHECK (layout.bars[3] == juce::Rectangle<float> (168.0f, 28.0f, 35.0f, 181.0f));
    CHECK (layout.integrated == juce::Rectangle<float> (220.0f, 28.0f, 326.0f, 83.0f));
    CHECK (layout.cells[LoudnessView::momentary] == juce::Rectangle<float> (220.0f, 111.0f, 108.0f, 49.0f));
    CHECK (layout.cells[LoudnessView::clips] == juce::Rectangle<float> (328.0f, 160.0f, 108.0f, 49.0f));
    CHECK_FALSE (layout.compact);
}

TEST_CASE ("With little room the loudness panel has narrower bars and three readings", "[layout]")
{
    // The same cases as webui/test/viewLayout.test.ts. In one row at the editor's smallest: one reading over the next
    const auto narrow = LoudnessView::layOut ({ 12.0f, 28.0f, 182.0f, 116.0f });

    CHECK (narrow.compact);
    CHECK (narrow.scale == juce::Rectangle<float> (12.0f, 28.0f, 22.0f, 102.0f));
    CHECK (narrow.bars[0] == juce::Rectangle<float> (39.0f, 28.0f, 11.0f, 116.0f));
    CHECK (narrow.bars[3] == juce::Rectangle<float> (96.0f, 28.0f, 11.0f, 116.0f));
    CHECK (narrow.integrated == juce::Rectangle<float> (120.0f, 28.0f, 74.0f, 38.0f));
    CHECK (narrow.cells[LoudnessView::shortTerm] == juce::Rectangle<float> (120.0f, 66.0f, 74.0f, 38.0f));
    CHECK (narrow.cells[LoudnessView::truePeak] == juce::Rectangle<float> (120.0f, 104.0f, 74.0f, 38.0f));
    CHECK (narrow.cells[LoudnessView::momentary].isEmpty());

    // Short but wide: side by side
    const auto wide = LoudnessView::layOut ({ 12.0f, 28.0f, 438.0f, 100.0f });

    CHECK (wide.integrated == juce::Rectangle<float> (154.0f, 28.0f, 98.0f, 100.0f));
    CHECK (wide.cells[LoudnessView::shortTerm] == juce::Rectangle<float> (252.0f, 28.0f, 98.0f, 100.0f));
    CHECK (wide.cells[LoudnessView::truePeak] == juce::Rectangle<float> (350.0f, 28.0f, 98.0f, 100.0f));
}

TEST_CASE ("Readings are written to one place, or as dashes with nothing to show", "[text]")
{
    // The same cases as webui/test/text.test.ts
    CHECK (MeterText::loudness (-23.04f) == "-23.0");
    CHECK (MeterText::loudness (-8.96f) == "-9.0");
    CHECK (MeterText::loudness (4.2f) == "4.2");
    CHECK (MeterText::loudness (MeterFrame::noLoudness) == "--.-");

    CHECK (MeterText::truePeak (-1.23f) == "-1.2");
    CHECK (MeterText::truePeak (0.36f) == "+0.4");
    CHECK (MeterText::truePeak (-0.04f) == "0.0");
    CHECK (MeterText::truePeak (12.0f) == "+12.0");
    CHECK (MeterText::truePeak (MeterFrame::noLoudness) == "--.-");

    CHECK (MeterText::clips (0.0f) == "0");
    CHECK (MeterText::clips (37.0f) == "37");
    CHECK (MeterText::clips (1000.0f) == "999+");
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "The editor opens at its default size, and at the size it was left", "[editor]")
{
    {
        AudioPluginAudioProcessorEditor editor (processor);
        CHECK (editor.getWidth() == EditorSize::defaultWidth);
        CHECK (editor.getHeight() == EditorSize::defaultHeight);
        CHECK (editor.isResizable());

        editor.setSize (1100, 700);
    }

    CHECK (processor.getEditorSize() == juce::Point<int> (1100, 700));

    AudioPluginAudioProcessorEditor reopened (processor);
    CHECK (reopened.getWidth() == 1100);
    CHECK (reopened.getHeight() == 700);

    const auto headerHeight = AudioPluginAudioProcessorEditor::headerLayout.height;
    CHECK (reopened.getDashboard().getBounds() == juce::Rectangle<int> (0, headerHeight, 1100, 700 - headerHeight));
}

static std::vector<juce::PopupMenu::Item> itemsOf (const juce::PopupMenu& menu)
{
    std::vector<juce::PopupMenu::Item> items;

    for (juce::PopupMenu::MenuItemIterator it (menu); it.next();)
        items.push_back (it.getItem());

    return items;
}

TEST_CASE_METHOD (ProcessorFixture, "The theme button's menu lists the themes, and choosing one sets it", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    auto& theme = buttonNamed (editor, "NEON");

    juce::StringArray listed, ticked;

    for (const auto& item : itemsOf (editor.themeMenu()))
    {
        listed.add (item.text);

        if (item.isTicked)
            ticked.add (item.text);
    }

    // Then a separator and the way to the custom theme's editor
    juce::StringArray expected (ParameterSpecs::themeNames.data(), static_cast<int> (ParameterSpecs::themeNames.size()));
    expected.addArray ({ "", "Edit custom..." });

    CHECK (listed == expected);
    CHECK (ticked == juce::StringArray ("Neon"));

    itemsOf (editor.themeMenu())[1].action();
    CHECK_THAT (getParameter (processor, Parameters::themeId), WithinAbs (1.0, 1.0e-4));
    CHECK (theme.getButtonText() == "EMBER");
    CHECK (itemsOf (editor.themeMenu())[1].isTicked);

    // The header takes the theme's colours
    CHECK (editor.getLookAndFeel().findColour (HeaderBar::titleColourId) == MeterTheme::of (1).primary);

}

TEST_CASE_METHOD (ProcessorFixture, "The custom theme's editor sets the base and the accent colours, and the meter takes them", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    auto& custom = editor.getCustomThemeEditor();

    itemsOf (editor.themeMenu()).back().action();
    CHECK_THAT (getParameter (processor, Parameters::themeId), WithinAbs (Themes::customIndex, 1.0e-4));
    CHECK (custom.isVisible());

    // The picker, on the first accent. The same colours as webui/test/colour.test.ts
    custom.setHue (0.75f);
    custom.setSaturationAndValue (1.0f, 0.5f);
    CHECK (processor.getCustomTheme().primary == 0x400080);
    CHECK (editor.getLookAndFeel().findColour (HeaderBar::titleColourId) == juce::Colour (0xff400080));

    custom.select (CustomThemeEditor::hot);
    custom.setHex ("#00FF80");
    custom.setHex ("no colour");
    CHECK (processor.getCustomTheme().hot == 0x00ff80);

    // A swatch pressed after a drag along the hue strip, with the mouse moving on it, only selects
    const auto layout = custom.getLayout();
    drag (custom, layout.hue.getCentre(), layout.hue.getCentre());
    const auto picked = processor.getCustomTheme();
    drag (custom, layout.swatches[CustomThemeEditor::secondary].getCentre(), layout.swatches[CustomThemeEditor::secondary].getCentre().translated (1, 0));
    CHECK (processor.getCustomTheme() == picked);

    // A look or a session loading under the editor shows on the next frame
    processor.setCustomTheme ({ 3, 0x112233, 0x445566, 0x778899 });
    editor.showNextFrame (0.0);
    CHECK (editor.getLookAndFeel().findColour (HeaderBar::titleColourId) == juce::Colour (0xff112233));
    CHECK (editor.getLookAndFeel().findColour (HeaderBar::backgroundColourId) == MeterTheme::of (3).panel);
}

TEST_CASE ("A theme is a built-in palette, or the custom one's base with its accents", "[theme]")
{
    // The same cases as webui/test/frame.test.ts
    const Themes::Custom custom { 1, 0x112233, 0x445566, 0x778899 };

    CHECK (Themes::coloursOf (Themes::paletteOf (1, custom)) == Themes::coloursOf (Themes::all[1]));

    const auto palette = Themes::paletteOf (Themes::customIndex, custom);
    CHECK (palette.background == Themes::all[1].background);
    CHECK (palette.primary == 0x112233);
    CHECK (palette.secondary == 0x445566);
    CHECK (palette.hot == 0x778899);

    CHECK (Themes::toHex (0x00a0ff) == "00a0ff");
    CHECK (Themes::fromHex ("19e6ff") == 0x19e6ffu);
    CHECK (Themes::fromHex ("#FF2BD6") == 0xff2bd6u);
    CHECK (Themes::fromHex ("12345") == std::nullopt);
    CHECK (Themes::fromHex ("zzzzzz") == std::nullopt);
}

TEST_CASE ("The custom theme's editor has its controls at the left and the picker at the right", "[layout]")
{
    using namespace CustomThemeLayout;

    // The same cases as webui/test/customThemeLayout.test.ts
    const auto layout = layOut ({ 120, 16, panelWidth, panelHeight });

    CHECK (layout.title == juce::Rectangle<int> (136, 28, 458, 24));
    CHECK (layout.close == juce::Rectangle<int> (594, 28, 70, 24));
    CHECK (layout.baseLabel == juce::Rectangle<int> (136, 60, 250, 12));
    CHECK (layout.base == juce::Rectangle<int> (136, 72, 250, 24));
    CHECK (layout.coloursLabel == juce::Rectangle<int> (136, 104, 250, 12));
    CHECK (layout.swatches[0] == juce::Rectangle<int> (136, 116, 78, 44));
    CHECK (layout.swatches[2] == juce::Rectangle<int> (308, 116, 78, 44));
    CHECK (layout.hex == juce::Rectangle<int> (136, 168, 250, 24));
    CHECK (layout.square == juce::Rectangle<int> (406, 60, 258, 108));
    CHECK (layout.hue == juce::Rectangle<int> (406, 176, 258, 16));
}

static juce::PopupMenu::Item viewItem (AudioPluginAudioProcessorEditor& editor, const juce::String& text)
{
    for (const auto& item : itemsOf (editor.viewMenu()))
        if (item.text == text)
            return item;

    FAIL ("No item " << text);
    std::abort();
}

TEST_CASE_METHOD (ProcessorFixture, "The view menu shows and hides the dashboard's panels, and sets the reactivity", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    auto& dashboard = editor.getDashboard();

    CHECK (viewItem (editor, "Waveform").isTicked);
    viewItem (editor, "Waveform").action();

    CHECK_THAT (getParameter (processor, Parameters::showWaveformId), WithinAbs (0.0, 1.0e-4));
    CHECK_FALSE (viewItem (editor, "Waveform").isTicked);
    CHECK_FALSE (dashboard.getPanel (DashboardLayout::waveform).isVisible());
    CHECK (dashboard.getPanel (DashboardLayout::spectrum).getBounds() == DashboardLayout::dashboard (dashboard.getLocalBounds(), noWaveform)[DashboardLayout::spectrum]);

    CHECK (viewItem (editor, "Full").isTicked);
    viewItem (editor, "Off").action();
    CHECK_THAT (getParameter (processor, Parameters::reactivityId), WithinAbs (0.0, 1.0e-4));
    CHECK (viewItem (editor, "Off").isTicked);
}

TEST_CASE_METHOD (ProcessorFixture, "Dragging the gap between two panels resizes them, and a double-click there puts them back", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    auto& dashboard = editor.getDashboard();
    auto& spectrum = dashboard.getPanel (DashboardLayout::spectrum);

    const auto height = spectrum.getHeight();
    const auto gap = dashboard.getDividers()[0].area.getCentre();

    drag (dashboard, gap, gap.translated (0, 30));
    CHECK (spectrum.getHeight() == height + 30);
    CHECK (processor.getPanelSizes() != DashboardLayout::Sizes {});

    dashboard.mouseDoubleClick (mouseEvent (dashboard, gap.translated (0, 30), gap.translated (0, 30)));
    CHECK (spectrum.getHeight() == height);
    CHECK (processor.getPanelSizes() == DashboardLayout::Sizes {});

    // A look or a session loading under the editor shows on the next frame
    DashboardLayout::Sizes loaded;
    loaded.rows = { 1.0, 1.0, 1.0 };
    processor.setPanelSizes (loaded);
    editor.showNextFrame (0.0);
    CHECK (spectrum.getBounds() == DashboardLayout::dashboard (dashboard.getLocalBounds(), DashboardLayout::allShown, loaded)[DashboardLayout::spectrum]);

    // An expanded panel has no gaps to drag
    setParameter (processor, Parameters::viewId, 1.0f);
    CHECK (dashboard.getDividers().empty());
}

TEST_CASE_METHOD (ProcessorFixture, "Only the left button drags a gap, and a press that moves nothing isn't an edit", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    auto& dashboard = editor.getDashboard();
    const auto gap = dashboard.getDividers()[0].area.getCentre();

    drag (dashboard, gap, gap.translated (0, 30), juce::ModifierKeys::rightButtonModifier);
    CHECK (processor.getPanelSizes() == DashboardLayout::Sizes {});

    drag (dashboard, gap, gap.translated (2, 0));
    CHECK (processor.getPanelSizes() == DashboardLayout::Sizes {});
}

TEST_CASE_METHOD (ProcessorFixture, "The dashboard's last panel can't be hidden, and has nothing to expand over", "[editor]")
{
    for (const auto* id : { &Parameters::showSpectrumId, &Parameters::showWaveformId, &Parameters::showStereoId })
        setParameter (processor, *id, 0.0f);

    AudioPluginAudioProcessorEditor editor (processor);
    auto& dashboard = editor.getDashboard();
    auto& loudness = dashboard.getPanel (DashboardLayout::loudness);

    CHECK_FALSE (viewItem (editor, "Loudness").isEnabled);
    CHECK (viewItem (editor, "Spectrum").isEnabled);
    CHECK (loudness.getBounds() == dashboard.getLocalBounds().reduced (DashboardLayout::padding));

    click (loudness, loudness.getExpandButtonBounds().getCentre());
    CHECK_THAT (getParameter (processor, Parameters::viewId), WithinAbs (0.0, 1.0e-4));
}

TEST_CASE_METHOD (ProcessorFixture, "A panel the dashboard doesn't show can still fill the editor", "[editor]")
{
    setParameter (processor, Parameters::showStereoId, 0.0f);
    setParameter (processor, Parameters::viewId, 2.0f);

    AudioPluginAudioProcessorEditor editor (processor);
    auto& dashboard = editor.getDashboard();
    auto& stereo = dashboard.getPanel (DashboardLayout::stereo);

    CHECK (stereo.isVisible());
    CHECK (stereo.getBounds() == dashboard.getLocalBounds().reduced (DashboardLayout::padding));
}

TEST_CASE_METHOD (ProcessorFixture, "A panel's corner button expands it over the dashboard, and again brings the dashboard back", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    auto& dashboard = editor.getDashboard();
    auto& stereo = dashboard.getPanel (DashboardLayout::stereo);
    auto& spectrum = dashboard.getPanel (DashboardLayout::spectrum);

    const auto home = DashboardLayout::dashboard (dashboard.getLocalBounds());
    CHECK (stereo.getBounds() == home[DashboardLayout::stereo]);

    click (stereo, stereo.getExpandButtonBounds().getCentre());
    CHECK_THAT (getParameter (processor, Parameters::viewId), WithinAbs (2.0, 1.0e-4));

    // It grows over a few frames
    editor.showNextFrame (0.05);
    CHECK (stereo.getWidth() > home[DashboardLayout::stereo].getWidth());
    CHECK (stereo.getWidth() < dashboard.getWidth() - 2 * DashboardLayout::padding);
    CHECK (spectrum.isVisible());

    editor.showNextFrame (1.0);
    CHECK (stereo.getBounds() == dashboard.getLocalBounds().reduced (DashboardLayout::padding));
    CHECK_FALSE (spectrum.isVisible());

    click (stereo, stereo.getExpandButtonBounds().getCentre());
    CHECK_THAT (getParameter (processor, Parameters::viewId), WithinAbs (0.0, 1.0e-4));
    editor.showNextFrame (1.0);
    CHECK (stereo.getBounds() == home[DashboardLayout::stereo]);
    CHECK (spectrum.isVisible());
}

TEST_CASE_METHOD (ProcessorFixture, "The editor opens in the saved view without animating", "[editor]")
{
    setParameter (processor, Parameters::viewId, 3.0f);

    AudioPluginAudioProcessorEditor editor (processor);
    auto& dashboard = editor.getDashboard();

    CHECK (dashboard.getPanel (DashboardLayout::loudness).getBounds() == dashboard.getLocalBounds().reduced (DashboardLayout::padding));
    CHECK_FALSE (dashboard.getPanel (DashboardLayout::spectrum).isVisible());
}

TEST_CASE_METHOD (ProcessorFixture, "Capture mode leaves only the meter, until its exit button is pressed", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    auto* header = descendantsOf<HeaderBar> (editor).front();
    auto& exit = buttonNamed (editor, "EXIT CAPTURE");

    CHECK (header->isVisible());
    CHECK_FALSE (exit.isVisible());

    buttonNamed (editor, "CAPTURE").onClick();
    CHECK (editor.isCaptureMode());
    CHECK_FALSE (header->isVisible());
    CHECK (editor.getDashboard().getY() == 0);
    CHECK_FALSE (exit.isVisible());

    // Moving the mouse brings the way out
    editor.mouseMove (juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), {}, {}, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, &editor, &editor,
                                        juce::Time::getCurrentTime(), {}, juce::Time::getCurrentTime(), 0, false));
    CHECK (exit.isVisible());

    exit.onClick();
    CHECK_FALSE (editor.isCaptureMode());
    CHECK (header->isVisible());
    CHECK (editor.getDashboard().getY() == AudioPluginAudioProcessorEditor::headerLayout.height);
}

TEST_CASE_METHOD (ProcessorFixture, "The editor draws what the engine hears, and RESET starts it again", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    auto& engine = processor.getMeterEngine();

    inBlocks (makeTone (2, 3.0, 100.0, 1.5f), [this] (juce::AudioBuffer<float>& block) { process (processor, block); });
    editor.showNextFrame (0.016);

    // The spectrum's trace is up where the tone is, in the theme's colours
    auto& spectrum = editor.getDashboard().getPanel (DashboardLayout::spectrum);
    const auto image = spectrum.createComponentSnapshot (spectrum.getLocalBounds(), true, 1.0f);
    const auto plot = SpectrumView::plotIn (spectrum.getContentBounds().toFloat());
    const auto x = juce::roundToInt (plot.getX() + plot.getWidth() * MeterScales::acrossSpectrum (100.0f));
    const auto low = image.getPixelAt (x, juce::roundToInt (plot.getBottom()) - 4);
    const auto empty = image.getPixelAt (juce::roundToInt (plot.getRight()) - 8, juce::roundToInt (plot.getY()) + 4);

    CHECK (empty == MeterTheme::of (0).panel);
    CHECK (low != MeterTheme::of (0).panel);

    CHECK (engine.updateFrame (0.0)[MeterFrame::clipCount] > 0.0f);

    CHECK (engine.updateFrame (0.0)[MeterFrame::integrated] > -10.0f);

    buttonNamed (editor, "RESET").onClick();
    inBlocks (makeTone (2, 1.0, 100.0, 0.1f), [this] (juce::AudioBuffer<float>& block) { process (processor, block); });

    const auto& after = engine.updateFrame (0.016);
    CHECK_THAT (after[MeterFrame::clipCount], WithinAbs (0.0, 0.0));
    CHECK (after[MeterFrame::integrated] < -20.0f);
}

TEST_CASE_METHOD (ProcessorFixture, "Every panel draws at the smallest and largest sizes, in every theme and view", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    inBlocks (makeTone (2, 1.0, 440.0, 0.5f), [this] (juce::AudioBuffer<float>& block) { process (processor, block); });

    // The sizes, themes and views that didn't draw
    juce::StringArray failed;

    for (const auto size : { juce::Point<int> (EditorSize::minWidth, EditorSize::minHeight), juce::Point<int> (EditorSize::maxWidth, EditorSize::maxHeight) })
    {
        editor.setSize (size.x, size.y);

        for (int theme = 0; theme < static_cast<int> (ParameterSpecs::themeNames.size()); ++theme)
        {
            for (int view = 0; view < static_cast<int> (ParameterSpecs::viewNames.size()); ++view)
            {
                setParameter (processor, Parameters::themeId, static_cast<float> (theme));
                setParameter (processor, Parameters::viewId, static_cast<float> (view));
                editor.showNextFrame (1.0);

                const auto image = editor.createComponentSnapshot (editor.getLocalBounds(), true, 1.0f);

                if (image.getPixelAt (2, size.y - 3) != MeterTheme::of (Themes::paletteOf (theme, processor.getCustomTheme())).background)
                    failed.add (juce::String (size.x) + "x" + juce::String (size.y) + " " + ParameterSpecs::themeNames[static_cast<size_t> (theme)]
                                + " " + ParameterSpecs::viewNames[static_cast<size_t> (view)]);
            }
        }
    }

    // Each one drew, around its panels, in its theme's background
    CHECK (failed.joinIntoString (", ") == "");
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "The title fits the header without being cut short, and the header's controls fit at the smallest size", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    const auto headers = descendantsOf<HeaderBar> (editor);
    REQUIRE (headers.size() == 1);

    const auto& layout = AudioPluginAudioProcessorEditor::headerLayout;
    const auto width = juce::GlyphArrangement::getStringWidth (headers[0]->titleFont(), headers[0]->getTitle());

    // drawFittedText only squeezes a title that's narrower than this
    CHECK (width * HeaderBar::minimumTitleScale < layout.titleWidth);

    editor.setSize (EditorSize::minWidth, EditorSize::minHeight);

    // The preset's name keeps room to read
    const auto buttons = descendantsOf<juce::TextButton> (*headers[0]);
    const auto preset = std::find_if (buttons.begin(), buttons.end(), [] (const juce::TextButton* button) { return button->getButtonText() == "Init"; });

    REQUIRE (preset != buttons.end());
    CHECK ((*preset)->getWidth() >= 80);
}

TEST_CASE_METHOD (ProcessorFixture, "The editor's text is Orbitron", "[editor][font]")
{
    AudioPluginAudioProcessorEditor editor (processor);

    for (auto* label : descendantsOf<juce::Label> (editor))
    {
        CAPTURE (label->getText());
        CHECK (editor.getLookAndFeel().getLabelFont (*label).getTypefacePtr()->getName() == "Orbitron");
    }

    for (auto* button : descendantsOf<juce::TextButton> (editor))
    {
        CAPTURE (button->getButtonText());
        CHECK (editor.getLookAndFeel().getTextButtonFont (*button, button->getHeight()).getTypefacePtr()->getName() == "Orbitron");
    }

    CHECK (editor.getLookAndFeel().getPopupMenuFont().getTypefacePtr()->getName() == "Orbitron");
}
