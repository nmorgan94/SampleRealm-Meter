// The shared backend's own tests are in SampleRealm-Modules. This is the meter's layer over it
import assert from 'node:assert/strict';
import { test } from 'node:test';
import type { EnginePort } from '@samplerealm/modules/demo/workletBackend.ts';
import { meterWorkletBackend } from '../../src/demo/workletBackend.ts';
import { info, meter } from '../engine.ts';

const fakePort = () => {
  const posted: unknown[] = [];
  const port: EnginePort = { postMessage: (message: unknown) => void posted.push(message), onmessage: null };
  return { port, posted };
};

test('frames from the worklet reach the listeners, and a reset reaches the worklet', () => {
  const backend = meterWorkletBackend(info, meter);
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
  assert.deepEqual(posted, [{ type: 'call', name: 'resetMeters' }]);
  assert.equal(resets, 1);
});
