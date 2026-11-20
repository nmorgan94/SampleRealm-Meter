// Mirrors Source/ui/meter/CustomThemeEditor.cpp: the custom theme's editor, over the meter
import { useState, useSyncExternalStore, type PointerEvent } from 'react';
import { TextButton } from '@samplerealm/modules/editor/controls.tsx';
import { juceHeight } from '@samplerealm/modules/editor/fonts.ts';
import { Modal } from '@samplerealm/modules/editor/Modal.tsx';
import type { CustomTheme, MeterBackend } from '../backend.ts';
import { hexToHsv, hsvToHex, type Hsv } from './colour.ts';
import { customThemeLayout, PANEL_HEIGHT, PANEL_WIDTH, SWATCH_GAP, SWATCH_NAME_HEIGHT } from './customThemeLayout.ts';
import { proportion } from './scales.ts';
import { colour, parseHex } from './theme.ts';

const ACCENTS = [
  { accent: 'primary', name: 'MAIN' },
  { accent: 'secondary', name: 'ACCENT' },
  { accent: 'hot', name: 'ALERT' },
] as const;

type Accent = (typeof ACCENTS)[number]['accent'];

// Red, yellow, green, cyan, blue, magenta and red again
const HUES = 'linear-gradient(to right, #f00, #ff0, #0f0, #0ff, #00f, #f0f, #f00)';

// The custom theme as it is now, through an edit or a look loading
export const useCustomTheme = (backend: MeterBackend) => useSyncExternalStore(backend.onCustomTheme, backend.customTheme);

// A drag that starts in one part of the picker stays with it
const dragging = (pick: (e: PointerEvent<HTMLDivElement>) => void) => ({
  onPointerDown: (e: PointerEvent<HTMLDivElement>) => {
    e.currentTarget.setPointerCapture(e.pointerId);
    pick(e);
  },
  onPointerMove: (e: PointerEvent<HTMLDivElement>) => {
    if (e.currentTarget.hasPointerCapture(e.pointerId)) pick(e);
  },
});

export function CustomThemeEditor({ backend, height, onClose }: { backend: MeterBackend; height: number; onClose(): void }) {
  const custom = useCustomTheme(backend);
  const [selected, setSelected] = useState<Accent>('primary');
  // The picker's own, as a grey or a black has no hue to read back
  const [picked, setPicked] = useState<Hsv>(() => hexToHsv(custom.primary));
  // What's being typed into the hex field
  const [typed, setTyped] = useState<string | null>(null);

  const themeNames = backend.param('theme').choices;
  const numThemes = backend.meter.themes.length;
  const shown = custom[selected];
  // Left alone where it already shows the colour, which keeps the hue of a grey
  const hsv = hsvToHex(picked) === shown ? picked : hexToHsv(shown);

  const panel = { left: 0, top: 0, width: PANEL_WIDTH, height: Math.min(PANEL_HEIGHT, height) };
  const layout = customThemeLayout(panel);

  const set = (change: Partial<CustomTheme>) => backend.setCustomTheme({ ...custom, ...change });

  const pick = (next: Hsv) => {
    setPicked(next);
    set({ [selected]: hsvToHex(next) });
  };

  const pickInSquare = (e: PointerEvent<HTMLDivElement>) => {
    const box = e.currentTarget.getBoundingClientRect();
    pick({ ...hsv, saturation: proportion(e.clientX, box.left, box.right), value: 1 - proportion(e.clientY, box.top, box.bottom) });
  };

  const pickHue = (e: PointerEvent<HTMLDivElement>) => {
    const box = e.currentTarget.getBoundingClientRect();
    pick({ ...hsv, hue: proportion(e.clientX, box.left, box.right) });
  };

  // Anything that isn't a colour is ignored
  const commitTyped = () => {
    const hex = typed === null ? null : parseHex(typed.trim());
    if (hex) set({ [selected]: hex });
    setTyped(null);
  };

  const label = { fontSize: juceHeight(9) };

  return (
    <Modal label="Custom theme" titleInset={{ x: 16, y: 12 }} panel={{ width: panel.width, height: panel.height }} onClose={onClose}>
      <TextButton text="Close" style={layout.close} onClick={onClose} />

      <div className="absolute flex items-center text-fg/60" style={{ ...layout.baseLabel, ...label }}>
        STARTS FROM
      </div>
      {/* To the next built-in theme, and round to the first again */}
      <TextButton text={`BASE: ${themeNames[custom.base].toUpperCase()}`} style={layout.base} onClick={() => set({ base: (custom.base + 1) % numThemes })} />

      <div className="absolute flex items-center text-fg/60" style={{ ...layout.coloursLabel, ...label }}>
        COLOURS
      </div>
      {ACCENTS.map(({ accent, name }, i) => (
        <button key={accent} type="button" aria-pressed={accent === selected} className="absolute cursor-pointer" style={layout.swatches[i]} onClick={() => setSelected(accent)}>
          <div
            className={`absolute inset-x-0 top-0 rounded ${accent === selected ? 'border-2 border-fg' : 'border border-overlay-outline'}`}
            style={{ height: layout.swatches[i].height - SWATCH_NAME_HEIGHT - SWATCH_GAP, background: colour(custom[accent]) }}
          />
          <div className={`absolute inset-x-0 bottom-0 flex items-center justify-center ${accent === selected ? 'text-fg' : 'text-fg/60'}`} style={{ height: SWATCH_NAME_HEIGHT, ...label }}>
            {name}
          </div>
        </button>
      ))}

      <input
        aria-label="Hex colour"
        className="absolute rounded-[3px] border border-input-outline bg-input px-1.5 text-fg outline-none focus:border-input-focus"
        style={{ ...layout.hex, fontSize: juceHeight(12) }}
        spellCheck={false}
        value={typed ?? `#${shown.toUpperCase()}`}
        onChange={(e) => setTyped(e.target.value)}
        onBlur={commitTyped}
        onKeyDown={(e) => {
          if (e.key === 'Enter') commitTyped();
        }}
      />

      {/* The hue at full strength, whitened towards the left and darkened towards the bottom */}
      <div
        className="absolute cursor-crosshair touch-none border border-overlay-outline"
        style={{ ...layout.square, background: `linear-gradient(to bottom, transparent, #000), linear-gradient(to right, #fff, transparent), #${hsvToHex({ hue: hsv.hue, saturation: 1, value: 1 })}` }}
        {...dragging(pickInSquare)}
      >
        <div
          className="pointer-events-none absolute size-2 -translate-1/2 rounded-full border-[1.5px] border-white ring-1 ring-black"
          style={{ left: `${hsv.saturation * 100}%`, top: `${(1 - hsv.value) * 100}%` }}
        />
      </div>
      <div className="absolute cursor-ew-resize touch-none border border-overlay-outline" style={{ ...layout.hue, background: HUES }} {...dragging(pickHue)}>
        <div className="pointer-events-none absolute -top-0.5 -bottom-0.5 w-1 -translate-x-1/2 border-[1.5px] border-white ring-1 ring-black" style={{ left: `${hsv.hue * 100}%` }} />
      </div>
    </Modal>
  );
}
