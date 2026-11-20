// PresetManager's own tests are in SampleRealm-Modules. These are this plugin's presets and state
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

namespace
{
    // The same order as webui/test/demo/factoryPresets.test.ts
    const juce::StringArray factoryNames { "Ember Pulse", "Ice Subtle", "Phosphor Calm", "Ember Loudness", "Phosphor Scope", "Ultraviolet Spectrum" };
}

//==============================================================================
TEST_CASE_METHOD (ProcessorFixture, "The factory presets are embedded and set this plugin's parameters", "[presets]")
{
    auto& presets = processor.getPresetManager();

    CHECK (namesOf (presets.getFactoryPresets()) == factoryNames);

    REQUIRE (presets.loadPreset (presets.find ("Ultraviolet Spectrum", true).value()));
    CHECK_THAT (getParameter (processor, Parameters::themeId), WithinAbs (3.0, 1.0e-4));
    CHECK_THAT (getParameter (processor, Parameters::viewId), WithinAbs (1.0, 1.0e-4));
}

TEST_CASE_METHOD (ProcessorFixture, "A look saved before the panels could be hidden shows them all", "[presets]")
{
    setParameter (processor, Parameters::showWaveformId, 0.0f);

    // The factory looks don't say which panels to show
    auto& presets = processor.getPresetManager();
    REQUIRE (presets.loadPreset (presets.find ("Ember Pulse", true).value()));

    CHECK_THAT (getParameter (processor, Parameters::showWaveformId), WithinAbs (1.0, 1.0e-4));
}

TEST_CASE_METHOD (ProcessorFixture, "Changing the custom theme is an edit to the look, and Init clears it", "[presets]")
{
    auto& presets = processor.getPresetManager();
    REQUIRE (presets.loadPreset (presets.find ("Ember Pulse", true).value()));
    CHECK_FALSE (presets.isModified());

    processor.setCustomTheme ({ 1, 0x112233, 0x445566, 0x778899 });
    CHECK (presets.isModified());

    presets.loadInit();
    CHECK (processor.getCustomTheme() == Themes::Custom {});
}

TEST_CASE_METHOD (ProcessorFixture, "A reopened session shows the same preset", "[presets][state]")
{
    auto& presets = processor.getPresetManager();
    REQUIRE (presets.loadPreset (presets.find ("Ice Subtle", true).value()));
    setParameter (processor, Parameters::themeId, 2.0f);

    juce::MemoryBlock state;
    processor.getStateInformation (state);

    AudioPluginAudioProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    const auto current = restored.getPresetManager().getCurrent();
    CHECK (current.name == "Ice Subtle");
    CHECK (current.category == "Dashboard");
    CHECK (current.isFactory);
    CHECK (restored.getPresetManager().isModified());
    CHECK_THAT (getParameter (restored, Parameters::themeId), WithinAbs (2.0, 1.0e-4));

    // Kept by the preset manager, not left on the parameters
    CHECK_FALSE (restored.getAPVTS().state.hasProperty ("presetName"));
}
