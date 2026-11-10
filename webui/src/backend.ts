type ParamKind = 'real' | 'integer' | 'choice';

export interface ParamInfo {
  id: string;
  name: string;
  kind: ParamKind;
  min: number;
  max: number;
  defaultValue: number;
  step: number;
  skew: number;
  unit: string;
  choices: string[];
}

// Source/shared/MeterFrame.h: a frame is its scalars, then the bands, their held peaks and the goniometer's x, y pairs
export interface FrameLayout {
  scalars: string[];
  numBands: number;
  numPoints: number;
  minHz: number;
  maxHz: number;
  // The bottom of each scale. A loudness at noLoudness has no reading yet
  noLoudness: number;
  peakFloorDb: number;
  bandFloorDb: number;
}

// Source/shared/MeterScales.h: where values sit on the editor's scales, and how fast it moves
export interface Scales {
  // The spectrum, in dB after its tilt
  spectrumTopDb: number;
  spectrumBottomDb: number;
  spectrumGridHz: number[];
  spectrumGridLabels: string[];
  spectrumGridDb: number[];
  // The loudness and true peak bars share one scale, in LUFS and dBTP
  levelTop: number;
  levelBottom: number;
  levelTicks: number[];
  // How far the display moves with the beat, by the reactivity parameter's choices
  reactivityAmounts: number[];
  // How long a panel takes to grow over the dashboard, and the goniometer's trace to fade to about a third
  expandSeconds: number;
  traceFadeSeconds: number;
}

// Source/shared/Themes.h: each colour as "rrggbb"
export type ThemeColours = Record<'background' | 'panel' | 'line' | 'text' | 'dim' | 'primary' | 'secondary' | 'hot', string>;

export interface UiInfo {
  version: string;
  commit: string;
  build: 'Debug' | 'Release';
  params: ParamInfo[];
  frame: FrameLayout;
  scales: Scales;
  // By the theme parameter's choices
  themes: ThemeColours[];
}

export interface PresetEntry {
  name: string;
  category: string;
  isFactory: boolean;
}

export interface PresetStatus {
  current: PresetEntry;
  modified: boolean;
}

// Mirrors Source/presets/PresetManager.h
export interface PresetBackend {
  status(): PresetStatus;
  onStatus(listener: (status: PresetStatus) => void): () => void;
  // Factory then user presets, rescanning the user ones
  list(): Promise<PresetEntry[]>;
  // The current preset if it's listed
  current(): Promise<PresetEntry | null>;
  // Resolves to whether it loaded
  load(preset: PresetEntry): Promise<boolean>;
  loadInit(): Promise<void>;
  loadNext(): Promise<void>;
  loadPrevious(): Promise<void>;
  exists(name: string): Promise<boolean>;
  // Each resolves to an error message, or null
  save(name: string, category: string): Promise<string | null>;
  rename(preset: PresetEntry, newName: string): Promise<string | null>;
  remove(preset: PresetEntry): Promise<string | null>;
  // Only where presets are files
  showFolder?(): void;
}

export const isSamePreset = (a: PresetEntry, b: PresetEntry) => a.name === b.name && a.isFactory === b.isFactory;

export interface PluginBackend {
  info: UiInfo;
  // "VST3 in Ableton Live", or where else the editor is running
  running: string;
  presets: PresetBackend;
  param(id: string): ParamInfo;
  // Real values, not 0..1
  getValue(id: string): number;
  setValue(id: string, value: number): void;
  beginGesture(id: string): void;
  endGesture(id: string): void;
  subscribe(id: string, listener: (value: number) => void): () => void;
  // The meter now, about 60 times a second: see meter/frame.ts
  onFrame(listener: (frame: Float32Array) => void): () => void;
  // Starts the integrated loudness, the range, the held peaks and the clip count again, as starting playback does
  resetMeters(): void;
}

export const paramLookup = <T>(entries: Map<string, T>) => (id: string) => {
  const found = entries.get(id);
  if (!found) throw new Error(`No parameter ${id}`);
  return found;
};

export const listenerSet = <Args extends unknown[]>() => {
  const listeners = new Set<(...args: Args) => void>();
  return {
    add: (listener: (...args: Args) => void) => {
      listeners.add(listener);
      return () => void listeners.delete(listener);
    },
    notify: (...args: Args) => listeners.forEach((listener) => listener(...args)),
  };
};
