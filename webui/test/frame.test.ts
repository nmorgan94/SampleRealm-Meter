import assert from 'node:assert/strict';
import { test } from 'node:test';
import { decodeFrame, frameReader } from '../src/meter/frame.ts';
import { choiceAt, themeVariables, colour } from '../src/meter/theme.ts';
import { engine, info, param } from './engine.ts';

test('the frame is laid out as the engine fills it', () => {
  const reader = frameReader(info.frame);

  assert.equal(reader.size, engine.frameSize());
  assert.equal(reader.bandsOffset, info.frame.scalars.length);
  assert.equal(reader.peaksOffset, reader.bandsOffset + info.frame.numBands);
  assert.equal(reader.pointsOffset, reader.peaksOffset + info.frame.numBands);

  // With nothing played, the engine's frame is the silent one
  engine.prepare(48000, 2);
  const frame = new Float32Array(engine.memory.buffer, engine.frame(0), reader.size);
  assert.deepEqual([...frame], [...reader.silent()]);
  assert.equal(reader.scalar(frame, 'integrated'), info.frame.noLoudness);
});

test('a frame sent as base64 comes back as its floats', () => {
  const sent = new Float32Array([0, -23.5, 1e-3, -100]);
  const base64 = Buffer.from(sent.buffer).toString('base64');

  assert.deepEqual([...decodeFrame(base64)], [...sent]);
});

test('there is a theme for each of the parameter\'s choices, and it sets the editor\'s colours', () => {
  assert.equal(info.themes.length, param('theme').choices.length);
  assert.deepEqual(info.themes[0], { background: '070a14', panel: '0c1120', line: '1d2a44', text: 'e8f4ff', dim: '6f84a8', primary: '19e6ff', secondary: 'ff2bd6', hot: 'ff3d5a' });

  assert.equal(choiceAt(info.themes, 99), info.themes[info.themes.length - 1]);
  assert.equal(choiceAt(info.themes, -1), info.themes[0]);

  assert.equal(colour('19e6ff'), 'rgb(25, 230, 255)');
  assert.equal(colour('19e6ff', 0.5), 'rgba(25, 230, 255, 0.5)');
  assert.equal(themeVariables(info.themes[0])['--color-thumb'], 'rgb(25, 230, 255)');
});
