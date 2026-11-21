// The palettes come from Source/shared/Themes.h in the UI info, as "rrggbb"
import type { CustomTheme, ThemeColours } from '../backend.ts';

export type Theme = ThemeColours;

// Themes::fromHex: six hex digits in either case, with or without a "#" before them
export const parseHex = (text: string): string | null => /^#?([0-9a-fA-F]{6})$/.exec(text)?.[1].toLowerCase() ?? null;

// Themes::paletteOf: the theme for a value of the theme parameter, which after the built-in ones is the custom one
export const themeOf = (themes: Theme[], index: number, custom: CustomTheme): Theme =>
  Math.round(index) < themes.length ? choiceAt(themes, index) : { ...choiceAt(themes, custom.base), primary: custom.primary, secondary: custom.secondary, hot: custom.hot };

// CustomThemeState::from: anything missing or that isn't a colour keeps its default
export function sanitiseCustomTheme(value: Partial<Record<keyof CustomTheme, unknown>> | undefined, defaults: CustomTheme, numThemes: number): CustomTheme {
  const base = Number(value?.base);
  const colour = (name: 'primary' | 'secondary' | 'hot') => (typeof value?.[name] === 'string' ? parseHex(value[name]) : null) ?? defaults[name];

  return {
    base: value?.base === undefined || !Number.isFinite(base) ? defaults.base : Math.min(numThemes - 1, Math.max(0, Math.trunc(base))),
    primary: colour('primary'),
    secondary: colour('secondary'),
    hot: colour('hot'),
  };
}

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
