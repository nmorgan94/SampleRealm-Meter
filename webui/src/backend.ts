import type { PluginBackend } from '@samplerealm/modules/backend.ts';

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

// Source/shared/MeterInfoJson.h
export interface MeterInfo {
  frame: FrameLayout;
  scales: Scales;
  // By the theme parameter's choices
  themes: ThemeColours[];
}

export interface MeterBackend extends PluginBackend {
  meter: MeterInfo;
  // The meter now, about 60 times a second: see meter/frame.ts
  onFrame(listener: (frame: Float32Array) => void): () => void;
  // Starts the integrated loudness, the range, the held peaks and the clip count again, as starting playback does
  resetMeters(): void;
}
