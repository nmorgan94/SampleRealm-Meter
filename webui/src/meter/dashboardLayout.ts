// Mirrors Source/ui/meter/DashboardLayout.h, with the same tests
import type { PanelSizes } from '../backend.ts';
import { bottom, EMPTY, reduced, right, type Rect } from './rect.ts';

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

// A drag leaves a panel at least this big
export const MIN_PANEL_WIDTH = 120;
export const MIN_PANEL_HEIGHT = 64;

// The spaces that are shared out: the dashboard's rows, the lower of them, and the one row of a short dashboard
export type Group = keyof PanelSizes;
export type Sizes = PanelSizes;

// The panel in each slot of the groups that hold panels. The rows are these two, then the lower row
const ROW_PANELS: Panel[] = ['spectrum', 'waveform'];
const LOWER_PANELS: Panel[] = ['stereo', 'loudness'];
const ONE_ROW_PANELS: Panel[] = ['spectrum', 'waveform', 'stereo', 'loudness'];
const LOWER_ROW_SLOT = 2;

// How each group's space is shared between its slots, as weights
export const defaultSizes = (): Sizes => ({ rows: [58, 25, 42], lower: [40, 60], oneRow: [3, 3, 2, 3] });

const GROUPS: Group[] = ['rows', 'lower', 'oneRow'];

export const sameSizes = (a: Sizes, b: Sizes) => GROUPS.every((group) => a[group].length === b[group].length && a[group].every((weight, i) => weight === b[group][i]));

// PanelSizesState::from: each list as numbers, or as text with spaces between. One of the wrong length, or with a
// weight that isn't over 0, keeps its default
export function sanitiseSizes(value: Partial<Record<Group, unknown>> | undefined): Sizes {
  const sizes = defaultSizes();

  for (const group of GROUPS) {
    const given = value?.[group];
    const list = Array.isArray(given) ? given.map(Number) : typeof given === 'string' ? given.trim().split(/\s+/).map(Number) : [];
    if (list.length === sizes[group].length && list.every((weight) => Number.isFinite(weight) && weight > 0)) sizes[group] = list;
  }

  return sizes;
}

// Which of a group's slots have anything in them
function present(group: Group, shown: Shown): boolean[] {
  if (group === 'rows') return [shown.spectrum, shown.waveform, shown.stereo || shown.loudness];
  if (group === 'lower') return [shown.stereo, shown.loudness];
  return [shown.spectrum, shown.waveform, shown.stereo, shown.loudness];
}

interface Part {
  slot: number;
  area: Rect;
}

// An area cut by weight into its present slots with gaps between, across or down. The last part takes what rounding leaves
function divided(area: Rect, weights: number[], slots: boolean[], across: boolean): Part[] {
  const shownSlots = weights.map((_, slot) => slot).filter((slot) => slots[slot]);
  const total = shownSlots.reduce((sum, slot) => sum + weights[slot], 0);
  const length = across ? area.width : area.height;
  const room = length - GAP * (shownSlots.length - 1);
  let at = across ? area.left : area.top;
  const end = at + length;

  return shownSlots.map((slot, i) => {
    const size = i === shownSlots.length - 1 ? end - at : Math.trunc((room * weights[slot]) / total);
    const part = across ? { ...area, left: at, width: size } : { ...area, top: at, height: size };
    at += size + GAP;
    return { slot, area: part };
  });
}

const isOneRow = (bounds: Rect) => bounds.height < ROW_BELOW_HEIGHT;

// Whether a group's parts sit side by side
const isAcross = (group: Group) => group !== 'rows';

const lowerParts = (row: Rect, shown: Shown, sizes: Sizes) => divided(row, sizes.lower, present('lower', shown), isAcross('lower'));

// A group's parts in a dashboard that has it
function partsOf(group: Group, bounds: Rect, shown: Shown, sizes: Sizes): Part[] {
  if (group !== 'lower') return divided(reduced(bounds, PADDING), sizes[group], present(group, shown), isAcross(group));

  const row = partsOf('rows', bounds, shown, sizes).find((part) => part.slot === LOWER_ROW_SLOT);
  return row ? lowerParts(row.area, shown, sizes) : [];
}

// The spectrum over the waveform, with the stereo field and the loudness side by side below, or all in one row
// in a short dashboard. What's shown fills the space
export function dashboard(bounds: Rect, shown: Shown = ALL_SHOWN, sizes: Sizes = defaultSizes()): Panels {
  const panels: Panels = { spectrum: EMPTY, stereo: EMPTY, loudness: EMPTY, waveform: EMPTY };

  if (isOneRow(bounds)) {
    for (const part of partsOf('oneRow', bounds, shown, sizes)) panels[ONE_ROW_PANELS[part.slot]] = part.area;
    return panels;
  }

  for (const row of partsOf('rows', bounds, shown, sizes)) {
    if (row.slot !== LOWER_ROW_SLOT) panels[ROW_PANELS[row.slot]] = row.area;
    else for (const part of lowerParts(row.area, shown, sizes)) panels[LOWER_PANELS[part.slot]] = part.area;
  }

  return panels;
}

// The gap between two neighbouring parts of a group, which a drag moves
export interface Divider {
  group: Group;
  // The slots either side
  before: number;
  after: number;
  // Between parts side by side
  across: boolean;
  area: Rect;
}

export function dividers(bounds: Rect, shown: Shown = ALL_SHOWN, sizes: Sizes = defaultSizes()): Divider[] {
  const between = (group: Group) => {
    const parts = partsOf(group, bounds, shown, sizes);
    const across = isAcross(group);

    return parts.slice(1).map((part, i): Divider => {
      const before = parts[i];
      const area = across ? { ...before.area, left: right(before.area), width: GAP } : { ...before.area, top: bottom(before.area), height: GAP };
      return { group, before: before.slot, after: part.slot, across, area };
    });
  };

  return isOneRow(bounds) ? between('oneRow') : [...between('rows'), ...between('lower')];
}

// The sizes after a divider has been dragged some pixels right or down. Its two neighbours share what they had,
// neither going under the minimum, and the group's weights become its parts' pixels
export function dragged(bounds: Rect, shown: Shown, sizes: Sizes, divider: Divider, pixels: number): Sizes {
  const parts = partsOf(divider.group, bounds, shown, sizes);
  const lengthOf = (part: Part) => (divider.across ? part.area.width : part.area.height);
  const lengthIn = (slot: number) => {
    const part = parts.find((p) => p.slot === slot);
    return part ? lengthOf(part) : 0;
  };

  const before = lengthIn(divider.before);
  const after = lengthIn(divider.after);

  // Nothing there to share out
  if (parts.some((part) => lengthOf(part) < 1) || before < 1 || after < 1) return sizes;

  // A panel already under the minimum, in a small editor, isn't made to grow
  const smallest = divider.across ? MIN_PANEL_WIDTH : MIN_PANEL_HEIGHT;
  const moved = Math.min(before + after - Math.min(smallest, after), Math.max(Math.min(smallest, before), before + Math.round(pixels)));
  if (moved === before) return sizes;

  const total = parts.reduce((sum, part) => sum + sizes[divider.group][part.slot], 0);
  const room = parts.reduce((sum, part) => sum + lengthOf(part), 0);

  // The slots that aren't showing keep their share
  const weights = sizes[divider.group].map((weight) => weight * (room / total));

  for (const part of parts) weights[part.slot] = lengthOf(part);
  weights[divider.before] = moved;
  weights[divider.after] = before + after - moved;

  return { ...sizes, [divider.group]: weights };
}

// The sizes with a group's back as they began
export const reset = (sizes: Sizes, group: Group): Sizes => ({ ...sizes, [group]: defaultSizes()[group] });

// The dashboard with one panel some way, from 0 to 1, to filling it
export function expanding(bounds: Rect, shown: Shown, expanded: Panel, amount: number, sizes: Sizes = defaultSizes()): Panels {
  const panels = dashboard(bounds, shown, sizes);
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
