// Mirrors the paint functions of Source/ui/meter: MeterPanel, SpectrumView, StereoFieldView and LoudnessView
import { juceHeight } from '@samplerealm/modules/editor/fonts.ts';
import type { Scales } from '../backend.ts';
import { bottom, right, type Rect } from './rect.ts';
import type { FrameReader } from './frame.ts';
import { strokeGlow } from './glow.ts';
import { acrossSpectrum, proportion } from './scales.ts';
import { clipsText, hasReading, loudnessText, truePeakText } from './text.ts';
import { colour, type Look } from './theme.ts';
import { BAR_LABEL_HEIGHT, contentBounds, expandButtonBounds, loudnessLayout, PANEL_CORNER, PANEL_INSET, PANEL_TITLE_HEIGHT, SCOPE_MARGIN, spectrumPlot, stereoLayout } from './viewLayout.ts';

type Context = CanvasRenderingContext2D;

export interface Scene {
  frame: Float32Array;
  reader: FrameReader;
  scales: Scales;
  look: Look;
}

// How much to glow right now, from 0 to 1: the beat's pulse, as far as the reactivity lets it
const glowAmount = ({ frame, reader, look }: Scene) => look.reactivity * reader.scalar(frame, 'pulse');

const font = (height: number, bold = false) => `${bold ? 700 : 400} ${juceHeight(height)}px Orbitron, sans-serif`;

type Align = 'left' | 'center' | 'right';

// juce::Graphics::drawText, centred vertically
function drawText(ctx: Context, text: string, bounds: Rect, align: Align, baseline: 'middle' | 'bottom' = 'middle') {
  ctx.textAlign = align;
  ctx.textBaseline = baseline === 'middle' ? 'middle' : 'alphabetic';
  const x = align === 'left' ? bounds.left : align === 'right' ? right(bounds) : bounds.left + bounds.width / 2;
  ctx.fillText(text, x, baseline === 'middle' ? bounds.top + bounds.height / 2 : bottom(bounds) - 2);
}

// CanvasRenderingContext2D.roundRect isn't in the web views of older systems
function roundedRect(ctx: Context | Path2D, { left, top, width, height }: Rect, radius: number) {
  const r = Math.max(0, Math.min(radius, width / 2, height / 2));
  ctx.moveTo(left + r, top);
  ctx.arcTo(left + width, top, left + width, top + height, r);
  ctx.arcTo(left + width, top + height, left, top + height, r);
  ctx.arcTo(left, top + height, left, top, r);
  ctx.arcTo(left, top, left + width, top, r);
  ctx.closePath();
}

function fillRounded(ctx: Context, bounds: Rect, radius: number) {
  ctx.beginPath();
  roundedRect(ctx, bounds, radius);
  ctx.fill();
}

//==============================================================================
// MeterPanel::paint, but for what's inside
export function drawPanel(ctx: Context, panel: Rect, title: string, expanded: boolean, look: Look) {
  const { theme } = look;

  ctx.fillStyle = colour(theme.panel);
  fillRounded(ctx, panel, PANEL_CORNER);

  ctx.strokeStyle = colour(theme.line);
  ctx.lineWidth = 1;
  ctx.beginPath();
  roundedRect(ctx, { left: panel.left + 0.5, top: panel.top + 0.5, width: panel.width - 1, height: panel.height - 1 }, PANEL_CORNER);
  ctx.stroke();

  if (look.capture) return;

  ctx.fillStyle = colour(theme.dim);
  ctx.font = font(11, true);
  drawText(ctx, title, { left: panel.left + PANEL_INSET, top: panel.top, width: panel.width - 2 * PANEL_INSET, height: PANEL_TITLE_HEIGHT + 4 }, 'left');

  // Two corners, pointing out to expand and in to go back
  const button = expandButtonBounds(panel);
  const cx = button.left + button.width / 2;
  const cy = button.top + button.height / 2;

  ctx.strokeStyle = colour(theme.dim);
  ctx.lineWidth = 1.5;
  ctx.lineJoin = 'miter';
  ctx.lineCap = 'butt';
  ctx.beginPath();

  if (expanded) {
    ctx.moveTo(cx + 2, cy - 7);
    ctx.lineTo(cx + 2, cy - 2);
    ctx.lineTo(cx + 7, cy - 2);
    ctx.moveTo(cx - 7, cy + 2);
    ctx.lineTo(cx - 2, cy + 2);
    ctx.lineTo(cx - 2, cy + 7);
  } else {
    ctx.moveTo(cx + 2, cy - 7);
    ctx.lineTo(cx + 7, cy - 7);
    ctx.lineTo(cx + 7, cy - 2);
    ctx.moveTo(cx - 7, cy + 2);
    ctx.lineTo(cx - 7, cy + 7);
    ctx.lineTo(cx - 2, cy + 7);
  }

  ctx.stroke();
}

// Too small to draw in, as while another panel takes over the dashboard
export const hasRoom = (panel: Rect) => {
  const content = contentBounds(panel);
  return content.width >= 24 && content.height >= 24;
};

//==============================================================================
// SpectrumView::curve: a smooth line through one level per band, starting at a frame offset
function spectrumCurve({ frame, reader, scales }: Scene, plot: Rect, offset: number): Path2D {
  const { numBands } = reader.layout;

  const pointAt = (band: number) =>
    [plot.left + (plot.width * (band + 0.5)) / numBands, plot.top + plot.height * (1 - proportion(frame[offset + band], scales.spectrumBottomDb, scales.spectrumTopDb))] as const;

  // Through the middle of each pair of points, bending at the points themselves
  const path = new Path2D();
  let previous = pointAt(0);
  path.moveTo(plot.left, previous[1]);
  path.lineTo(previous[0], previous[1]);

  for (let band = 1; band < numBands; band++) {
    const next = pointAt(band);
    path.quadraticCurveTo(previous[0], previous[1], (previous[0] + next[0]) / 2, (previous[1] + next[1]) / 2);
    previous = next;
  }

  path.lineTo(previous[0], previous[1]);
  path.lineTo(right(plot), previous[1]);
  return path;
}

// SpectrumView::paintContent
export function drawSpectrum(ctx: Context, scene: Scene, area: Rect) {
  const { reader, scales, look } = scene;
  const { theme } = look;
  const { minHz, maxHz } = reader.layout;
  const amount = glowAmount(scene);
  const plot = spectrumPlot(area);
  const labels: Rect = { ...area, top: bottom(plot), height: bottom(area) - bottom(plot) };

  const xOf = (hz: number) => plot.left + plot.width * acrossSpectrum(hz, minHz, maxHz);
  const yOf = (db: number) => plot.top + plot.height * (1 - proportion(db, scales.spectrumBottomDb, scales.spectrumTopDb));

  // The grid brightens with the beat
  ctx.fillStyle = colour(theme.line, 0.55 + 0.45 * amount);
  for (const hz of scales.spectrumGridHz) ctx.fillRect(xOf(hz) - 0.5, plot.top, 1, plot.height);
  for (const db of scales.spectrumGridDb) ctx.fillRect(plot.left, yOf(db) - 0.5, plot.width, 1);

  if (!look.capture) {
    ctx.fillStyle = colour(theme.dim);
    ctx.font = font(10);
    scales.spectrumGridHz.forEach((hz, i) => drawText(ctx, scales.spectrumGridLabels[i], { left: xOf(hz) - 20, top: labels.top, width: 40, height: labels.height }, 'center'));
    for (const db of scales.spectrumGridDb) drawText(ctx, `${db}`, { left: plot.left + 4, top: yOf(db) + 1, width: 40, height: 12 }, 'left');
  }

  const trace = spectrumCurve(scene, plot, reader.bandsOffset);

  const fill = new Path2D(trace);
  fill.lineTo(right(plot), bottom(plot));
  fill.lineTo(plot.left, bottom(plot));
  fill.closePath();

  ctx.save();
  ctx.beginPath();
  ctx.rect(Math.floor(plot.left), Math.floor(plot.top), Math.ceil(right(plot)) - Math.floor(plot.left), Math.ceil(bottom(plot)) - Math.floor(plot.top));
  ctx.clip();

  const down = ctx.createLinearGradient(0, plot.top, 0, bottom(plot));
  down.addColorStop(0, colour(theme.primary, 0.42 + 0.2 * amount));
  down.addColorStop(1, colour(theme.primary, 0.02));
  ctx.fillStyle = down;
  ctx.fill(fill);

  ctx.strokeStyle = colour(theme.secondary, 0.6);
  ctx.lineWidth = 1;
  ctx.lineJoin = 'miter';
  ctx.lineCap = 'butt';
  ctx.stroke(spectrumCurve(scene, plot, reader.peaksOffset));

  const across = ctx.createLinearGradient(plot.left, 0, right(plot), 0);
  across.addColorStop(0, colour(theme.primary));
  across.addColorStop(1, colour(theme.secondary));
  strokeGlow(ctx, trace, across, 1.8, amount);

  ctx.restore();
}

//==============================================================================
// The goniometer's trace, which fades like a scope's phosphor: StereoFieldView::advance. A canvas's alpha is
// 8 bits and rounds, so a fade this gentle never takes the faintest pixels to nothing, and they'd build up into
// a haze. Two canvases take turns instead: each is drawn on for two turns and shown for its second, by which
// time it holds all of the afterglow, and then starts again clean
export function stereoTrace(scales: Scales) {
  const makeLayer = (size: number) => {
    const canvas = document.createElement('canvas');
    canvas.width = canvas.height = size;
    return { canvas, ctx: canvas.getContext('2d')! };
  };

  // A turn is as long as the slowest fade takes to go under half a step of the alpha
  const turnSeconds = scales.traceFadeSeconds * (1 + Math.max(...scales.reactivityAmounts)) * Math.log(512);

  const layers = [makeLayer(0), makeLayer(0)];
  let shown = 0;
  let turn = 0;
  // How long since there was anything to draw
  let quietFor = Infinity;

  return {
    get canvas() {
      return layers[shown].canvas;
    },
    // Whether the trace still has fading to do though the frame hasn't changed
    get moving() {
      return quietFor < 2 * turnSeconds;
    },
    advance(scene: Scene, panel: Rect, seconds: number) {
      const side = Math.round(stereoLayout(contentBounds(panel)).scope.width);

      if (side < 24) {
        layers.forEach((_, i) => (layers[i] = makeLayer(0)));
        turn = 0;
        quietFor = Infinity;
        return;
      }

      // Sharp on high-resolution screens while it's small, and cheap to fade while it's big
      const scale = side <= 400 ? 2 : 1;

      const { frame, reader } = scene;
      const centre = side / 2;
      const radius = centre - SCOPE_MARGIN;
      const path = new Path2D();
      let furthest = 0;

      for (let i = 0; i < reader.layout.numPoints; i++) {
        const x = frame[reader.pointsOffset + 2 * i];
        const y = frame[reader.pointsOffset + 2 * i + 1];
        furthest = Math.max(furthest, Math.abs(x), Math.abs(y));

        if (i === 0) path.moveTo(centre + x * radius, centre - y * radius);
        else path.lineTo(centre + x * radius, centre - y * radius);
      }

      // Silence leaves the trace to fade out
      const silent = furthest < 1e-4;
      quietFor = silent ? quietFor + seconds : 0;
      turn += seconds;

      // The other canvas has had its turn, so this one starts again
      if (turn >= turnSeconds) {
        layers[shown] = makeLayer(side * scale);
        shown = 1 - shown;
        turn = 0;
      }

      const fade = Math.exp(-seconds / (scales.traceFadeSeconds * (1 + scene.look.reactivity)));

      layers.forEach((layer, i) => {
        if (layer.canvas.width !== side * scale) {
          // The trace comes along, so it doesn't go out while a panel grows or shrinks
          const resized = makeLayer(side * scale);
          if (layer.canvas.width > 0) resized.ctx.drawImage(layer.canvas, 0, 0, side * scale, side * scale);
          layers[i] = layer = resized;
        }

        const { canvas, ctx } = layer;

        // Every pixel's alpha, multiplied down
        ctx.setTransform(1, 0, 0, 1, 0, 0);
        ctx.globalCompositeOperation = 'destination-out';
        ctx.fillStyle = `rgba(0, 0, 0, ${1 - fade})`;
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        ctx.globalCompositeOperation = 'source-over';

        if (silent) return;

        ctx.setTransform(scale, 0, 0, scale, 0, 0);
        strokeGlow(ctx, path, colour(scene.look.theme.primary), 1.2, glowAmount(scene));
      });
    },
  };
}

export type StereoTrace = ReturnType<typeof stereoTrace>;

// StereoFieldView::paintBar
function drawBipolarBar(ctx: Context, look: Look, bounds: Rect, value: number, left: string, name: string, rightText: string, hotBelowZero: boolean) {
  const { theme } = look;
  const labels: Rect = { ...bounds, height: 14 };
  const track: Rect = { left: bounds.left, top: bounds.top + 14, width: bounds.width, height: 6 };

  if (!look.capture) {
    ctx.fillStyle = colour(theme.dim);
    ctx.font = font(9);
    drawText(ctx, left, labels, 'left');
    drawText(ctx, name, labels, 'center');
    drawText(ctx, rightText, labels, 'right');
  }

  ctx.fillStyle = colour(theme.line, 0.6);
  fillRounded(ctx, track, 3);

  const centre = track.left + track.width / 2;
  const x = centre + (Math.min(1, Math.max(-1, value)) * track.width) / 2;

  ctx.fillStyle = colour(hotBelowZero && value < 0 ? theme.hot : theme.primary);
  ctx.fillRect(Math.min(centre, x), track.top, Math.abs(x - centre), track.height);

  ctx.fillStyle = colour(theme.text);
  fillRounded(ctx, { left: x - 1.5, top: track.top - 2, width: 3, height: track.height + 4 }, 1.5);
}

// StereoFieldView::paintContent
export function drawStereo(ctx: Context, scene: Scene, area: Rect, trace: StereoTrace) {
  const { frame, reader, look } = scene;
  const { theme } = look;
  const layout = stereoLayout(area);
  const { scope } = layout;

  if (scope.width >= 24) {
    const cx = scope.left + scope.width / 2;
    const cy = scope.top + scope.height / 2;
    const radius = scope.width / 2 - SCOPE_MARGIN;
    const diagonal = radius * 0.70710678;

    ctx.lineWidth = 1;
    ctx.strokeStyle = colour(theme.line);
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, 2 * Math.PI);
    ctx.stroke();

    ctx.fillStyle = colour(theme.line, 0.7);
    ctx.fillRect(cx - 0.5, cy - radius, 1, radius * 2);
    ctx.fillRect(cx - radius, cy - 0.5, radius * 2, 1);

    ctx.strokeStyle = colour(theme.line, 0.45);
    ctx.beginPath();
    ctx.moveTo(cx - diagonal, cy - diagonal);
    ctx.lineTo(cx + diagonal, cy + diagonal);
    ctx.moveTo(cx - diagonal, cy + diagonal);
    ctx.lineTo(cx + diagonal, cy - diagonal);
    ctx.stroke();

    if (!look.capture) {
      ctx.fillStyle = colour(theme.dim);
      ctx.font = font(10);
      const label = (text: string, x: number, y: number) => drawText(ctx, text, { left: x - 8, top: y - 6, width: 16, height: 12 }, 'center');
      label('L', cx - diagonal - 9, cy - diagonal - 7);
      label('R', cx + diagonal + 9, cy - diagonal - 7);
    }

    if (trace.canvas.width > 0) ctx.drawImage(trace.canvas, scope.left, scope.top, scope.width, scope.height);
  }

  drawBipolarBar(ctx, look, layout.correlation, reader.scalar(frame, 'correlation'), '-1', 'CORRELATION', '+1', true);
  drawBipolarBar(ctx, look, layout.balance, reader.scalar(frame, 'balance'), 'L', 'BALANCE', 'R', false);
}

//==============================================================================
// LoudnessView::paintBar
function drawLevelBar(ctx: Context, scene: Scene, bounds: Rect, name: string, value: number, hotOverZero: boolean) {
  const { look, scales } = scene;
  const { theme } = look;
  const track: Rect = { ...bounds, height: bounds.height - BAR_LABEL_HEIGHT };
  const label: Rect = { ...bounds, top: bottom(track), height: BAR_LABEL_HEIGHT };

  if (!look.capture) {
    ctx.fillStyle = colour(theme.dim);
    ctx.font = font(9);
    drawText(ctx, name, label, 'center', 'bottom');
  }

  ctx.fillStyle = colour(theme.line, 0.45);
  fillRounded(ctx, track, 2);

  const top = bottom(track) - track.height * proportion(value, scales.levelBottom, scales.levelTop);

  if (top < bottom(track)) {
    // Brighter the higher it goes, and stronger with the beat
    const up = ctx.createLinearGradient(0, track.top, 0, bottom(track));
    up.addColorStop(0, colour(theme.secondary));
    up.addColorStop(1, colour(theme.primary));
    ctx.fillStyle = up;
    const before = ctx.globalAlpha;
    ctx.globalAlpha = before * (0.8 + 0.2 * glowAmount(scene));
    ctx.fillRect(track.left, top, track.width, bottom(track) - top);
    ctx.globalAlpha = before;
  }

  // Over full scale
  const zero = bottom(track) - track.height * proportion(0, scales.levelBottom, scales.levelTop);

  if (hotOverZero && top < zero) {
    ctx.fillStyle = colour(theme.hot);
    ctx.fillRect(track.left, top, track.width, zero - top);
  }
}

// LoudnessView::paintReading
function drawReading(ctx: Context, look: Look, bounds: Rect, name: string, value: string, style: string, largest: number) {
  ctx.fillStyle = colour(look.theme.dim);
  ctx.font = font(9);
  drawText(ctx, name, { ...bounds, height: 14 }, 'left');

  // As big as fits, for five characters or so
  const below: Rect = { ...bounds, top: bounds.top + 14, height: bounds.height - 14 };
  const height = Math.min(largest, Math.max(12, Math.min(below.height - 4, below.width / 3.6)));

  ctx.fillStyle = style;
  ctx.font = font(height, true);
  drawText(ctx, value, { ...below, height: Math.min(below.height, height + 4) }, 'left');
}

// LoudnessView::paintContent
export function drawLoudness(ctx: Context, scene: Scene, area: Rect) {
  const { frame, reader, scales, look } = scene;
  const { theme } = look;
  const { noLoudness } = reader.layout;
  const layout = loudnessLayout(area);
  const value = (name: Parameters<FrameReader['scalar']>[1]) => reader.scalar(frame, name);

  const track = { ...layout.bars[0], height: layout.bars[0].height - BAR_LABEL_HEIGHT };
  const yOf = (level: number) => bottom(track) - track.height * proportion(level, scales.levelBottom, scales.levelTop);

  if (!look.capture) {
    ctx.fillStyle = colour(theme.dim);
    ctx.font = font(9);
    for (const tick of scales.levelTicks) drawText(ctx, `${tick}`, { left: layout.scale.left, top: yOf(tick) - 6, width: layout.scale.width, height: 12 }, 'right');
  }

  drawLevelBar(ctx, scene, layout.bars[0], 'MOM', value('momentary'), false);
  drawLevelBar(ctx, scene, layout.bars[1], 'ST', value('shortTerm'), false);
  drawLevelBar(ctx, scene, layout.bars[2], 'L', value('truePeakLeft'), true);
  drawLevelBar(ctx, scene, layout.bars[3], 'R', value('truePeakRight'), true);

  // The integrated loudness as a line across the loudness bars
  const integrated = value('integrated');

  if (hasReading(integrated, noLoudness)) {
    ctx.fillStyle = colour(theme.text);
    ctx.fillRect(layout.bars[0].left - 2, yOf(integrated) - 1, right(layout.bars[1]) + 2 - (layout.bars[0].left - 2), 2);
  }

  const paused = value('integrating') < 0.5;
  drawReading(ctx, look, layout.integrated, paused ? 'INTEGRATED LUFS - PAUSED' : 'INTEGRATED LUFS', loudnessText(integrated, noLoudness), colour(theme.primary), 96);

  const text = colour(theme.text);
  const hot = colour(theme.hot);
  const { cells } = layout;

  drawReading(ctx, look, cells.momentary, 'MOMENTARY', loudnessText(value('momentary'), noLoudness), text, 40);
  drawReading(ctx, look, cells.shortTerm, 'SHORT-TERM', loudnessText(value('shortTerm'), noLoudness), text, 40);
  drawReading(ctx, look, cells.range, 'RANGE LU', loudnessText(value('range'), noLoudness), text, 40);
  drawReading(ctx, look, cells.truePeak, 'TRUE PEAK MAX', truePeakText(value('maxTruePeak'), noLoudness), value('maxTruePeak') > 0 ? hot : text, 40);
  drawReading(ctx, look, cells.clips, 'CLIPS', clipsText(value('clipCount')), value('clipCount') > 0 ? hot : text, 40);
}
