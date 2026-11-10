#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"
#include "PluginEditor.h"
#include "shared/MeterScales.h"
#include "ui/meter/MeterText.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

template <typename T>
static std::vector<T*> descendantsOf (juce::Component& parent)
{
    std::vector<T*> found;

    for (auto* child : parent.getChildren())
    {
        if (auto* match = dynamic_cast<T*> (child))
            found.push_back (match);

        const auto below = descendantsOf<T> (*child);
        found.insert (found.end(), below.begin(), below.end());
    }

    return found;
}

static juce::TextButton& buttonNamed (juce::Component& parent, const juce::String& text)
{
    for (auto* button : descendantsOf<juce::TextButton> (parent))
        if (button->getButtonText() == text)
            return *button;

    FAIL ("No button " << text);
    std::abort();
}

static void click (juce::Component& component, juce::Point<int> position)
{
    const auto event = juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), position.toFloat(), {}, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                         &component, &component, juce::Time::getCurrentTime(), position.toFloat(), juce::Time::getCurrentTime(), 1, false);
    component.mouseUp (event);
}

//==============================================================================
TEST_CASE ("The dashboard puts the spectrum over the stereo field and the loudness", "[layout]")
{
    using namespace DashboardLayout;

    // The same cases as webui/test/dashboardLayout.test.ts
    const auto panels = dashboard ({ 0, 44, 960, 556 });
    CHECK (panels[spectrum] == juce::Rectangle<int> (10, 54, 940, 305));
    CHECK (panels[stereo] == juce::Rectangle<int> (10, 369, 372, 221));
    CHECK (panels[loudness] == juce::Rectangle<int> (392, 369, 558, 221));
}

TEST_CASE ("An expanding panel grows from its place to fill the dashboard", "[layout]")
{
    using namespace DashboardLayout;

    const juce::Rectangle<int> bounds { 0, 44, 960, 556 };

    CHECK (expanding (bounds, stereo, 0.0f) == dashboard (bounds));
    CHECK (expanding (bounds, stereo, 1.0f)[stereo] == juce::Rectangle<int> (10, 54, 940, 536));
    CHECK (expanding (bounds, stereo, 0.25f)[stereo] == juce::Rectangle<int> (10, 290, 514, 300));

    // The others stay where they were, to fade out
    CHECK (expanding (bounds, stereo, 1.0f)[spectrum] == dashboard (bounds)[spectrum]);

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

TEST_CASE_METHOD (ProcessorFixture, "The theme and reactivity buttons step through their choices", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);

    auto& theme = buttonNamed (editor, "NEON");
    theme.onClick();
    CHECK_THAT (getParameter (processor, Parameters::themeId), WithinAbs (1.0, 1.0e-4));
    CHECK (theme.getButtonText() == "EMBER");

    // The header takes the theme's colours
    CHECK (editor.getLookAndFeel().findColour (HeaderBar::titleColourId) == MeterTheme::of (1).primary);

    setParameter (processor, Parameters::themeId, 4.0f);
    CHECK (theme.getButtonText() == "ICE");
    theme.onClick();
    CHECK_THAT (getParameter (processor, Parameters::themeId), WithinAbs (0.0, 1.0e-4));

    auto& reactivity = buttonNamed (editor, "FX FULL");
    reactivity.onClick();
    CHECK (reactivity.getButtonText() == "FX OFF");
    CHECK_THAT (getParameter (processor, Parameters::reactivityId), WithinAbs (0.0, 1.0e-4));
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

                if (image.getPixelAt (2, size.y - 3) != MeterTheme::of (theme).background)
                    failed.add (juce::String (size.x) + "x" + juce::String (size.y) + " " + ParameterSpecs::themeNames[static_cast<size_t> (theme)]
                                + " " + ParameterSpecs::viewNames[static_cast<size_t> (view)]);
            }
        }
    }

    // Each one drew, around its panels, in its theme's background
    CHECK (failed.joinIntoString (", ") == "");
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "About shows the version and build type, the commit and the UI", "[editor]")
{
    const auto lines = juce::StringArray::fromLines (About::text (processor));

    REQUIRE (lines.size() == 5);
    CHECK (lines[0] == "Version " JucePlugin_VersionString);
   #if JUCE_DEBUG
    CHECK (lines[1] == "Debug build");
   #else
    CHECK (lines[1] == "Release build");
   #endif
    CHECK (lines[2] == juce::String ("Commit ") + About::commit);
    CHECK (lines[3] == "Undefined");
    CHECK (lines[4] == "Native UI");
}

TEST_CASE_METHOD (ProcessorFixture, "The title fits the header without being cut short, and the header's controls fit at the smallest size", "[editor]")
{
    AudioPluginAudioProcessorEditor editor (processor);
    const auto headers = descendantsOf<HeaderBar> (editor);
    REQUIRE (headers.size() == 1);

    const auto& layout = AudioPluginAudioProcessorEditor::headerLayout;
    const auto width = juce::GlyphArrangement::getStringWidth (HeaderBar::titleFont (layout), headers[0]->getTitle());

    // drawFittedText only squeezes a title that's narrower than this
    CHECK (width * HeaderBar::minimumTitleScale < layout.titleWidth);

    editor.setSize (EditorSize::minWidth, EditorSize::minHeight);

    // The preset's name keeps room to read
    for (auto* button : descendantsOf<juce::TextButton> (*headers[0]))
        if (button->getButtonText() == "Init")
            CHECK (button->getWidth() >= 80);
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
