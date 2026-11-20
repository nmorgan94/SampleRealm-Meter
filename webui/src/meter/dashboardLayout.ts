// Mirrors Source/ui/meter/DashboardLayout.h, with the same tests
import { EMPTY, reduced, type Rect } from './rect.ts';

// In the order of the view parameter's choices after the dashboard
export const PANELS = ['spectrum', 'stereo', 'loudness', 'waveform'] as const;
export type Panel = (typeof PANELS)[number];
export type Panels = Record<Panel, Rect>;

// Which panels the dashboard has. One that isn't shown gets an empty rectangle
export type Shown = Record<Panel, boolean>;
export const ALL_SHOWN: Shown = { spectrum: true, stereo: true, loudness: true, waveform: true };

export const PADDING = 10;
export const GAP = 10;

// A dashboard shorter than this is one row
export const ROW_BELOW_HEIGHT = 416;

type Weighted = readonly [Panel, number];

const SPECTRUM_ROW_WEIGHT = 58;
const WAVEFORM_ROW_WEIGHT = 25;
const LOWER_ROW_WEIGHT = 42;
const LOWER_ROW: Weighted[] = [
  ['stereo', 40],
  ['loudness', 60],
];
const ONE_ROW: Weighted[] = [
  ['spectrum', 3],
  ['waveform', 3],
  ['stereo', 2],
  ['loudness', 3],
];

// An area cut by weight into parts with gaps between, across or down. The last part takes what rounding leaves
function divided(area: Rect, weights: number[], across: boolean): Rect[] {
  const length = across ? area.width : area.height;
  const room = length - GAP * (weights.length - 1);
  const total = weights.reduce((sum, weight) => sum + weight, 0);
  let at = across ? area.left : area.top;
  const end = at + length;

  return weights.map((weight, i) => {
    const size = i === weights.length - 1 ? end - at : Math.trunc((room * weight) / total);
    const part = across ? { ...area, left: at, width: size } : { ...area, top: at, height: size };
    at += size + GAP;
    return part;
  });
}

// The spectrum over the waveform, with the stereo field and the loudness side by side below, or all in one row
// in a short dashboard. What's shown fills the space
export function dashboard(bounds: Rect, shown: Shown = ALL_SHOWN): Panels {
  const inner = reduced(bounds, PADDING);
  const panels: Panels = { spectrum: EMPTY, stereo: EMPTY, loudness: EMPTY, waveform: EMPTY };

  const placeAcross = (area: Rect, row: Weighted[]) => {
    const present = row.filter(([panel]) => shown[panel]);
    const parts = divided(
      area,
      present.map(([, weight]) => weight),
      true
    );
    present.forEach(([panel], i) => (panels[panel] = parts[i]));
  };

  if (bounds.height < ROW_BELOW_HEIGHT) {
    placeAcross(inner, ONE_ROW);
    return panels;
  }

  const hasLowerRow = shown.stereo || shown.loudness;
  const rows = divided(inner, [...(shown.spectrum ? [SPECTRUM_ROW_WEIGHT] : []), ...(shown.waveform ? [WAVEFORM_ROW_WEIGHT] : []), ...(hasLowerRow ? [LOWER_ROW_WEIGHT] : [])], false);

  if (shown.spectrum) panels.spectrum = rows.shift()!;
  if (shown.waveform) panels.waveform = rows.shift()!;
  if (hasLowerRow) placeAcross(rows.shift()!, LOWER_ROW);

  return panels;
}

// The dashboard with one panel some way, from 0 to 1, to filling it
export function expanding(bounds: Rect, shown: Shown, expanded: Panel, amount: number): Panels {
  const panels = dashboard(bounds, shown);
  const to = reduced(bounds, PADDING);

  // A panel the dashboard doesn't show grows from the middle
  const from = shown[expanded] ? panels[expanded] : { left: to.left + to.width / 2, top: to.top + to.height / 2, width: 0, height: 0 };
  const along = (a: number, b: number) => Math.round(a + (b - a) * amount);

  return {
    ...panels,
    [expanded]: { left: along(from.left, to.left), top: along(from.top, to.top), width: along(from.width, to.width), height: along(from.height, to.height) },
  };
}

// Slow at each end
export const ease = (amount: number) => amount * amount * (3 - 2 * amount);
