// The shared worklet backend, with the meter's info, frames, reset and custom theme
import { xmlTags } from '@samplerealm/modules/demo/localPresets.ts';
import { workletBackend, type WorkletBackend } from '@samplerealm/modules/demo/workletBackend.ts';
import { listenerSet, type UiInfo } from '@samplerealm/modules/backend.ts';
import type { CustomTheme, MeterBackend, MeterInfo, PanelSizes } from '../backend.ts';
import { defaultSizes, sameSizes, sanitiseSizes } from '../meter/dashboardLayout.ts';
import { valueStore } from '../meter/store.ts';
import { sanitiseCustomTheme } from '../meter/theme.ts';

// This plugin's, as a site's demos share its storage
const STORAGE_KEY = 'samplerealm-meter-presets';

// What wasm/Engine.cpp exports beside the shared ones
export const FRAMES = { name: 'frame', size: 'frameSize', perSecond: 60 };

// What a look holds beside the parameters: CustomThemeState and PanelSizesState in the plugin
interface Extra {
  customTheme?: Partial<Record<keyof CustomTheme, unknown>>;
  panelSizes?: Partial<Record<keyof PanelSizes, unknown>>;
}

export type MeterWorkletBackend = WorkletBackend &
  MeterBackend & {
    // Called on each reset, which the engine only makes while it's running
    onReset(listener: () => void): () => void;
  };

export function meterWorkletBackend(info: UiInfo, meter: MeterInfo, factoryPresets: string[] = []): MeterWorkletBackend {
  const custom = valueStore(meter.customTheme);
  const sizes = valueStore(defaultSizes());

  const backend = workletBackend<Extra>(info, {
    storageKey: STORAGE_KEY,
    factoryPresets,
    extraState: {
      // The plugin saves them as a <CustomTheme> and a <PanelSizes> beside the parameters
      parse: (xml) => ({ customTheme: xmlTags(xml, 'CustomTheme')[0], panelSizes: xmlTags(xml, 'PanelSizes')[0] }),
      capture: () => ({ customTheme: custom.get(), panelSizes: sizes.get() }),
      // A look without one, and Init, have the default
      apply: ({ customTheme, panelSizes }) => {
        custom.show(sanitiseCustomTheme(customTheme, meter.customTheme, meter.themes.length));
        sizes.show(sanitiseSizes(panelSizes));
      },
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
    customTheme: custom.get,
    onCustomTheme: custom.listen,
    setCustomTheme: (theme) => {
      const now = custom.get();
      if (theme.base === now.base && theme.primary === now.primary && theme.secondary === now.secondary && theme.hot === now.hot) return;
      custom.show(theme);
      backend.markModified();
    },
    panelSizes: sizes.get,
    onPanelSizes: sizes.listen,
    setPanelSizes: (next) => {
      if (sameSizes(next, sizes.get())) return;
      sizes.show(next);
      backend.markModified();
    },
  };
}
