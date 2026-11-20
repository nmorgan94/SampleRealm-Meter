import type { PluginBackend } from '@samplerealm/modules/backend.ts';

// Source/shared/MeterFrame.h: a frame is its scalars, then the bands, their held peaks, the goniometer's x, y pairs
// and the waveform's columns
export interface FrameLayout {
  scalars: string[];
  numBands: number;
  numPoints: number;
  // The waveform: waveformBands peaks for each column, from 0 to about 1, the oldest column first
  numColumns: number;
  waveformBands: number;
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

// Themes::Custom: the user's own theme, which is one of the built-in ones with its three accents changed
export interface CustomTheme {
  // The built-in theme's place in the list
  base: number;
  primary: string;
  secondary: string;
  hot: string;
}

// Source/shared/MeterInfoJson.h
export interface MeterInfo {
  frame: FrameLayout;
  scales: Scales;
  // The built-in themes, by the theme parameter's choices. The choice after them is the custom theme
  themes: ThemeColours[];
  // The custom theme before the user has changed it
  customTheme: CustomTheme;
}

export interface MeterBackend extends PluginBackend {
  meter: MeterInfo;
  // The custom theme, which sessions and looks save beside the parameters
  customTheme(): CustomTheme;
  onCustomTheme(listener: (theme: CustomTheme) => void): () => void;
  setCustomTheme(theme: CustomTheme): void;
  // The meter now, about 60 times a second: see meter/frame.ts
  onFrame(listener: (frame: Float32Array) => void): () => void;
  // Starts the integrated loudness, the range, the held peaks and the clip count again, as starting playback does
  resetMeters(): void;
}
