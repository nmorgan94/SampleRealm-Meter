// juce::Rectangle's functions the meter's layouts use
import type { Rect } from '@samplerealm/modules/editor/layout.ts';

export type { Rect };

export const right = (r: Rect) => r.left + r.width;
export const bottom = (r: Rect) => r.top + r.height;
export const contains = (r: Rect, x: number, y: number) => x >= r.left && x < right(r) && y >= r.top && y < bottom(r);

// juce::Rectangle::reduced
export const reduced = ({ left, top, width, height }: Rect, by: number): Rect => ({ left: left + by, top: top + by, width: width - 2 * by, height: height - 2 * by });
