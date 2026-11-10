// Mirrors Source/shared/MeterFrame.h, whose layout arrives in the UI info
import type { FrameLayout } from '../backend.ts';

export type Scalar =
  | 'momentary'
  | 'shortTerm'
  | 'integrated'
  | 'range'
  | 'truePeakLeft'
  | 'truePeakRight'
  | 'maxTruePeak'
  | 'clipCount'
  | 'correlation'
  | 'balance'
  | 'pulse'
  | 'integrating';

export interface FrameReader {
  layout: FrameLayout;
  // Where the bands, their held peaks and the goniometer's x, y pairs start
  bandsOffset: number;
  peaksOffset: number;
  pointsOffset: number;
  size: number;
  scalar(frame: Float32Array, name: Scalar): number;
  // MeterFrame::silent
  silent(): Float32Array;
}

export function frameReader(layout: FrameLayout): FrameReader {
  const index = new Map(layout.scalars.map((name, i) => [name, i]));
  const bandsOffset = layout.scalars.length;
  const peaksOffset = bandsOffset + layout.numBands;
  const pointsOffset = peaksOffset + layout.numBands;
  const size = pointsOffset + 2 * layout.numPoints;

  const at = (name: Scalar) => {
    const found = index.get(name);
    if (found === undefined) throw new Error(`No ${name} in the frame`);
    return found;
  };

  return {
    layout,
    bandsOffset,
    peaksOffset,
    pointsOffset,
    size,
    scalar: (frame, name) => frame[at(name)],
    silent: () => {
      const frame = new Float32Array(size);
      for (const name of ['momentary', 'shortTerm', 'integrated', 'range', 'maxTruePeak'] as const) frame[at(name)] = layout.noLoudness;
      frame[at('truePeakLeft')] = frame[at('truePeakRight')] = layout.peakFloorDb;
      frame[at('integrating')] = 1;
      frame.fill(layout.bandFloorDb, bandsOffset, pointsOffset);
      return frame;
    },
  };
}

// The plugin sends a frame as its bytes in base64
export function decodeFrame(base64: string): Float32Array {
  const text = atob(base64);
  const bytes = new Uint8Array(text.length);
  for (let i = 0; i < text.length; i++) bytes[i] = text.charCodeAt(i);
  return new Float32Array(bytes.buffer);
}
