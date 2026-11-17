#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <editor/editor.h>
#include "Parameters.h"
#include "dsp/MeterEngine.h"
#include "shared/EditorSize.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    MeterEngine& getMeterEngine()                  { return meterEngine; }
    PresetManager& getPresetManager()              { return presetManager; }

    static PresetManager::Config createPresetConfig();

    //==============================================================================
    /** The editor's size, kept with the session but not with presets. */
    juce::Point<int> getEditorSize() const noexcept   { return { editorWidth.load(), editorHeight.load() }; }
    void setEditorSize (int width, int height) noexcept;

private:
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters",
                                               createParameterLayout (ParameterSpecs::all, Parameters::versionHint) };
    PresetManager presetManager { apvts, createPresetConfig() };

    MeterEngine meterEngine;

    std::atomic<int> editorWidth { EditorSize::defaultWidth }, editorHeight { EditorSize::defaultHeight };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
