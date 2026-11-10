import assert from 'node:assert/strict';
import { test } from 'node:test';
import { workletBackend, type EnginePort } from '../../src/demo/workletBackend.ts';
import { info } from '../engine.ts';

const fakePort = () => {
  const posted: unknown[] = [];
  const port: EnginePort = { postMessage: (message: unknown) => void posted.push(message), onmessage: null };
  return { port, posted };
};

test('changes made before the worklet attaches reach it once it does, in order and once each', () => {
  const backend = workletBackend(info);
  backend.setValue('theme', 3);
  backend.setValue('theme', 1);

  const { port, posted } = fakePort();
  backend.attach(port);
  backend.setValue('view', 2);

  assert.deepEqual(posted, [
    { type: 'param', index: 0, value: 3 },
    { type: 'param', index: 0, value: 1 },
    { type: 'param', index: 2, value: 2 },
  ]);
});

test('values read back and notify before the worklet attaches', () => {
  const backend = workletBackend(info);
  const heard: number[] = [];
  backend.subscribe('theme', (value) => heard.push(value));

  assert.equal(backend.getValue('theme'), 0);
  backend.setValue('theme', 4);

  assert.equal(backend.getValue('theme'), 4);
  assert.deepEqual(heard, [4]);
});

test('frames from the worklet reach the listeners, and a reset reaches the worklet', () => {
  const backend = workletBackend(info);
  const { port, posted } = fakePort();
  backend.attach(port);

  const heard: Float32Array[] = [];
  const stop = backend.onFrame((frame) => heard.push(frame));
  const frame = new Float32Array([1, 2, 3]);

  port.onmessage?.({ data: { type: 'frame', frame } } as MessageEvent);
  stop();
  port.onmessage?.({ data: { type: 'frame', frame } } as MessageEvent);
  assert.deepEqual(heard, [frame]);

  // The page hears of a reset too, to run the engine for it while paused
  let resets = 0;
  backend.onReset(() => resets++);
  backend.resetMeters();
  assert.deepEqual(posted, [{ type: 'reset' }]);
  assert.equal(resets, 1);
});
