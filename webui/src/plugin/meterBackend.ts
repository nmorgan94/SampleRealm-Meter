// The C++ side of these names is Source/webview/MeterWebView.cpp
import { getNativeFunction } from '@juce-framework/webview';
import { initialData, onEvent, pluginBackend } from '@samplerealm/modules/plugin/pluginBackend.ts';
import type { CustomTheme, MeterBackend, MeterInfo, PanelSizes } from '../backend.ts';
import { decodeFrame } from '../meter/frame.ts';
import { valueStore } from '../meter/store.ts';

export function meterBackend(): MeterBackend {
  const custom = valueStore(initialData<CustomTheme>('customTheme'));
  const sizes = valueStore(initialData<PanelSizes>('panelSizes'));

  // A look or a session loading. The page's own edits aren't sent back
  onEvent<CustomTheme>('customTheme', custom.show);
  onEvent<PanelSizes>('panelSizes', sizes.show);

  return {
    ...pluginBackend(),
    meter: JSON.parse(initialData<string>('meterInfo')) as MeterInfo,
    onFrame: (listener) => onEvent<string>('frame', (frame) => listener(decodeFrame(frame))),
    resetMeters: () => void getNativeFunction('meterReset')(),
    customTheme: custom.get,
    onCustomTheme: custom.listen,
    setCustomTheme: (theme) => {
      custom.show(theme);
      void getNativeFunction('setCustomTheme')(theme);
    },
    panelSizes: sizes.get,
    onPanelSizes: sizes.listen,
    setPanelSizes: (next) => {
      sizes.show(next);
      void getNativeFunction('setPanelSizes')(next);
    },
  };
}
