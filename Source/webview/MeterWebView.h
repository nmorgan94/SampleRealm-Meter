#pragma once

#include "../PluginProcessor.h"

/** The shared WebView editor, with the meter's info, frames and reset. The page side is webui/src/plugin/meterBackend.ts. */
juce::AudioProcessorEditor* createMeterWebView (AudioPluginAudioProcessor&);
