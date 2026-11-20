// Mirrors Source/ui/meter/MeterControls.h: the header's buttons for the meter
import { TextButton } from '@samplerealm/modules/editor/controls.tsx';
import { useParam } from '@samplerealm/modules/editor/hooks.ts';
import { useMenu, type MenuItem } from '@samplerealm/modules/editor/Menu.tsx';
import { formatValue } from '@samplerealm/modules/editor/params.ts';
import type { MeterBackend } from '../backend.ts';
import type { Panel, Shown } from './dashboardLayout.ts';
import type { Rect } from './rect.ts';

const THEME_WIDTH = 104;
const VIEW_WIDTH = 56;
const RESET_WIDTH = 56;
const CAPTURE_WIDTH = 74;
const GAP = 4;

export const CONTROLS_WIDTH = THEME_WIDTH + VIEW_WIDTH + RESET_WIDTH + CAPTURE_WIDTH + 3 * GAP;

// In the order the dashboard lays them out: panelControls in PluginEditor.cpp
const PANEL_CONTROLS: { panel: Panel; name: string }[] = [
  { panel: 'spectrum', name: 'Spectrum' },
  { panel: 'waveform', name: 'Waveform' },
  { panel: 'stereo', name: 'Stereo field' },
  { panel: 'loudness', name: 'Loudness' },
];

// MeterControls::resized
const layOut = ({ left, top, width, height }: Rect) => {
  const at = (x: number, w: number): Rect => ({ left: x, top, width: w, height });
  const capture = at(left + width - CAPTURE_WIDTH, CAPTURE_WIDTH);
  const reset = at(capture.left - GAP - RESET_WIDTH, RESET_WIDTH);
  const view = at(reset.left - GAP - VIEW_WIDTH, VIEW_WIDTH);
  const theme = at(view.left - GAP - THEME_WIDTH, THEME_WIDTH);
  return { theme, view, reset, capture };
};

// AudioPluginAudioProcessorEditor::addChoices: a choice parameter's values, the current one ticked
const choiceItems =(choices: string[], current: number, onSelect: (index: number) => void): MenuItem[] =>
  choices.map((label, index) => ({ kind: 'item', label, ticked: index === current, onSelect: () => onSelect(index) }));

// AudioPluginAudioProcessorEditor::viewMenu: the panels, ticked where the dashboard has them, then the reactivity
export function viewMenu(shown: Shown, setShown: (panel: Panel, on: boolean) => void, reactivity: MenuItem[]): MenuItem[] {
  const numShown = PANEL_CONTROLS.filter(({ panel }) => shown[panel]).length;

  return [
    { kind: 'header', label: 'Panels' },
    // The last panel showing stays
    ...PANEL_CONTROLS.map(({ panel, name }): MenuItem => ({ kind: 'item', label: name, ticked: shown[panel], disabled: shown[panel] && numShown <= 1, onSelect: () => setShown(panel, !shown[panel]) })),
    { kind: 'header', label: 'Reactivity' },
    ...reactivity,
  ];
}

// Which panels the dashboard has, from their parameters
export function useShown(backend: MeterBackend) {
  const spectrum = useParam(backend, 'showSpectrum');
  const waveform = useParam(backend, 'showWaveform');
  const stereo = useParam(backend, 'showStereo');
  const loudness = useParam(backend, 'showLoudness');
  const params = { spectrum, waveform, stereo, loudness };

  const shown: Shown = { spectrum: spectrum.value >= 0.5, waveform: waveform.value >= 0.5, stereo: stereo.value >= 0.5, loudness: loudness.value >= 0.5 };
  return { shown, setShown: (panel: Panel, on: boolean) => params[panel].commit(on ? 1 : 0) };
}

// AudioPluginAudioProcessorEditor::themeMenu: every theme, then the way to the custom theme's editor
export const themeMenu = (choices: string[], current: number, onSelect: (index: number) => void, onEditCustom: () => void): MenuItem[] => [
  ...choiceItems(choices, current, onSelect),
  { kind: 'separator' },
  { kind: 'item', label: 'Edit custom...', onSelect: onEditCustom },
];

interface MeterControlsProps {
  backend: MeterBackend;
  bounds: Rect;
  onCapture(): void;
  onEditCustom(): void;
}

export function MeterControls({ backend, bounds, onCapture, onEditCustom }: MeterControlsProps) {
  const theme = useParam(backend, 'theme');
  const reactivity = useParam(backend, 'reactivity');
  const { shown, setShown } = useShown(backend);
  const menu = useMenu();
  const boxes = layOut(bounds);

  return (
    <>
      <TextButton
        text={formatValue(theme.param, theme.value).toUpperCase()}
        label="Theme"
        style={boxes.theme}
        onClick={(e) => menu.open(e.currentTarget, themeMenu(theme.param.choices, Math.round(theme.value), theme.commit, onEditCustom))}
      />
      <TextButton
        text="VIEW"
        style={boxes.view}
        onClick={(e) => menu.open(e.currentTarget, viewMenu(shown, setShown, choiceItems(reactivity.param.choices, Math.round(reactivity.value), reactivity.commit)))}
      />
      {menu.element}
      <TextButton text="RESET" style={boxes.reset} onClick={() => backend.resetMeters()} />
      <TextButton text="CAPTURE" style={boxes.capture} onClick={onCapture} />
    </>
  );
}
