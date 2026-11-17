// The shared worklet backend, with the meter's info, frames and reset
import { workletBackend, type WorkletBackend } from '@samplerealm/modules/demo/workletBackend.ts';
import { listenerSet, type UiInfo } from '@samplerealm/modules/backend.ts';
import type { MeterBackend, MeterInfo } from '../backend.ts';

// This plugin's, as a site's demos share its storage
const STORAGE_KEY = 'samplerealm-meter-presets';

// What wasm/Engine.cpp exports beside the shared ones
export const FRAMES = { name: 'frame', size: 'frameSize', perSecond: 60 };

export type MeterWorkletBackend = WorkletBackend &
  MeterBackend & {
    // Called on each reset, which the engine only makes while it's running
    onReset(listener: () => void): () => void;
  };

export function meterWorkletBackend(info: UiInfo, meter: MeterInfo, factoryPresets: string[] = []): MeterWorkletBackend {
  const backend = workletBackend(info, { storageKey: STORAGE_KEY, factoryPresets });
  const frameListeners = listenerSet<[Float32Array]>();
  const resetListeners = listenerSet<[]>();

  backend.onMessage((data) => {
    if (data.type === 'frame') frameListeners.notify(data.frame as Float32Array);
  });

  return {
    ...backend,
    meter,
    onFrame: frameListeners.add,
    resetMeters: () => {
      backend.post({ type: 'call', name: 'resetMeters' });
      resetListeners.notify();
    },
    onReset: resetListeners.add,
  };
}
