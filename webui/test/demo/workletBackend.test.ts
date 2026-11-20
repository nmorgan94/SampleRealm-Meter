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

test('a look keeps the custom theme, changing it is an edit, and Init clears it', async () => {
  // As PresetManager writes a look: the custom theme beside the parameters
  const withCustom = `<?xml version="1.0" encoding="UTF-8"?>
<PRESET name="Mine" category="Dashboard" author="Me" formatVersion="1">
  <Parameters>
    <PARAM id="theme" value="5.0"/>
    <CustomTheme base="2" primary="112233" secondary="445566" hot="778899"/>
  </Parameters>
</PRESET>`;

  const backend = meterWorkletBackend(info, meter, [withCustom]);
  const custom = { base: 2, primary: '112233', secondary: '445566', hot: '778899' };
  assert.deepEqual(backend.customTheme(), meter.customTheme);

  const heard: unknown[] = [];
  backend.onCustomTheme((theme) => heard.push(theme));

  const [mine] = await backend.presets.list();
  await backend.presets.load(mine!);
  assert.deepEqual(backend.customTheme(), custom);
  assert.deepEqual(heard, [custom]);
  assert.equal(backend.presets.status().modified, false);

  // The same again is no edit
  backend.setCustomTheme({ ...custom });
  assert.equal(backend.presets.status().modified, false);

  backend.setCustomTheme({ ...custom, base: 0 });
  assert.equal(backend.presets.status().modified, true);

  // Saved in the page's own looks too
  assert.equal(await backend.presets.save('Changed', 'User'), null);
  await backend.presets.loadInit();
  assert.deepEqual(backend.customTheme(), meter.customTheme);

  const changed = (await backend.presets.list()).find((preset) => preset.name === 'Changed');
  await backend.presets.load(changed!);
  assert.deepEqual(backend.customTheme(), { ...custom, base: 0 });
});
