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

    struct PanelControl
    {
        DashboardLayout::Panel panel;
        const char* name;
        const juce::ParameterID& shownId;
    };

    // In the order the dashboard lays them out, which is the view menu's
    const std::array<PanelControl, DashboardLayout::numPanels> panelControls { {
        { DashboardLayout::spectrum, "Spectrum", Parameters::showSpectrumId },
        { DashboardLayout::waveform, "Waveform", Parameters::showWaveformId },
        { DashboardLayout::stereo, "Stereo field", Parameters::showStereoId },
        { DashboardLayout::loudness, "Loudness", Parameters::showLoudnessId },
    } };
}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      themeAttachment (parameter (p, Parameters::themeId), [this] (float value) { themeChanged (juce::roundToInt (value)); }),
      reactivityAttachment (parameter (p, Parameters::reactivityId), [this] (float value)
      {
          const auto index = static_cast<size_t> (juce::jlimit (0, static_cast<int> (MeterScales::reactivityAmounts.size()) - 1, juce::roundToInt (value)));
          look.reactivity = MeterScales::reactivityAmounts[index];
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

    controls.themeButton.onClick = [this] { themeMenu().showMenuAsync (juce::PopupMenu::Options().withTargetComponent (controls.themeButton)); };
    controls.viewButton.onClick = [this] { viewMenu().showMenuAsync (juce::PopupMenu::Options().withTargetComponent (controls.viewButton)); };
    controls.resetButton.onClick = [this] { processorRef.getMeterEngine().requestReset(); };
    controls.captureButton.onClick = [this] { setCaptureMode (true); };
    exitCaptureButton.onClick = [this] { setCaptureMode (false); };

    dashboard.onViewChosen = [this] (int view) { viewAttachment.setValueAsCompleteGesture (static_cast<float> (view)); };

    dashboard.setSizes (processorRef.getPanelSizes());
    dashboard.onSizesChosen = [this] (const DashboardLayout::Sizes& sizes)
    {
        processorRef.setPanelSizes (sizes);
        dashboard.setSizes (sizes);
    };

    themeAttachment.sendInitialUpdate();
    reactivityAttachment.sendInitialUpdate();
    viewAttachment.sendInitialUpdate();

    for (const auto& control : panelControls)
    {
        auto& attachment = shownAttachments[control.panel];

        attachment = std::make_unique<juce::ParameterAttachment> (parameter (p, control.shownId), [this, panel = control.panel] (float value)
        {
            shown[panel] = value >= 0.5f;
            dashboard.setShown (shown);
        });

        attachment->sendInitialUpdate();
    }

    addChildComponent (prompt);
    addChildComponent (manual);
    addChildComponent (customTheme);

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

    customThemeShown = processorRef.getCustomTheme();
    look.theme = MeterTheme::of (Themes::paletteOf (index, customThemeShown));
    controls.setThemeName (ParameterSpecs::themeNames[static_cast<size_t> (index)]);
    customLookAndFeel.setTheme (look.theme);
    sendLookAndFeelChange();
    repaint();
}

void AudioPluginAudioProcessorEditor::customThemeChanged()
{
    auto& theme = parameter (processorRef, Parameters::themeId);
    themeChanged (juce::roundToInt (theme.convertFrom0to1 (theme.getValue())));
}

void AudioPluginAudioProcessorEditor::addChoices (juce::PopupMenu& menu, const juce::ParameterID& id, juce::ParameterAttachment& attachment)
{
    auto& choice = parameter (processorRef, id);
    const auto current = juce::roundToInt (choice.convertFrom0to1 (choice.getValue()));
    auto index = 0;

    for (const auto& name : choice.getAllValueStrings())
    {
        menu.addItem (juce::PopupMenu::Item (name)
                          .setTicked (index == current)
                          .setAction ([safeThis = juce::Component::SafePointer (this), &attachment, index]
                          {
                              if (safeThis != nullptr)
                                  attachment.setValueAsCompleteGesture (static_cast<float> (index));
                          }));
        ++index;
    }
}

juce::PopupMenu AudioPluginAudioProcessorEditor::themeMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&getLookAndFeel());
    addChoices (menu, Parameters::themeId, themeAttachment);
    menu.addSeparator();

    menu.addItem (juce::PopupMenu::Item ("Edit custom...")
                      .setAction ([safeThis = juce::Component::SafePointer (this)]
                      {
                          if (safeThis == nullptr)
                              return;

                          safeThis->themeAttachment.setValueAsCompleteGesture (static_cast<float> (Themes::customIndex));
                          safeThis->customTheme.show();
                      }));

    return menu;
}

juce::PopupMenu AudioPluginAudioProcessorEditor::viewMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&getLookAndFeel());
    menu.addSectionHeader ("Panels");

    const auto numShown = std::count (shown.begin(), shown.end(), true);

    for (const auto& control : panelControls)
    {
        const auto isShown = shown[control.panel];

        // The last panel showing stays
        menu.addItem (juce::PopupMenu::Item (control.name)
                          .setTicked (isShown)
                          .setEnabled (! isShown || numShown > 1)
                          .setAction ([safeThis = juce::Component::SafePointer (this), panel = control.panel, isShown]
                          {
                              if (safeThis != nullptr)
                                  safeThis->shownAttachments[panel]->setValueAsCompleteGesture (isShown ? 0.0f : 1.0f);
                          }));
    }

    menu.addSectionHeader ("Reactivity");
    addChoices (menu, Parameters::reactivityId, reactivityAttachment);
    return menu;
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
    // A look or session loading. The custom theme's editor and a drag in the dashboard show their own edits
    if (processorRef.getCustomTheme() != customThemeShown)
    {
        customThemeChanged();

        if (customTheme.isVisible())
            customTheme.refresh();
    }

    dashboard.setSizes (processorRef.getPanelSizes());
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
