#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

//==============================================================================
// What hosts and saved sessions see. Change it only on purpose
TEST_CASE_METHOD (ProcessorFixture, "Parameters match the snapshot hosts rely on", "[parameters]")
{
    struct Expected
    {
        const char* id;
        int versionHint;
        const char* name;
        const char* label;
        float start, end, interval, skew, defaultValue;
        std::array<const char*, 3> textAt0HalfAnd1;
    };

    const std::vector<Expected> snapshot {
        { "theme", 1, "Theme", "", 0.0f, 5.0f, 1.0f, 1.0f, 0.0f, { "Neon", "Phosphor", "Custom" } },
        { "reactivity", 1, "Reactivity", "", 0.0f, 2.0f, 1.0f, 1.0f, 2.0f, { "Off", "Subtle", "Full" } },
        { "view", 1, "View", "", 0.0f, 4.0f, 1.0f, 1.0f, 0.0f, { "Dashboard", "Stereo", "Waveform" } },
        { "showSpectrum", 1, "Show Spectrum", "", 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, { "Off", "On", "On" } },
        { "showWaveform", 1, "Show Waveform", "", 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, { "Off", "On", "On" } },
        { "showStereo", 1, "Show Stereo", "", 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, { "Off", "On", "On" } },
        { "showLoudness", 1, "Show Loudness", "", 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, { "Off", "On", "On" } },
    };

    const auto& parameters = processor.getParameters();
    REQUIRE (static_cast<size_t> (parameters.size()) == snapshot.size());

    for (size_t i = 0; i < snapshot.size(); ++i)
    {
        const auto& expected = snapshot[i];
        CAPTURE (i, expected.id);

        auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (parameters[static_cast<int> (i)]);
        REQUIRE (parameter != nullptr);

        CHECK (parameter->getParameterID() == expected.id);
        CHECK (parameter->getVersionHint() == expected.versionHint);
        CHECK (parameter->getName (100) == expected.name);
        CHECK (parameter->getLabel() == expected.label);

        const auto& range = parameter->getNormalisableRange();
        CHECK_THAT (range.start, WithinAbs (expected.start, 1.0e-6));
        CHECK_THAT (range.end, WithinAbs (expected.end, 1.0e-6));
        CHECK_THAT (range.interval, WithinAbs (expected.interval, 1.0e-6));
        CHECK_THAT (range.skew, WithinAbs (expected.skew, 1.0e-6));
        CHECK_THAT (parameter->convertFrom0to1 (parameter->getDefaultValue()), WithinAbs (expected.defaultValue, 1.0e-6));

        CHECK (parameter->getText (0.0f, 100) == expected.textAt0HalfAnd1[0]);
        CHECK (parameter->getText (0.5f, 100) == expected.textAt0HalfAnd1[1]);
        CHECK (parameter->getText (1.0f, 100) == expected.textAt0HalfAnd1[2]);

        // How the meter looks isn't for automating
        CHECK_FALSE (parameter->isAutomatable());
    }
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "The audio goes out exactly as it came in", "[dsp]")
{
    // Includes a block larger than the size given to prepareToPlay, which some hosts send
    const auto numSamples = GENERATE (1, 64, blockSize, blockSize * 4);

    // Silence, denormals, full scale and very loud
    const auto amplitude = GENERATE (0.0f, 1.0e-40f, 1.0f, 1000.0f);

    CAPTURE (numSamples, amplitude);

    const auto input = makeTone (2, numSamples / sampleRate, 440.0, amplitude);
    auto buffer = input;
    process (processor, buffer);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        CHECK (std::equal (input.getReadPointer (channel), input.getReadPointer (channel) + numSamples, buffer.getReadPointer (channel)));
}

TEST_CASE_METHOD (ProcessorFixture, "Audio that isn't a number passes through and leaves the meter working", "[dsp]")
{
    auto buffer = makeConstantBuffer (2, blockSize, std::numeric_limits<float>::quiet_NaN());
    buffer.setSample (1, 3, std::numeric_limits<float>::infinity());
    process (processor, buffer);
    CHECK (std::isnan (buffer.getSample (0, 0)));

    inBlocks (makeTone (2, 3.0, 1000.0, 0.5f), [this] (juce::AudioBuffer<float>& block) { process (processor, block); });

    const auto& frame = processor.getMeterEngine().updateFrame (0.016);

    // The first value that isn't a number, if there is one
    const auto broken = std::find_if (frame.begin(), frame.end(), [] (float value) { return ! std::isfinite (value); });
    const auto brokenIndex = broken - frame.begin();

    CAPTURE (brokenIndex);
    CHECK (broken == frame.end());

    CHECK_THAT (frame[MeterFrame::momentary], WithinAbs (-6.02, 0.2));
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "Only matching mono or stereo layouts are supported", "[buses]")
{
    using Set = juce::AudioChannelSet;

    // Each layout is { { input bus }, { output bus } }
    CHECK (processor.isBusesLayoutSupported ({ { Set::mono() },   { Set::mono() } }));
    CHECK (processor.isBusesLayoutSupported ({ { Set::stereo() }, { Set::stereo() } }));

    CHECK_FALSE (processor.isBusesLayoutSupported ({ { Set::mono() },   { Set::stereo() } }));
    CHECK_FALSE (processor.isBusesLayoutSupported ({ { Set::stereo() }, { Set::mono() } }));
    CHECK_FALSE (processor.isBusesLayoutSupported ({ { Set::create5point1() }, { Set::create5point1() } }));
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "State round trips into a new instance, with the editor's size", "[state]")
{
    setParameter (processor, Parameters::themeId, 3.0f);
    setParameter (processor, Parameters::reactivityId, 0.0f);
    setParameter (processor, Parameters::viewId, 2.0f);
    processor.setEditorSize (1200, 700);

    juce::MemoryBlock state;
    processor.getStateInformation (state);

    AudioPluginAudioProcessor restored;
    CHECK (restored.getEditorSize() == juce::Point<int> (EditorSize::defaultWidth, EditorSize::defaultHeight));
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    CHECK_THAT (getParameter (restored, Parameters::themeId), WithinAbs (3.0, 1.0e-4));
    CHECK_THAT (getParameter (restored, Parameters::reactivityId), WithinAbs (0.0, 1.0e-4));
    CHECK_THAT (getParameter (restored, Parameters::viewId), WithinAbs (2.0, 1.0e-4));
    CHECK (restored.getEditorSize() == juce::Point<int> (1200, 700));

    // Kept by the processor, not left where a saved preset would pick it up
    CHECK_FALSE (restored.getAPVTS().state.hasProperty ("editorWidth"));
}

TEST_CASE_METHOD (ProcessorFixture, "The custom theme is saved with the session", "[state]")
{
    const Themes::Custom custom { 2, 0x112233, 0x445566, 0x778899 };
    processor.setCustomTheme (custom);

    juce::MemoryBlock state;
    processor.getStateInformation (state);

    AudioPluginAudioProcessor restored;
    CHECK (restored.getCustomTheme() == Themes::Custom {});

    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    CHECK (restored.getCustomTheme() == custom);
}

TEST_CASE_METHOD (ProcessorFixture, "A custom theme that isn't valid in the state keeps its defaults", "[state]")
{
    juce::ValueTree tree (CustomThemeState::type);
    tree.setProperty (CustomThemeState::baseProp, 99, nullptr);
    tree.setProperty (CustomThemeState::primaryProp, "not a colour", nullptr);
    tree.setProperty (CustomThemeState::hotProp, "00FF80", nullptr);
    processor.getAPVTS().state.appendChild (tree, nullptr);

    const Themes::Custom defaults;
    CHECK (processor.getCustomTheme() == Themes::Custom { Themes::customIndex - 1, defaults.primary, defaults.secondary, 0x00ff80 });
}

TEST_CASE_METHOD (ProcessorFixture, "The panels' sizes are saved with the session", "[state]")
{
    DashboardLayout::Sizes sizes;
    // A hidden panel's weight after a drag is seldom a whole number
    sizes.rows = { 269.0 * 526.0 / 247.0, 131.5, 226.0 };
    sizes.oneRow = { 1.0, 2.0, 3.0, 4.0 };
    processor.setPanelSizes (sizes);
    CHECK (processor.getPanelSizes() == sizes);

    juce::MemoryBlock state;
    processor.getStateInformation (state);

    AudioPluginAudioProcessor restored;
    CHECK (restored.getPanelSizes() == DashboardLayout::Sizes {});

    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    CHECK (restored.getPanelSizes() == sizes);
}

TEST_CASE_METHOD (ProcessorFixture, "Panel sizes that aren't valid in the state keep their defaults", "[state]")
{
    juce::ValueTree tree (PanelSizesState::type);
    tree.setProperty (PanelSizesState::rowsProp, "1 2", nullptr);
    tree.setProperty (PanelSizesState::lowerProp, "3 0", nullptr);
    tree.setProperty (PanelSizesState::oneRowProp, "4 3 2 1", nullptr);
    processor.getAPVTS().state.appendChild (tree, nullptr);

    DashboardLayout::Sizes expected;
    expected.oneRow = { 4.0, 3.0, 2.0, 1.0 };
    CHECK (processor.getPanelSizes() == expected);

    // As the web UI sends them
    CHECK (PanelSizesState::from (PanelSizesState::toVar (expected)) == expected);
}

TEST_CASE ("Panel sizes edited by hand read as the web UI reads them", "[state]")
{
    juce::ValueTree tree (PanelSizesState::type);
    tree.setProperty (PanelSizesState::rowsProp, " 300  131.5 226 ", nullptr);
    tree.setProperty (PanelSizesState::lowerProp, "40abc 60", nullptr);

    DashboardLayout::Sizes expected;
    expected.rows = { 300.0, 131.5, 226.0 };
    CHECK (PanelSizesState::from (tree) == expected);
}

TEST_CASE_METHOD (ProcessorFixture, "The editor's size stays within its limits, and a preset doesn't change it", "[state]")
{
    processor.setEditorSize (10, 99999);
    CHECK (processor.getEditorSize() == juce::Point<int> (EditorSize::minWidth, EditorSize::maxHeight));

    processor.setEditorSize (1000, 640);
    auto& presets = processor.getPresetManager();
    REQUIRE (presets.loadPreset (presets.find ("Ember Pulse", true).value()));
    CHECK (processor.getEditorSize() == juce::Point<int> (1000, 640));
}

TEST_CASE_METHOD (ProcessorFixture, "Invalid state is ignored", "[state]")
{
    setParameter (processor, Parameters::themeId, 2.0f);

    SECTION ("Random bytes")
    {
        const char garbage[] = "definitely not plugin state";
        processor.setStateInformation (garbage, static_cast<int> (sizeof (garbage)));
    }

    SECTION ("Empty data")
    {
        processor.setStateInformation (nullptr, 0);
    }

    SECTION ("Valid XML with the wrong root tag")
    {
        juce::MemoryBlock state;
        juce::AudioProcessor::copyXmlToBinary (juce::XmlElement ("SomethingElse"), state);
        processor.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    }

    CHECK_THAT (getParameter (processor, Parameters::themeId), WithinAbs (2.0, 1.0e-4));
}
