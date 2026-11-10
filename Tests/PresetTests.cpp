#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "TestHelpers.h"
#include "ui/PresetActions.h"

using namespace TestHelpers;
using Catch::Matchers::WithinAbs;

namespace
{
    // The plugin's factory presets, with user presets in a folder of their own
    struct PresetFixture : ProcessorFixture
    {
        ~PresetFixture()   { userDirectory.deleteRecursively(); }

        juce::File userDirectory = juce::File::createTempFile ("presets");

        PresetManager presets { processor.getAPVTS(), [this]
        {
            auto config = AudioPluginAudioProcessor::createPresetConfig();
            config.userDirectory = userDirectory;
            return config;
        }() };

        float theme()   { return getParameter (processor, Parameters::themeId); }
    };

    juce::StringArray namesOf (const juce::Array<PresetManager::Preset>& list)
    {
        juce::StringArray names;

        for (const auto& preset : list)
            names.add (preset.name);

        return names;
    }

    // The same order as webui/test/demo/presetState.test.ts
    const juce::StringArray factoryNames { "Ember Pulse", "Ice Subtle", "Phosphor Calm", "Ember Loudness", "Phosphor Scope", "Ultraviolet Spectrum" };
}

//==============================================================================
TEST_CASE_METHOD (PresetFixture, "Factory presets are listed by category, then name", "[presets]")
{
    CHECK (namesOf (presets.getFactoryPresets()) == factoryNames);
    CHECK (namesOf (presets.getPresets()) == factoryNames);

    for (const auto& preset : presets.getFactoryPresets())
    {
        CAPTURE (preset.name);
        CHECK (preset.isFactory);
        CHECK ((preset.category == "Dashboard" || preset.category == "Focus"));
    }
}

TEST_CASE_METHOD (PresetFixture, "A loaded preset sets its values and shows its name", "[presets]")
{
    CHECK (presets.getCurrent().name == "Init");

    REQUIRE (presets.loadPreset (presets.find ("Ultraviolet Spectrum", true).value()));

    CHECK_THAT (theme(), WithinAbs (3.0, 1.0e-4));
    CHECK_THAT (getParameter (processor, Parameters::viewId), WithinAbs (1.0, 1.0e-4));
    CHECK (presets.getCurrent().name == "Ultraviolet Spectrum");
    CHECK (presets.getCurrent().category == "Focus");
    CHECK (presets.getCurrent().isFactory);
    CHECK_FALSE (presets.isModified());
}

TEST_CASE_METHOD (PresetFixture, "Moving a parameter marks the preset modified until the next load", "[presets]")
{
    REQUIRE (presets.loadPreset (presets.find ("Ember Pulse", true).value()));

    setParameter (processor, Parameters::themeId, 3.0f);
    CHECK (presets.isModified());

    REQUIRE (presets.loadPreset (presets.find ("Ember Pulse", true).value()));
    CHECK_FALSE (presets.isModified());
    CHECK_THAT (theme(), WithinAbs (1.0, 1.0e-4));
}

TEST_CASE_METHOD (PresetFixture, "Changing state outside the parameters marks the preset modified", "[presets]")
{
    REQUIRE (presets.loadPreset (presets.find ("Ember Pulse", true).value()));

    processor.updateHostDisplay (juce::AudioProcessor::ChangeDetails{}.withProgramChanged (true));
    CHECK_FALSE (presets.isModified());

    processor.updateHostDisplay (juce::AudioProcessor::ChangeDetails{}.withNonParameterStateChanged (true));
    CHECK (presets.isModified());
}

TEST_CASE_METHOD (PresetFixture, "Init restores the defaults", "[presets]")
{
    REQUIRE (presets.loadPreset (presets.find ("Ice Subtle", true).value()));
    setParameter (processor, Parameters::themeId, 2.0f);

    presets.loadInit();

    CHECK_THAT (theme(), WithinAbs (ParameterSpecs::theme.defaultValue, 1.0e-4));
    CHECK (presets.getCurrent().name == "Init");
    CHECK_FALSE (presets.isModified());
}

TEST_CASE_METHOD (PresetFixture, "Next and previous wrap, and start from either end after Init", "[presets]")
{
    REQUIRE (presets.loadNext());
    CHECK (presets.getCurrent().name == factoryNames[0]);

    REQUIRE (presets.loadPrevious());
    CHECK (presets.getCurrent().name == factoryNames[factoryNames.size() - 1]);

    REQUIRE (presets.loadNext());
    CHECK (presets.getCurrent().name == factoryNames[0]);

    presets.loadInit();
    REQUIRE (presets.loadPrevious());
    CHECK (presets.getCurrent().name == factoryNames[factoryNames.size() - 1]);
    CHECK_THAT (theme(), WithinAbs (3.0, 1.0e-4));
}

TEST_CASE_METHOD (PresetFixture, "User presets save, rename and delete in their folder", "[presets]")
{
    setParameter (processor, Parameters::themeId, 2.0f);

    REQUIRE (presets.saveUserPreset ("Mine", "Focus").wasOk());
    const auto mine = userDirectory.getChildFile ("Mine.srmeter");
    CHECK (mine.existsAsFile());
    CHECK (presets.getCurrent().name == "Mine");
    CHECK_FALSE (presets.getCurrent().isFactory);
    CHECK_FALSE (presets.isModified());

    // Listed after the factory presets
    REQUIRE (presets.getPresets().getLast().file == mine);

    presets.loadInit();
    REQUIRE (presets.loadPreset (presets.getPresets().getLast()));
    CHECK_THAT (theme(), WithinAbs (2.0, 1.0e-4));
    CHECK (presets.getCurrent().name == "Mine");

    REQUIRE (presets.renameUserPreset (presets.getPresets().getLast(), "Yours").wasOk());
    const auto yours = userDirectory.getChildFile ("Yours.srmeter");
    CHECK_FALSE (mine.exists());
    CHECK (yours.existsAsFile());
    CHECK (presets.getCurrent().name == "Yours");
    CHECK (namesOf (presets.getUserPresets()) == juce::StringArray { "Yours" });

    REQUIRE (presets.deleteUserPreset (presets.getPresets().getLast()).wasOk());
    CHECK_FALSE (yours.exists());
    CHECK (presets.getUserPresets().isEmpty());
    CHECK (presets.isModified());
}

TEST_CASE_METHOD (PresetFixture, "User presets can't take a factory name", "[presets]")
{
    CHECK (presets.saveUserPreset ("ember pulse", {}).failed());
    CHECK (presets.saveUserPreset ("   ", {}).failed());
    CHECK_FALSE (userDirectory.getChildFile ("ember pulse.srmeter").exists());

    REQUIRE (presets.saveUserPreset ("Mine", {}).wasOk());
    CHECK (presets.renameUserPreset (presets.getUserPresets().getFirst(), "Phosphor Calm").failed());
    CHECK (userDirectory.getChildFile ("Mine.srmeter").existsAsFile());
}

TEST_CASE_METHOD (PresetFixture, "Next and previous skip a user preset deleted outside the plugin", "[presets]")
{
    REQUIRE (presets.saveUserPreset ("A", {}).wasOk());
    REQUIRE (presets.saveUserPreset ("B", {}).wasOk());
    REQUIRE (presets.loadPreset (presets.find ("A", false).value()));

    REQUIRE (userDirectory.getChildFile ("B.srmeter").deleteFile());

    REQUIRE (presets.loadNext());
    CHECK (presets.getCurrent().name == factoryNames[0]);
}

TEST_CASE_METHOD (PresetFixture, "Next and previous step past a preset that won't load", "[presets]")
{
    REQUIRE (presets.saveUserPreset ("A", {}).wasOk());
    REQUIRE (userDirectory.getChildFile ("Broken.srmeter").replaceWithText ("not a preset"));

    REQUIRE (presets.loadNext());
    CHECK (presets.getCurrent().name == factoryNames[0]);

    REQUIRE (presets.loadPrevious());
    CHECK (presets.getCurrent().name == "A");
}

TEST_CASE_METHOD (PresetFixture, "Choosing a preset that won't load says so, and keeps the current one", "[presets][ui]")
{
    OverlayFixture overlay;
    PromptOverlay prompt;
    overlay.parent.addChildComponent (prompt);
    PresetActions actions { presets, prompt };

    REQUIRE (presets.saveUserPreset ("A", {}).wasOk());
    REQUIRE (userDirectory.getChildFile ("Broken.srmeter").replaceWithText ("not a preset"));
    presets.refreshUserPresets();

    actions.load (presets.find ("A", false).value());
    CHECK_FALSE (prompt.isVisible());

    actions.load (presets.find ("Broken", false).value());
    CHECK (prompt.isVisible());
    CHECK (presets.getCurrent().name == "A");
}

TEST_CASE_METHOD (PresetFixture, "Rescanning moves the change counter only when the folder changed", "[presets]")
{
    presets.refreshUserPresets();
    const auto before = presets.getChangeCounter();

    presets.refreshUserPresets();
    CHECK (presets.getChangeCounter() == before);

    REQUIRE (userDirectory.createDirectory().wasOk());
    REQUIRE (userDirectory.getChildFile ("Mine.srmeter").replaceWithText ("<PRESET/>"));
    presets.refreshUserPresets();
    CHECK (presets.getChangeCounter() != before);
}

TEST_CASE_METHOD (PresetFixture, "Presets are found by name and kind", "[presets]")
{
    REQUIRE (presets.saveUserPreset ("Mine", {}).wasOk());

    CHECK (presets.find ("Phosphor Calm", true).has_value());
    CHECK (presets.find ("Mine", false)->file == userDirectory.getChildFile ("Mine.srmeter"));
    CHECK_FALSE (presets.find ("Phosphor Calm", false).has_value());
    CHECK_FALSE (presets.find ("Mine", true).has_value());
}

TEST_CASE_METHOD (PresetFixture, "State from before presets existed shows Init, modified", "[presets][state]")
{
    REQUIRE (presets.loadPreset (presets.find ("Ice Subtle", true).value()));

    juce::ValueTree state (processor.getAPVTS().state.getType());
    state.appendChild ({ "PARAM", { { "id", Parameters::themeId.getParamID() }, { "value", 2.0f } } }, nullptr);
    presets.restoreState (state);

    CHECK_THAT (theme(), WithinAbs (2.0, 1.0e-4));
    CHECK (presets.getCurrent().name == "Init");
    CHECK (presets.isModified());
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
