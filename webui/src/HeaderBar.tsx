// Mirrors Source/ui/HeaderBar.cpp
import type { ReactNode } from 'react';
import { aboutText, type About } from './about.ts';
import type { PresetBackend } from './backend.ts';
import { FittedText, TextButton, type Rect } from './controls.tsx';
import { juceHeight } from './fonts.ts';
import { centred } from './layout.ts';
import { useMenu } from './Menu.tsx';
import { presetActions } from './presetActions.ts';
import { PresetBar } from './PresetBar.tsx';
import type { Prompt } from './PromptOverlay.tsx';

// HeaderBar::Layout, which each plugin sets
export interface HeaderLayout {
  height: number;
  // Left of the title, right of the controls
  edge: number;
  // A longer title is squeezed to fit
  titleWidth: number;
  titleHeight: number;
  // Either side of the preset bar and menu
  gap: number;
  controlsHeight: number;
  // The plugin's controls, at the right
  trailingWidth: number;
  // Above and below them
  trailingInset: number;
}

// HeaderBar::resized
const layOut = (width: number, layout: HeaderLayout) => {
  const { height, edge, titleWidth, gap, controlsHeight, trailingWidth, trailingInset } = layout;
  const trailing: Rect = { left: width - edge - trailingWidth, top: trailingInset, width: trailingWidth, height: height - 2 * trailingInset };
  const left = edge + titleWidth + gap;
  const controls: Rect = { left, top: centred(height, controlsHeight), width: trailing.left - gap - left, height: controlsHeight };
  return {
    title: { left: edge, top: 0, width: titleWidth, height },
    trailing,
    menu: { ...controls, left: controls.left + controls.width - 28, width: 28 },
    presetBar: { ...controls, width: controls.width - 28 - 4 },
  };
};

interface HeaderBarProps {
  title: string;
  width: number;
  layout: HeaderLayout;
  backend: { info: About; running: string; presets: PresetBackend };
  prompt: Prompt;
  showManual(): void;
  trailing(bounds: Rect): ReactNode;
}

export function HeaderBar({ title, width, layout, backend, prompt, showManual, trailing }: HeaderBarProps) {
  const menu = useMenu();
  const boxes = layOut(width, layout);
  const actions = presetActions(backend.presets, prompt);

  const showMenu = async (button: HTMLElement) => {
    menu.open(button, [
      ...(await actions.menuItems()),
      { kind: 'separator' },
      { kind: 'item', label: 'Manual', onSelect: showManual },
      { kind: 'item', label: 'About', onSelect: () => prompt.showMessage('About', aboutText(backend.info, backend.running)) },
    ]);
  };

  return (
    <>
      <div className="absolute inset-x-0 top-0 bg-header" style={{ height: layout.height }} />
      <h1
        className="absolute flex items-center font-bold whitespace-nowrap text-title"
        style={{ ...boxes.title, fontSize: juceHeight(layout.titleHeight) }}
      >
        <FittedText text={title} width={layout.titleWidth} />
      </h1>
      <PresetBar presets={backend.presets} actions={actions} bounds={boxes.presetBar} />
      <TextButton text="..." style={boxes.menu} onClick={(e) => void showMenu(e.currentTarget)} />
      {menu.element}
      {trailing(boxes.trailing)}
    </>
  );
}
