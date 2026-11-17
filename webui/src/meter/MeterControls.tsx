// Mirrors Source/ui/meter/MeterControls.h: the header's buttons for the meter
import { TextButton } from '@samplerealm/modules/editor/controls.tsx';
import { useParam } from '@samplerealm/modules/editor/hooks.ts';
import { formatValue } from '@samplerealm/modules/editor/params.ts';
import type { MeterBackend } from '../backend.ts';
import type { Rect } from './rect.ts';

const THEME_WIDTH = 104;
const REACTIVITY_WIDTH = 84;
const RESET_WIDTH = 56;
const CAPTURE_WIDTH = 74;
const GAP = 4;

export const CONTROLS_WIDTH = THEME_WIDTH + REACTIVITY_WIDTH + RESET_WIDTH + CAPTURE_WIDTH + 3 * GAP;

// MeterControls::resized
const layOut = ({ left, top, width, height }: Rect) => {
  const at = (x: number, w: number): Rect => ({ left: x, top, width: w, height });
  const capture = at(left + width - CAPTURE_WIDTH, CAPTURE_WIDTH);
  const reset = at(capture.left - GAP - RESET_WIDTH, RESET_WIDTH);
  const reactivity = at(reset.left - GAP - REACTIVITY_WIDTH, REACTIVITY_WIDTH);
  const theme = at(reactivity.left - GAP - THEME_WIDTH, THEME_WIDTH);
  return { theme, reactivity, reset, capture };
};

// A choice that steps to the next on each click, and round to the first again
function useStepping(backend: MeterBackend, id: string) {
  const { value, commit, param } = useParam(backend, id);
  return { text: formatValue(param, value).toUpperCase(), step: () => commit((Math.round(value) + 1) % param.choices.length) };
}

export function MeterControls({ backend, bounds, onCapture }: { backend: MeterBackend; bounds: Rect; onCapture(): void }) {
  const theme = useStepping(backend, 'theme');
  const reactivity = useStepping(backend, 'reactivity');
  const boxes = layOut(bounds);

  return (
    <>
      <TextButton text={theme.text} label="Theme" style={boxes.theme} onClick={theme.step} />
      <TextButton text={`FX ${reactivity.text}`} label="Reactivity" style={boxes.reactivity} onClick={reactivity.step} />
      <TextButton text="RESET" style={boxes.reset} onClick={() => backend.resetMeters()} />
      <TextButton text="CAPTURE" style={boxes.capture} onClick={onCapture} />
    </>
  );
}
