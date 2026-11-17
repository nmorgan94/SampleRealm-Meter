import { useCallback, useEffect, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import { DemoFrame } from '@samplerealm/modules/demo/DemoFrame.tsx';
import { instantiateEngine, loadEngine, readJson, startWorklet } from '@samplerealm/modules/demo/engine.ts';
import processorUrl from '@samplerealm/modules/demo/processor.js?url';
import { juceHeight } from '@samplerealm/modules/editor/fonts.ts';
import type { MeterInfo } from '../backend.ts';
import { EDITOR_HEIGHT, EDITOR_WIDTH, Editor } from '../Editor.tsx';
import { factoryPresets } from './factoryPresets.ts';
import { loopTransport, makeLoop } from './loop.ts';
import { FRAMES, meterWorkletBackend, type MeterWorkletBackend } from './workletBackend.ts';
import './styles.css';

const ENGINE_URL = 'engine.wasm';

// As the plugin's default stereo layout
const CHANNELS = 2;

interface Audio {
  context: AudioContext;
  node: AudioWorkletNode;
  loop: AudioBuffer;
}

async function startAudio(backend: MeterWorkletBackend, wasm: ArrayBuffer, context: AudioContext): Promise<Audio> {
  const node = await startWorklet(context, processorUrl, {
    numberOfInputs: 1,
    outputChannelCount: [CHANNELS],
    channelCount: CHANNELS,
    channelCountMode: 'explicit',
    processorOptions: { wasm, channels: CHANNELS, frames: FRAMES },
  });

  node.connect(context.destination);
  backend.attach(node.port);
  return { context, node, loop: makeLoop(context) };
}

// How long the engine keeps running after Pause, so the meter falls away rather than freezing. The last to go
// are the short-term loudness, 3 s and a block, and the spectrum's 78 dB at 24 dB a second, 3.25 s
const SETTLE_MS = 3500;

// The loop runs through the engine, so Pause also lets the meter fall
function usePlayer(audio: Audio | null, backend: MeterWorkletBackend | undefined) {
  const [playing, setPlaying] = useState(false);
  const source = useRef<AudioBufferSourceNode | null>(null);
  // Seconds played, counting every time round the loop, as a host's position does
  const offset = useRef(0);
  const startedAt = useRef(0);
  const settling = useRef<number>();

  useEffect(() => () => clearTimeout(settling.current), []);

  // Runs the engine for long enough to settle, then stops it
  const settle = useCallback(() => {
    if (!audio) return;
    clearTimeout(settling.current);
    void audio.context.resume();
    settling.current = window.setTimeout(() => void audio.context.suspend(), SETTLE_MS);
  }, [audio]);

  // A reset while paused needs the engine running to make it and show it
  useEffect(() => {
    if (!backend) return;
    return backend.onReset(() => {
      if (!source.current) settle();
    });
  }, [backend, settle]);

  const play = useCallback(() => {
    if (!audio || !backend || source.current) return;
    const { context, node, loop } = audio;
    // Before resuming, so the engine has it for the first block
    backend.setTransport(loopTransport(true, offset.current));
    clearTimeout(settling.current);
    context.resume();
    const next = new AudioBufferSourceNode(context, { buffer: loop, loop: true });
    next.connect(node);
    next.start(0, offset.current % loop.duration);
    startedAt.current = context.currentTime - offset.current;
    source.current = next;
    setPlaying(true);
  }, [audio, backend]);

  const pause = useCallback(() => {
    if (!audio || !backend || !source.current) return;
    offset.current = audio.context.currentTime - startedAt.current;
    source.current.stop();
    source.current.disconnect();
    source.current = null;
    settle();
    backend.setTransport(loopTransport(false, offset.current));
    setPlaying(false);
  }, [audio, backend, settle]);

  return { playing, toggle: playing ? pause : play };
}

function Demo() {
  const [engine, setEngine] = useState<{ backend: MeterWorkletBackend; wasm: ArrayBuffer } | null>(null);
  const [audio, setAudio] = useState<Audio | null>(null);
  const [failed, setFailed] = useState(false);
  const context = useRef<AudioContext | null>(null);
  const player = usePlayer(audio, engine?.backend);

  useEffect(() => {
    loadEngine(ENGINE_URL)
      .then(({ wasm, info, module }) => {
        const infoEngine = instantiateEngine(module);
        const meter = readJson<MeterInfo>(infoEngine, infoEngine.meterInfo());
        setEngine({ backend: meterWorkletBackend(info, meter, factoryPresets), wasm });
      })
      .catch((error) => {
        console.error('Demo failed to load:', error);
        setFailed(true);
      });
  }, []);

  // Ready to play at once; the loop waits for Play
  useEffect(() => {
    if (!engine || context.current) return;

    const audioContext = new AudioContext();
    // It starts running where autoplay is allowed
    audioContext.suspend();
    context.current = audioContext;

    startAudio(engine.backend, engine.wasm, audioContext)
      .then(setAudio)
      .catch((error) => {
        console.error('Demo audio failed to start:', error);
        audioContext.close();
        setFailed(true);
      });
  }, [engine]);

  useEffect(() => () => void context.current?.close(), []);

  return (
    <DemoFrame
      width={EDITOR_WIDTH}
      height={EDITOR_HEIGHT}
      status={failed ? 'failed' : audio ? 'ready' : 'loading'}
      editor={engine && <Editor backend={engine.backend} />}
    >
      <button
        type="button"
        className="mt-2.5 h-7 w-20 cursor-pointer border border-line bg-background text-fg disabled:cursor-default disabled:opacity-50"
        style={{ fontSize: juceHeight(15) }}
        disabled={!audio}
        aria-pressed={player.playing}
        onClick={player.toggle}
      >
        {player.playing ? 'PAUSE' : 'PLAY'}
      </button>
    </DemoFrame>
  );
}

const root = document.getElementById('root');
if (root) createRoot(root).render(<Demo />);
