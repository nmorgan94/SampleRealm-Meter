#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "shared/MeterScales.h"

namespace
{
    juce::RangedAudioParameter& parameter (AudioPluginAudioProcessor& processor, const juce::ParameterID& id)
    {
        auto* found = processor.getAPVTS().getParameter (id.getParamID());
        jassert (found != nullptr);
        return *found;
    }

    // To the next choice, and round to the first again
    void stepChoice (juce::ParameterAttachment& attachment, juce::RangedAudioParameter& choice)
    {
        const auto count = choice.getNumSteps();
        const auto current = juce::roundToInt (choice.convertFrom0to1 (choice.getValue()));
        attachment.setValueAsCompleteGesture (static_cast<float> ((current + 1) % count));
    }
}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      themeAttachment (parameter (p, Parameters::themeId), [this] (float value) { themeChanged (juce::roundToInt (value)); }),
      reactivityAttachment (parameter (p, Parameters::reactivityId), [this] (float value)
      {
          const auto index = static_cast<size_t> (juce::jlimit (0, static_cast<int> (MeterScales::reactivityAmounts.size()) - 1, juce::roundToInt (value)));
          look.reactivity = MeterScales::reactivityAmounts[index];
          controls.setReactivityName (ParameterSpecs::reactivityNames[index]);
      }),
      viewAttachment (parameter (p, Parameters::viewId), [this] (float value)
      {
          // The first time, the editor opens already in the view
          dashboard.setView (juce::roundToInt (value), std::exchange (viewShown, true));
      }),
      vblank (this, [this] (double now)
      {
          // The first frame comes no time after anything
          const auto seconds = lastFrameTime ? now - *lastFrameTime : 0.0;

          if (lastFrameTime && seconds < minFrameSeconds)
              return;

          lastFrameTime = now;
          showNextFrame (seconds);
      })
{
    setLookAndFeel (&customLookAndFeel);

    addAndMakeVisible (dashboard);
    addAndMakeVisible (header);
    addChildComponent (exitCaptureButton);

    controls.themeButton.onClick = [this] { stepChoice (themeAttachment, parameter (processorRef, Parameters::themeId)); };
    controls.reactivityButton.onClick = [this] { stepChoice (reactivityAttachment, parameter (processorRef, Parameters::reactivityId)); };
    controls.resetButton.onClick = [this] { processorRef.getMeterEngine().requestReset(); };
    controls.captureButton.onClick = [this] { setCaptureMode (true); };
    exitCaptureButton.onClick = [this] { setCaptureMode (false); };

    dashboard.onViewChosen = [this] (int view) { viewAttachment.setValueAsCompleteGesture (static_cast<float> (view)); };

    themeAttachment.sendInitialUpdate();
    reactivityAttachment.sendInitialUpdate();
    viewAttachment.sendInitialUpdate();

    addChildComponent (prompt);
    addChildComponent (manual);

    // Anywhere in the editor, to bring back the way out of capture mode
    addMouseListener (this, true);

    // Drop anything played while no editor was showing
    processorRef.getMeterEngine().discardPending();

    // Read first: setting the limits resizes the editor, which stores its size
    const auto size = processorRef.getEditorSize();

    setResizable (true, true);
    setResizeLimits (EditorSize::minWidth, EditorSize::minHeight, EditorSize::maxWidth, EditorSize::maxHeight);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (size.x, size.y);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    removeMouseListener (this);
    setLookAndFeel (nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::themeChanged (int theme)
{
    const auto index = juce::jlimit (0, static_cast<int> (ParameterSpecs::themeNames.size()) - 1, theme);

    look.theme = MeterTheme::of (index);
    controls.setThemeName (ParameterSpecs::themeNames[static_cast<size_t> (index)]);
    customLookAndFeel.setTheme (look.theme);
    sendLookAndFeelChange();
    repaint();
}

void AudioPluginAudioProcessorEditor::setCaptureMode (bool shouldCapture)
{
    if (look.capture == shouldCapture)
        return;

    look.capture = shouldCapture;
    header.setVisible (! shouldCapture);
    exitCaptureButton.setVisible (false);
    stopTimer();

    resized();
    repaint();
}

void AudioPluginAudioProcessorEditor::showNextFrame (double seconds)
{
    dashboard.setFrame (processorRef.getMeterEngine().updateFrame (seconds), juce::jlimit (0.0, MeterEngine::maxFrameSeconds, seconds));
}

void AudioPluginAudioProcessorEditor::mouseMove (const juce::MouseEvent&)
{
    if (! look.capture)
        return;

    exitCaptureButton.setVisible (true);
    exitCaptureButton.toFront (false);
    startTimer (exitCaptureMs);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    // While the mouse is over the button, it stays
    if (exitCaptureButton.isMouseOver())
        return;

    stopTimer();
    exitCaptureButton.setVisible (false);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (look.theme.background);

    if (look.capture)
    {
        g.setColour (look.theme.dim);
        g.setFont (CustomLookAndFeel::orbitron (CustomLookAndFeel::boldWeight).withHeight (11.0f));
        g.drawText ("SAMPLEREALM METER", getLocalBounds().removeFromBottom (wordmarkHeight).withTrimmedRight (DashboardLayout::padding + 2),
                    juce::Justification::centredRight);
    }
}

void AudioPluginAudioProcessorEditor::resized()
{
    processorRef.setEditorSize (getWidth(), getHeight());

    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();

    exitCaptureButton.setBounds (bounds.reduced (DashboardLayout::padding + 8).removeFromTop (24).removeFromRight (112));

    if (look.capture)
        bounds.removeFromBottom (wordmarkHeight - DashboardLayout::padding);
    else
        header.setBounds (bounds.removeFromTop (headerLayout.height));

    dashboard.setBounds (bounds);
}
