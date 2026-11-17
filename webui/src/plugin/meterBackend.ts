// The C++ side of these names is Source/webview/MeterWebView.cpp
import { getNativeFunction } from '@juce-framework/webview';
import { initialData, onEvent, pluginBackend } from '@samplerealm/modules/plugin/pluginBackend.ts';
import type { MeterBackend, MeterInfo } from '../backend.ts';
import { decodeFrame } from '../meter/frame.ts';

export function meterBackend(): MeterBackend {
  return {
    ...pluginBackend(),
    meter: JSON.parse(initialData<string>('meterInfo')) as MeterInfo,
    onFrame: (listener) => onEvent<string>('frame', (frame) => listener(decodeFrame(frame))),
    resetMeters: () => void getNativeFunction('meterReset')(),
  };
}
