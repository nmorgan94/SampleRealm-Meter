// The shared worklet backend, with the meter's info, frames, reset and custom theme
import { xmlTags } from '@samplerealm/modules/demo/localPresets.ts';
import { workletBackend, type WorkletBackend } from '@samplerealm/modules/demo/workletBackend.ts';
import { listenerSet, type UiInfo } from '@samplerealm/modules/backend.ts';
import type { CustomTheme, MeterBackend, MeterInfo } from '../backend.ts';
import { customThemeStore, sanitiseCustomTheme } from '../meter/theme.ts';

// This plugin's, as a site's demos share its storage
const STORAGE_KEY = 'samplerealm-meter-presets';

// What wasm/Engine.cpp exports beside the shared ones
export const FRAMES = { name: 'frame', size: 'frameSize', perSecond: 60 };

// What a look holds beside the parameters: CustomThemeState in the plugin
interface Extra {
  customTheme?: Partial<Record<keyof CustomTheme, unknown>>;
}

export type MeterWorkletBackend = WorkletBackend &
  MeterBackend & {
    // Called on each reset, which the engine only makes while it's running
    onReset(listener: () => void): () => void;
  };

export function meterWorkletBackend(info: UiInfo, meter: MeterInfo, factoryPresets: string[] = []): MeterWorkletBackend {
  const custom = customThemeStore(meter.customTheme);

  const backend = workletBackend<Extra>(info, {
    storageKey: STORAGE_KEY,
    factoryPresets,
    extraState: {
      // The plugin saves it as a <CustomTheme> beside the parameters
      parse: (xml) => ({ customTheme: xmlTags(xml, 'CustomTheme')[0] }),
      capture: () => ({ customTheme: custom.customTheme() }),
      // A look without one, and Init, have the default
      apply: ({ customTheme }) => custom.show(sanitiseCustomTheme(customTheme, meter.customTheme, meter.themes.length)),
    },
  });

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
    customTheme: custom.customTheme,
    onCustomTheme: custom.onCustomTheme,
    setCustomTheme: (theme) => {
      const now = custom.customTheme();
      if (theme.base === now.base && theme.primary === now.primary && theme.secondary === now.secondary && theme.hot === now.hot) return;
      custom.show(theme);
      backend.markModified();
    },
  };
}
