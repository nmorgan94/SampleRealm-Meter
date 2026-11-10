// The palettes come from Source/shared/Themes.h in the UI info, as "rrggbb"
import type { ThemeColours } from '../backend.ts';

export type Theme = ThemeColours;

// Mirrors MeterLook in Source/ui/meter/MeterLook.h
export interface Look {
  theme: Theme;
  // One of the scales' reactivityAmounts
  reactivity: number;
  // Clean capture mode: no titles or small labels
  capture: boolean;
}

export const colour = (hex: string, alpha = 1) => {
  const value = parseInt(hex, 16);
  const rgb = `${(value >> 16) & 255}, ${(value >> 8) & 255}, ${value & 255}`;
  return alpha >= 1 ? `rgb(${rgb})` : `rgba(${rgb}, ${alpha})`;
};

// The item for a choice parameter's value, kept within the list
export const choiceAt = <T>(list: T[], index: number) => list[Math.min(list.length - 1, Math.max(0, Math.round(index)))];

// The editor's CSS colours, which the header, buttons, menus and overlays use: CustomLookAndFeel::setTheme
export const themeVariables = (theme: Theme): Record<string, string> => ({
  '--color-background': colour(theme.background),
  '--color-widget': colour(theme.panel),
  '--color-line': colour(theme.line),
  '--color-fg': colour(theme.text),
  '--color-dim': colour(theme.dim),
  '--color-thumb': colour(theme.primary),
  '--color-clip': colour(theme.hot),
  '--color-title': colour(theme.primary),
});
