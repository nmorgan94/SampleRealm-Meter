// Mirrors Source/ui/meter/Dashboard.cpp: the meter's three panels on one canvas. One can grow to fill the
// dashboard while the others fade out
import { useEffect, useRef } from 'react';
import type { MeterBackend } from '../backend.ts';
import { contains, type Rect } from './rect.ts';
import { ease, expanding, PANELS, type Panel } from './dashboardLayout.ts';
import { drawLoudness, drawPanel, drawSpectrum, drawStereo, hasRoom, stereoTrace, type Scene } from './draw.ts';
import { frameReader } from './frame.ts';
import { colour, type Look } from './theme.ts';
import { contentBounds, expandButtonBounds } from './viewLayout.ts';

// The same numbers as the view parameter's choices: the dashboard, then each panel expanded
export const DASHBOARD_VIEW = 0;

// MeterEngine::maxFrameSeconds: a stalled page catches up over a few frames rather than in one jump
const MAX_FRAME_SECONDS = 0.25;

const TITLES: Record<Panel, string> = { spectrum: 'SPECTRUM', stereo: 'STEREO FIELD', loudness: 'LOUDNESS' };

interface DashboardProps {
  backend: MeterBackend;
  look: Look;
  view: number;
  bounds: Rect;
  // Called with the view a click asks for
  onViewChosen(view: number): void;
}

export function Dashboard({ backend, look, view, bounds, onViewChosen }: DashboardProps) {
  const canvas = useRef<HTMLCanvasElement>(null);

  // What the drawing loop reads, kept out of React's renders
  const live = useRef({ look, view, bounds, onViewChosen, expanded: 'spectrum' as Panel, expansion: 0, shown: false, changed: true });
  const state = live.current;

  if (state.view !== view || !state.shown) {
    const animate = state.shown;

    if (view !== DASHBOARD_VIEW) {
      const panel = PANELS[Math.min(PANELS.length, Math.max(1, view)) - 1];
      // Straight from one panel to another, there's nothing to grow from, and the editor opens already in its view
      if (!animate || (panel !== state.expanded && state.expansion > 0)) state.expansion = 1;
      state.expanded = panel;
    } else if (!animate) {
      state.expansion = 0;
    }

    state.shown = true;
  }

  Object.assign(state, { look, view, bounds, onViewChosen, changed: true });

  const panelsNow = () => expanding({ left: 0, top: 0, width: state.bounds.width, height: state.bounds.height }, state.expanded, ease(state.expansion));

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
        const isExpanded = panel === state.expanded;
        if (!isExpanded && state.expansion >= 1) continue;

        ctx.globalAlpha = isExpanded ? 1 : 1 - eased;
        drawPanel(ctx, panels[panel], TITLES[panel], isExpanded && state.view !== DASHBOARD_VIEW, state.look);

        if (hasRoom(panels[panel])) {
          const area = contentBounds(panels[panel]);
          if (panel === 'spectrum') drawSpectrum(ctx, scene, area);
          else if (panel === 'stereo') drawStereo(ctx, scene, area, trace);
          else drawLoudness(ctx, scene, area);
        }
      }

      ctx.globalAlpha = 1;
    };

    request = requestAnimationFrame(draw);

    return () => {
      cancelAnimationFrame(request);
      stopListening();
    };
  }, [backend, state]);

  // The panel under a point, and what a click there does
  const hit = (x: number, y: number) => {
    const panels = panelsNow();
    const order = state.expansion > 0 ? [state.expanded, ...PANELS.filter((p) => p !== state.expanded)] : PANELS;
    const visible = order.filter((panel) => panel === state.expanded || state.expansion < 1);
    const panel = visible.find((p) => contains(panels[p], x, y));
    if (!panel) return null;

    const onButton = !state.look.capture && contains(expandButtonBounds(panels[panel]), x, y);
    return { panel, onButton };
  };

  const toggle = (panel: Panel) => {
    const panelView = PANELS.indexOf(panel) + 1;
    state.onViewChosen(state.view === panelView ? DASHBOARD_VIEW : panelView);
  };

  const at = (e: { clientX: number; clientY: number; currentTarget: HTMLElement }) => {
    const box = e.currentTarget.getBoundingClientRect();
    // In the editor's own pixels, which the demo's scaling doesn't change
    const scale = box.width > 0 ? state.bounds.width / box.width : 1;
    return hit((e.clientX - box.left) * scale, (e.clientY - box.top) * scale);
  };

  return (
    <canvas
      ref={canvas}
      className="absolute block"
      style={{ left: bounds.left, top: bounds.top, width: bounds.width, height: bounds.height }}
      role="img"
      aria-label="Spectrum, stereo field and loudness meters"
      onClick={(e) => {
        const found = at(e);
        if (found?.onButton) toggle(found.panel);
      }}
      onDoubleClick={(e) => {
        const found = at(e);
        if (found && !found.onButton) toggle(found.panel);
      }}
      onPointerMove={(e) => {
        const found = at(e);
        e.currentTarget.style.cursor = found?.onButton ? 'pointer' : 'default';
      }}
    />
  );
}
