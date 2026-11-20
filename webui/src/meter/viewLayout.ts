// Mirrors the layOut functions of Source/ui/meter's views and MeterPanel's bounds, with the same tests
import { bottom, EMPTY, right, type Rect } from './rect.ts';

// MeterPanel
export const PANEL_INSET = 12;
export const PANEL_TITLE_HEIGHT = 22;
export const PANEL_CORNER = 8;
export const EXPAND_BUTTON_SIZE = 22;
export const EXPAND_BUTTON_INSET = 5;

// Inside a panel's box, under its title
export const contentBounds = (panel: Rect): Rect => {
  const trim = PANEL_TITLE_HEIGHT - PANEL_INSET / 2;
  return { left: panel.left + PANEL_INSET, top: panel.top + PANEL_INSET + trim, width: panel.width - 2 * PANEL_INSET, height: panel.height - 2 * PANEL_INSET - trim };
};

export const expandButtonBounds = (panel: Rect): Rect => ({
  left: right(panel) - EXPAND_BUTTON_INSET - EXPAND_BUTTON_SIZE,
  top: panel.top + EXPAND_BUTTON_INSET,
  width: EXPAND_BUTTON_SIZE,
  height: EXPAND_BUTTON_SIZE,
});

//==============================================================================
// SpectrumView
export const SPECTRUM_LABEL_HEIGHT = 16;
export const SPECTRUM_LABEL_SPACING = 30;

export const spectrumPlot = (area: Rect): Rect => ({ ...area, height: area.height - SPECTRUM_LABEL_HEIGHT });

//==============================================================================
// WaveformView::columnX: where a column is across the plot, the newest at its right edge. The phase, how much of
// the next column has been heard, moves them all left by up to a column
export const waveformColumnX = (plot: Rect, column: number, phase: number, numColumns: number) => plot.left + (plot.width * (column + 1 - phase)) / numColumns;

//==============================================================================
// StereoFieldView
export const BAR_HEIGHT = 26;
export const BARS_GAP = 6;
export const SCOPE_MARGIN = 12;
export const WIDE_RATIO = 1.4;
export const SIDE_GAP = 14;

export interface StereoLayout {
  scope: Rect;
  correlation: Rect;
  balance: Rect;
}

// A square scope with the two bars under it, or beside it in a wide panel
export function stereoLayout(area: Rect): StereoLayout {
  if (area.width > area.height * WIDE_RATIO) {
    const side = Math.floor(Math.max(0, area.height));
    const barsLeft = area.left + side + SIDE_GAP;
    const barsWidth = area.width - side - SIDE_GAP;
    const barsTop = area.top + (area.height - (2 * BAR_HEIGHT + BARS_GAP)) / 2;

    return {
      scope: { left: area.left, top: area.top, width: side, height: side },
      correlation: { left: barsLeft, top: barsTop, width: barsWidth, height: BAR_HEIGHT },
      balance: { left: barsLeft, top: barsTop + BAR_HEIGHT + BARS_GAP, width: barsWidth, height: BAR_HEIGHT },
    };
  }

  const above = { ...area, height: area.height - 2 * BAR_HEIGHT - BARS_GAP };
  const side = Math.floor(Math.max(0, Math.min(above.width, above.height)));

  return {
    scope: { left: Math.floor(above.left + above.width / 2 - side / 2), top: Math.floor(above.top + above.height / 2 - side / 2), width: side, height: side },
    correlation: { left: area.left, top: bottom(area) - 2 * BAR_HEIGHT, width: area.width, height: BAR_HEIGHT },
    balance: { left: area.left, top: bottom(area) - BAR_HEIGHT, width: area.width, height: BAR_HEIGHT },
  };
}

//==============================================================================
// LoudnessView
export const BAR_LABEL_HEIGHT = 14;
export const SCALE_WIDTH = 22;

export const CELLS = ['momentary', 'shortTerm', 'range', 'truePeak', 'clips'] as const;
export type Cell = (typeof CELLS)[number];

export interface LoudnessLayout {
  // Momentary, short-term, left peak, right peak
  bars: Rect[];
  // The bars' numbers, to their left
  scale: Rect;
  integrated: Rect;
  cells: Record<Cell, Rect>;
  // With little room: narrower bars, and only the integrated, the short-term and the max true peak
  compact: boolean;
}

const COMPACT_BELOW_WIDTH = 330;
const COMPACT_BELOW_HEIGHT = 112;
const COMPACT_ACROSS_WIDTH = 270;

export function loudnessLayout(area: Rect): LoudnessLayout {
  const SMALL_GAP = 5;
  const GROUP_GAP = 14;

  const compact = area.width < COMPACT_BELOW_WIDTH || area.height < COMPACT_BELOW_HEIGHT;
  const COLUMN_GAP = compact ? 12 : 16;

  const barAreaWidth = Math.floor(Math.min(compact ? 130 : 210, Math.max(compact ? 96 : 130, area.width * 0.36)));
  const scale: Rect = { left: area.left, top: area.top, width: SCALE_WIDTH, height: area.height - BAR_LABEL_HEIGHT };

  const barsLeft = area.left + SCALE_WIDTH + SMALL_GAP;
  const barWidth = Math.floor((barAreaWidth - SCALE_WIDTH - SMALL_GAP - 2 * SMALL_GAP - GROUP_GAP) / 4);
  const bars: Rect[] = [];
  let x = barsLeft;

  for (let i = 0; i < 4; i++) {
    bars.push({ left: x, top: area.top, width: barWidth, height: area.height });
    x += barWidth + (i === 1 ? GROUP_GAP : SMALL_GAP);
  }

  const readings: Rect = { left: area.left + barAreaWidth + COLUMN_GAP, top: area.top, width: area.width - barAreaWidth - COLUMN_GAP, height: area.height };

  if (compact) {
    // Side by side where each has the width for its number, and otherwise one over the next
    const across = readings.width >= COMPACT_ACROSS_WIDTH;
    const size = Math.floor((across ? readings.width : readings.height) / 3);
    const at = (i: number): Rect => (across ? { ...readings, left: readings.left + i * size, width: size } : { ...readings, top: readings.top + i * size, height: size });

    return { bars, scale, integrated: at(0), cells: { momentary: EMPTY, shortTerm: at(1), range: EMPTY, truePeak: at(2), clips: EMPTY }, compact };
  }

  const integratedHeight = Math.floor(readings.height * 0.46);
  const grid = { left: readings.left, top: readings.top + integratedHeight, width: readings.width, height: readings.height - integratedHeight };
  const cellWidth = Math.floor(grid.width / 3);
  const cellHeight = Math.floor(grid.height / 2);

  const cells = Object.fromEntries(
    CELLS.map((cell, i) => [cell, { left: grid.left + cellWidth * (i % 3), top: grid.top + cellHeight * Math.trunc(i / 3), width: cellWidth, height: cellHeight }])
  ) as Record<Cell, Rect>;

  return { bars, scale, integrated: { ...readings, height: integratedHeight }, cells, compact };
}
