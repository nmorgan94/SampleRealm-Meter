// Mirrors Source/ui/meter/DashboardLayout.h, with the same tests
import { reduced, type Rect } from '../layout.ts';

export const PANELS = ['spectrum', 'stereo', 'loudness'] as const;
export type Panel = (typeof PANELS)[number];
export type Panels = Record<Panel, Rect>;

export const PADDING = 10;
export const GAP = 10;

// The spectrum across the top, the stereo field and the loudness side by side below
export function dashboard(bounds: Rect): Panels {
  const inner = reduced(bounds, PADDING);
  const spectrumHeight = Math.trunc(((inner.height - GAP) * 58) / 100);
  const below = { top: inner.top + spectrumHeight + GAP, height: inner.height - spectrumHeight - GAP };
  const stereoWidth = Math.trunc(((inner.width - GAP) * 40) / 100);

  return {
    spectrum: { left: inner.left, top: inner.top, width: inner.width, height: spectrumHeight },
    stereo: { left: inner.left, ...below, width: stereoWidth },
    loudness: { left: inner.left + stereoWidth + GAP, ...below, width: inner.width - stereoWidth - GAP },
  };
}

// The dashboard with one panel some way, from 0 to 1, to filling it
export function expanding(bounds: Rect, expanded: Panel, amount: number): Panels {
  const panels = dashboard(bounds);
  const from = panels[expanded];
  const to = reduced(bounds, PADDING);
  const along = (a: number, b: number) => Math.round(a + (b - a) * amount);

  return {
    ...panels,
    [expanded]: { left: along(from.left, to.left), top: along(from.top, to.top), width: along(from.width, to.width), height: along(from.height, to.height) },
  };
}

// Slow at each end
export const ease = (amount: number) => amount * amount * (3 - 2 * amount);
