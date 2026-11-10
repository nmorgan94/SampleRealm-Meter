// Mirrors AudioPluginAudioProcessorEditor
import { memo, useEffect, useMemo, useRef, useState, type CSSProperties } from 'react';
import type { PluginBackend } from './backend.ts';
import { TextButton } from './controls.tsx';
import { EditorRootContext } from './editorRoot.ts';
import { juceHeight } from './fonts.ts';
import { HeaderBar, type HeaderLayout } from './HeaderBar.tsx';
import { useParam } from './hooks.ts';
import { useManual } from './ManualOverlay.tsx';
import { Dashboard } from './meter/Dashboard.tsx';
import { PADDING } from './meter/dashboardLayout.ts';
import { CONTROLS_WIDTH, MeterControls } from './meter/MeterControls.tsx';
import { choiceAt, themeVariables, type Look } from './meter/theme.ts';
import { usePrompt } from './PromptOverlay.tsx';

// Source/shared/EditorSize.h's default. The plugin's window can be dragged to other sizes, and the editor fills it
export const EDITOR_WIDTH = 960;
export const EDITOR_HEIGHT = 600;

// AudioPluginAudioProcessorEditor::headerLayout
const HEADER_LAYOUT: HeaderLayout = {
  height: 44,
  edge: 12,
  titleWidth: 176,
  titleHeight: 16,
  gap: 12,
  controlsHeight: 24,
  trailingWidth: CONTROLS_WIDTH,
  trailingInset: 10,
};

// In capture mode, the strip along the bottom with the maker's name
const WORDMARK_HEIGHT = 22;
const EXIT_CAPTURE_MS = 2500;

function useSize(element: HTMLElement | null) {
  const [size, setSize] = useState({ width: 0, height: 0 });

  useEffect(() => {
    if (!element) return;
    const measure = () => setSize({ width: element.clientWidth, height: element.clientHeight });
    measure();
    const observer = new ResizeObserver(measure);
    observer.observe(element);
    return () => observer.disconnect();
  }, [element]);

  return size;
}

export const Editor = memo(function Editor({ backend }: { backend: PluginBackend }) {
  const [root, setRoot] = useState<HTMLDivElement | null>(null);
  const { width, height } = useSize(root);
  const { prompt, overlay } = usePrompt();
  const manual = useManual();

  const theme = useParam(backend, 'theme');
  const reactivity = useParam(backend, 'reactivity');
  const view = useParam(backend, 'view');

  // Clean capture mode: only the meter, with no header or labels
  const [capture, setCapture] = useState(false);
  const [exitShown, setExitShown] = useState(false);
  const exitTimer = useRef<number>();

  useEffect(() => () => clearTimeout(exitTimer.current), []);

  const amounts = backend.info.scales.reactivityAmounts;

  const look = useMemo<Look>(
    () => ({
      theme: choiceAt(backend.info.themes, theme.value),
      reactivity: choiceAt(amounts, reactivity.value),
      capture,
    }),
    [backend, amounts, theme.value, reactivity.value, capture]
  );

  const leaveCapture = () => {
    clearTimeout(exitTimer.current);
    setExitShown(false);
    setCapture(false);
  };

  // Anywhere in the editor, to bring back the way out of capture mode
  const showExit = () => {
    if (!capture) return;
    setExitShown(true);
    clearTimeout(exitTimer.current);
    exitTimer.current = window.setTimeout(() => setExitShown(false), EXIT_CAPTURE_MS);
  };

  // AudioPluginAudioProcessorEditor::resized
  const top = capture ? 0 : HEADER_LAYOUT.height;
  const dashboard = { left: 0, top, width, height: height - top - (capture ? WORDMARK_HEIGHT - PADDING : 0) };

  return (
    <EditorRootContext.Provider value={root}>
      <div ref={setRoot} className="editor relative size-full overflow-hidden bg-background" style={themeVariables(look.theme) as CSSProperties} onPointerMove={showExit}>
        {width > 0 && height > 0 && (
          <>
            <Dashboard backend={backend} look={look} view={Math.round(view.value)} bounds={dashboard} onViewChosen={view.commit} />
            {capture ? (
              <>
                <div
                  className="absolute bottom-0 flex items-center font-bold whitespace-nowrap text-dim"
                  style={{ right: PADDING + 2, height: WORDMARK_HEIGHT, fontSize: juceHeight(11) }}
                >
                  SAMPLEREALM METER
                </div>
                {exitShown && <TextButton text="EXIT CAPTURE" style={{ right: PADDING + 8, top: PADDING + 8, width: 112, height: 24 }} onClick={leaveCapture} />}
              </>
            ) : (
              <HeaderBar
                title="SampleRealm: METER"
                width={width}
                layout={HEADER_LAYOUT}
                backend={backend}
                prompt={prompt}
                showManual={manual.showManual}
                trailing={(bounds) => <MeterControls backend={backend} bounds={bounds} onCapture={() => setCapture(true)} />}
              />
            )}
          </>
        )}
        {overlay}
        {manual.overlay}
      </div>
    </EditorRootContext.Provider>
  );
});
