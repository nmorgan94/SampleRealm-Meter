#include "MeterWebView.h"

#if PLUGIN_WEBVIEW_EDITOR

#include <webview_editor/webview_editor.h>
#include "../shared/MeterInfoJson.h"
#include "../ui/meter/MeterLook.h"

namespace
{
    constexpr int framesPerSecond = 60;
}

juce::AudioProcessorEditor* createMeterWebView (AudioPluginAudioProcessor& processor)
{
    // Drop anything played while no editor was showing
    processor.getMeterEngine().discardPending();

    // Read first: the editor stores its size as it opens
    const auto size = processor.getEditorSize();

    // The custom theme and panel sizes the page has, so only a change made elsewhere, such as a look loading, is sent to it
    const auto customThemeSent = std::make_shared<Themes::Custom> (processor.getCustomTheme());
    const auto panelSizesSent = std::make_shared<DashboardLayout::Sizes> (processor.getPanelSizes());

    return new WebViewEditor (processor, {
        .specs = ParameterSpecs::all,
        .parameters = processor.getAPVTS(),
        .presets = &processor.getPresetManager(),
        .width = size.x,
        .height = size.y,
        // The first theme's, until the page draws its own
        .background = MeterTheme::of (0).background,
        .onTick = [&processor, customThemeSent, panelSizesSent,
                   lastFrameTime = juce::Time::getMillisecondCounterHiRes(),
                   lastFrameSent = MeterFrame::Data {},
                   framesUnsent = 0] (const WebViewEditor::EmitEvent& emit) mutable
        {
            const auto now = juce::Time::getMillisecondCounterHiRes();
            const auto seconds = (now - std::exchange (lastFrameTime, now)) / 1000.0;

            // The page was opening or hidden: what played meanwhile isn't shown
            if (seconds > MeterEngine::maxFrameSeconds)
                processor.getMeterEngine().discardPending();

            if (const auto custom = processor.getCustomTheme(); custom != *customThemeSent)
            {
                *customThemeSent = custom;
                emit ("customTheme", CustomThemeState::toVar (custom));
            }

            if (const auto sizes = processor.getPanelSizes(); sizes != *panelSizesSent)
            {
                *panelSizesSent = sizes;
                emit ("panelSizes", PanelSizesState::toVar (sizes));
            }

            const auto& frame = processor.getMeterEngine().updateFrame (seconds);

            // A stopped or silent meter soon shows the same frame every time. It still goes twice a second,
            // for a page that has only just loaded
            if (frame != lastFrameSent || ++framesUnsent >= framesPerSecond / 2)
            {
                lastFrameSent = frame;
                framesUnsent = 0;

                // As its bytes rather than as hundreds of numbers in JSON
                emit ("frame", juce::Base64::toBase64 (frame.data(), frame.size() * sizeof (float)));
            }
        },
        .withOptions = [&processor, customThemeSent, panelSizesSent] (juce::WebBrowserComponent::Options options)
        {
            return options
                .withInitialisationData ("meterInfo", juce::String (meterInfoJson()))
                .withInitialisationData ("customTheme", CustomThemeState::toVar (*customThemeSent))
                .withInitialisationData ("panelSizes", PanelSizesState::toVar (*panelSizesSent))
                .withNativeFunction ("setPanelSizes", [&processor, panelSizesSent] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    if (! args.isEmpty())
                    {
                        *panelSizesSent = PanelSizesState::from (args.getReference (0));
                        processor.setPanelSizes (*panelSizesSent);
                    }

                    complete ({});
                })
                .withNativeFunction ("setCustomTheme", [&processor, customThemeSent] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    if (! args.isEmpty())
                    {
                        *customThemeSent = CustomThemeState::from (args.getReference (0));
                        processor.setCustomTheme (*customThemeSent);
                    }

                    complete ({});
                })
                .withNativeFunction ("meterReset", [&processor] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    processor.getMeterEngine().requestReset();
                    complete ({});
                });
        },
        .tickHz = framesPerSecond,
        .resizeLimits = WebViewEditor::ResizeLimits { EditorSize::minWidth, EditorSize::minHeight, EditorSize::maxWidth, EditorSize::maxHeight },
        .onResized = [&processor] (int width, int height) { processor.setEditorSize (width, height); } });
}

#endif
