// The C++ side of these names is Source/webview/MeterWebView.cpp
import { getNativeFunction } from '@juce-framework/webview';
import { initialData, onEvent, pluginBackend } from '@samplerealm/modules/plugin/pluginBackend.ts';
import type { CustomTheme, MeterBackend, MeterInfo } from '../backend.ts';
import { decodeFrame } from '../meter/frame.ts';
import { customThemeStore } from '../meter/theme.ts';

export function meterBackend(): MeterBackend {
  const custom = customThemeStore(initialData<CustomTheme>('customTheme'));

  // A look or a session loading. The page's own edits aren't sent back
  onEvent<CustomTheme>('customTheme', custom.show);

  return {
    ...pluginBackend(),
    meter: JSON.parse(initialData<string>('meterInfo')) as MeterInfo,
    onFrame: (listener) => onEvent<string>('frame', (frame) => listener(decodeFrame(frame))),
    resetMeters: () => void getNativeFunction('meterReset')(),
    customTheme: custom.customTheme,
    onCustomTheme: custom.onCustomTheme,
    setCustomTheme: (theme) => {
      custom.show(theme);
      void getNativeFunction('setCustomTheme')(theme);
    },
  };
}
