// Mirrors Source/ui/meter/Dashboard.cpp: the meter's panels on one canvas. One can grow to fill the
// dashboard while the others fade out
import { useEffect, useRef } from 'react';
import type { MeterBackend, PanelSizes } from '../backend.ts';
import { contains, type Point, type Rect } from './rect.ts';
import { dividers, dragged, ease, expanding, PANELS, reset, sameSizes, type Divider, type Panel, type Shown } from './dashboardLayout.ts';
import { drawLoudness, drawPanel, drawSpectrum, drawStereo, drawWaveform, hasRoom, stereoTrace, type Scene } from './draw.ts';
import { frameReader } from './frame.ts';
import { colour, type Look } from './theme.ts';
import { contentBounds, expandButtonBounds } from './viewLayout.ts';

// The same numbers as the view parameter's choices: the dashboard, then each panel expanded
export const DASHBOARD_VIEW = 0;

// MeterEngine::maxFrameSeconds: a stalled page catches up over a few frames rather than in one jump
const MAX_FRAME_SECONDS = 0.25;

const TITLES: Record<Panel, string> = { spectrum: 'SPECTRUM', stereo: 'STEREO FIELD', loudness: 'LOUDNESS', waveform: 'WAVEFORM' };

interface DashboardProps {
  backend: MeterBackend;
  look: Look;
  view: number;
  // Which panels the dashboard has. An expanded panel shows whether it's one of them or not
  shown: Shown;
  bounds: Rect;
  // Called with the view a click asks for
  onViewChosen(view: number): void;
}

export function Dashboard({ backend, look, view, shown, bounds, onViewChosen }: DashboardProps) {
  const canvas = useRef<HTMLCanvasElement>(null);

  // What the drawing loop reads, kept out of React's renders. The sizes come straight from the backend, so a drag renders nothing
  const live = useRef({
    look,
    view,
    shown,
    sizes: backend.panelSizes(),
    bounds,
    onViewChosen,
    expanded: 'spectrum' as Panel,
    expansion: 0,
    opened: false,
    changed: true,
    // Where the mouse is, for the spectrum's readout
    hover: null as Point | null,
  });
  const state = live.current;

  // The gap last pressed, with the sizes and the place the press began
  const pressed = useRef<{ divider: Divider; from: PanelSizes; x: number; y: number } | null>(null);

  if (state.view !== view || !state.opened) {
    const animate = state.opened;

    if (view !== DASHBOARD_VIEW) {
      const panel = PANELS[Math.min(PANELS.length, Math.max(1, view)) - 1];
      // Straight from one panel to another, there's nothing to grow from, and the editor opens already in its view
      if (!animate || (panel !== state.expanded && state.expansion > 0)) state.expansion = 1;
      state.expanded = panel;
    } else if (!animate) {
      state.expansion = 0;
    }

    state.opened = true;
  }

  Object.assign(state, { look, view, shown, bounds, onViewChosen, changed: true });

  const local = () => ({ left: 0, top: 0, width: state.bounds.width, height: state.bounds.height });
  const panelsNow = () => expanding(local(), state.shown, state.expanded, ease(state.expansion), state.sizes);

  // Dashboard::getDividers: the gaps that can be dragged, none while a panel is expanded
  const dividerAt = (x: number, y: number) =>
    state.view === DASHBOARD_VIEW && state.expansion === 0 ? dividers(local(), state.shown, state.sizes).find((divider) => contains(divider.area, x, y)) : undefined;

  // Dashboard::layOut: the expanded panel once it has started to grow, and the dashboard's own until it has filled
  const isExpanded = (panel: Panel) => panel === state.expanded && state.expansion > 0;
  const isVisible = (panel: Panel) => isExpanded(panel) || (state.shown[panel] && state.expansion < 1);

  // Not for the dashboard's only panel, which has nothing to expand over
  const isExpandable = () => state.view !== DASHBOARD_VIEW || PANELS.filter((panel) => state.shown[panel]).length > 1;

  // The panel under a point: the expanded one is over the others
  const panelAt = (x: number, y: number) => {
    const panels = panelsNow();
    const order = state.expansion > 0 ? [state.expanded, ...PANELS.filter((p) => p !== state.expanded)] : PANELS;
    return order.filter(isVisible).find((p) => contains(panels[p], x, y));
  };

  const overSpectrum = (at: Point | null) => (at && panelAt(at.x, at.y) === 'spectrum' ? at : null);

  useEffect(() => {
    const element = canvas.current;
    const ctx = element?.getContext('2d');
    if (!element || !ctx) return;

    const reader = frameReader(backend.meter.frame);
    const { scales } = backend.meter;
    const trace = stereoTrace(scales);
    let frame = reader.silent();
    let fresh = true;
    let lastDraw = performance.now();
    let lastFrame = lastDraw;
    let request = 0;

    const showSizes = (next: PanelSizes) => Object.assign(state, { sizes: next, changed: true });
    showSizes(backend.panelSizes());
    const stopSizes = backend.onPanelSizes(showSizes);

    const stopListening = backend.onFrame((next) => {
      if (next.length !== reader.size) return;
      // A stopped or silent meter soon shows the same frame every time, and there's nothing to draw again
      fresh ||= trace.moving || next.some((value, i) => value !== frame[i]);
      frame = next;
    });

    const draw = (now: number) => {
      request = requestAnimationFrame(draw);

      const seconds = Math.min(MAX_FRAME_SECONDS, (now - lastDraw) / 1000);
      lastDraw = now;

      const target = state.view === DASHBOARD_VIEW ? 0 : 1;
      const moving = state.expansion !== target;

      if (moving) {
        const step = seconds / scales.expandSeconds;
        state.expansion = target > state.expansion ? Math.min(target, state.expansion + step) : Math.max(target, state.expansion - step);
      }

      if (!fresh && !moving && !state.changed) return;

      const { width, height } = state.bounds;
      const ratio = window.devicePixelRatio || 1;

      if (element.width !== Math.round(width * ratio) || element.height !== Math.round(height * ratio)) {
        element.width = Math.round(width * ratio);
        element.height = Math.round(height * ratio);
      }

      const scene: Scene = { frame, reader, scales, look: state.look };
      const panels = panelsNow();
      const eased = ease(state.expansion);
      const spectrumHover = overSpectrum(state.hover);

      if (fresh) {
        trace.advance(scene, panels.stereo, Math.min(MAX_FRAME_SECONDS, (now - lastFrame) / 1000));
        lastFrame = now;
      }

      fresh = false;
      state.changed = false;

      ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
      ctx.globalAlpha = 1;
      ctx.fillStyle = colour(state.look.theme.background);
      ctx.fillRect(0, 0, width, height);

      // The expanded panel last, over the others
      for (const panel of [...PANELS.filter((p) => p !== state.expanded), state.expanded]) {
        if (!isVisible(panel)) continue;

        ctx.globalAlpha = isExpanded(panel) ? 1 : 1 - eased;
        drawPanel(ctx, panels[panel], TITLES[panel], isExpanded(panel) && state.view !== DASHBOARD_VIEW, isExpandable(), state.look);

        if (hasRoom(panels[panel])) {
          const area = contentBounds(panels[panel]);
          if (panel === 'spectrum') drawSpectrum(ctx, scene, area, spectrumHover);
          else if (panel === 'stereo') drawStereo(ctx, scene, area, trace);
          else if (panel === 'loudness') drawLoudness(ctx, scene, area);
          else drawWaveform(ctx, scene, area);
        }
      }

      ctx.globalAlpha = 1;
    };

    request = requestAnimationFrame(draw);

    return () => {
      cancelAnimationFrame(request);
      stopListening();
      stopSizes();
    };
  }, [backend, state]);

  // Only the spectrum draws the mouse, so a move elsewhere needs no redraw
  const hoverAt = (to: Point | null) => {
    if (overSpectrum(state.hover) || overSpectrum(to)) state.changed = true;
    state.hover = to;
  };

  // The panel under a point, and what a click there does
  const hit = (x: number, y: number) => {
    const panel = panelAt(x, y);
    if (!panel || !isExpandable()) return null;

    const onButton = !state.look.capture && contains(expandButtonBounds(panelsNow()[panel]), x, y);
    return { panel, onButton };
  };

  const toggle = (panel: Panel) => {
    const panelView = PANELS.indexOf(panel) + 1;
    state.onViewChosen(state.view === panelView ? DASHBOARD_VIEW : panelView);
  };

  const point = (e: { clientX: number; clientY: number; currentTarget: HTMLElement }) => {
    const box = e.currentTarget.getBoundingClientRect();
    // In the editor's own pixels, which the demo's scaling doesn't change
    const scale = box.width > 0 ? state.bounds.width / box.width : 1;
    return { x: (e.clientX - box.left) * scale, y: (e.clientY - box.top) * scale };
  };

  return (
    <canvas
      ref={canvas}
      className="absolute block"
      style={{ left: bounds.left, top: bounds.top, width: bounds.width, height: bounds.height }}
      role="img"
      aria-label="Spectrum, waveform, stereo field and loudness meters"
      onPointerDown={(e) => {
        const { x, y } = point(e);
        const divider = e.button === 0 ? dividerAt(x, y) : undefined;
        pressed.current = divider ? { divider, from: state.sizes, x, y } : null;
        if (!divider) return;
        e.currentTarget.setPointerCapture(e.pointerId);
        // The panels move under the point while it drags
        hoverAt(null);
      }}
      onClick={(e) => {
        // A drag let go over a panel's button isn't a click on it
        if (pressed.current) return;
        const { x, y } = point(e);
        const found = hit(x, y);
        if (found?.onButton) toggle(found.panel);
      }}
      onDoubleClick={(e) => {
        const { x, y } = point(e);
        const divider = dividerAt(x, y);
        const found = hit(x, y);
        if (divider) backend.setPanelSizes(reset(state.sizes, divider.group));
        else if (found && !found.onButton) toggle(found.panel);
      }}
      onPointerLeave={() => hoverAt(null)}
      onPointerMove={(e) => {
        const { x, y } = point(e);
        const held = pressed.current;

        if (held && e.currentTarget.hasPointerCapture(e.pointerId)) {
          const next = dragged(local(), state.shown, held.from, held.divider, held.divider.across ? x - held.x : y - held.y);
          if (!sameSizes(next, state.sizes)) backend.setPanelSizes(next);
          return;
        }

        hoverAt({ x, y });
        const divider = dividerAt(x, y);
        e.currentTarget.style.cursor = divider ? (divider.across ? 'col-resize' : 'row-resize') : hit(x, y)?.onButton ? 'pointer' : 'default';
      }}
    />
  );
}
