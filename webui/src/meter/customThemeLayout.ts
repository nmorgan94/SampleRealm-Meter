// Mirrors CustomThemeLayout in Source/ui/meter/CustomThemeEditor.h, with the same tests
import type { Rect } from './rect.ts';

// Short enough for the editor at its shortest
export const PANEL_WIDTH = 560;
export const PANEL_HEIGHT = 188;

// A swatch is its colour over its name
export const SWATCH_NAME_HEIGHT = 12;
export const SWATCH_GAP = 4;

export interface CustomThemeLayout {
  title: Rect;
  close: Rect;
  baseLabel: Rect;
  base: Rect;
  coloursLabel: Rect;
  swatches: Rect[];
  hex: Rect;
  // The picker: saturation across and brightness up, over the hues
  square: Rect;
  hue: Rect;
}

export function customThemeLayout(panel: Rect): CustomThemeLayout {
  const area = { left: panel.left + 16, top: panel.top + 12, width: panel.width - 32, height: panel.height - 24 };
  const content = { top: area.top + 32, height: area.height - 32 };
  const left = { left: area.left, width: 250 };
  const right = { left: area.left + 270, width: area.width - 270 };
  const row = (top: number, height: number): Rect => ({ ...left, top: content.top + top, height });

  return {
    title: { left: area.left, top: area.top, width: area.width - 70, height: 24 },
    close: { left: area.left + area.width - 70, top: area.top, width: 70, height: 24 },
    baseLabel: row(0, 12),
    base: row(12, 24),
    coloursLabel: row(44, 12),
    swatches: [0, 1, 2].map((i) => ({ left: left.left + i * 86, top: content.top + 56, width: 78, height: 44 })),
    hex: row(108, 24),
    square: { ...right, top: content.top, height: content.height - 24 },
    hue: { ...right, top: content.top + content.height - 16, height: 16 },
  };
}
