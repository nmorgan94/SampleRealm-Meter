#pragma once

#include "PluginProcessor.h"
#include "ui/About.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/HeaderBar.h"
#include "ui/Manual.h"
#include "ui/PromptOverlay.h"
#include "ui/meter/Dashboard.h"
#include "ui/meter/MeterControls.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    // As HEADER_LAYOUT in webui/src/Editor.tsx
    static constexpr HeaderBar::Layout headerLayout { .height = 44, .edge = 12, .titleWidth = 176, .titleHeight = 16.0f,
                                                      .gap = 12, .controlsHeight = 24, .trailingWidth = MeterControls::width, .trailingInset = 10 };

    // In capture mode, the strip along the bottom with the maker's name
    static constexpr int wordmarkHeight = 22;
    static constexpr int exitCaptureMs = 2500;

    // A refresh sooner than this after the last one drawn is skipped. Just under 10 ms, so displays
    // up to 100 Hz draw every refresh, and faster ones every second or third: 60 a second at 120 Hz
    static constexpr double minFrameSeconds = 1.0 / 105.0;

    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;

    /** Clean capture mode: only the meter, with no header or labels. */
    void setCaptureMode (bool shouldCapture);
    bool isCaptureMode() const noexcept   { return look.capture; }

    /** Draws the meter as it is now, after a time. Called on every screen refresh. */
    void showNextFrame (double seconds);

    Dashboard& getDashboard()   { return dashboard; }

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    CustomLookAndFeel customLookAndFeel;
    MeterLook look;

    PromptOverlay prompt;
    ManualOverlay manual;
    MeterControls controls;
    HeaderBar header { "SampleRealm: METER", headerLayout, processorRef.getPresetManager(), prompt, [this] { manual.show(); },
                       About::text (processorRef), controls };
    Dashboard dashboard { look };
    juce::TextButton exitCaptureButton { "EXIT CAPTURE" };

    juce::ParameterAttachment themeAttachment, reactivityAttachment, viewAttachment;
    bool viewShown = false;

    std::optional<double> lastFrameTime;
    juce::VBlankAttachment vblank;

    void themeChanged (int theme);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
