import assert from 'node:assert/strict';
import { test } from 'node:test';
import { decodeFrame, frameReader } from '../src/meter/frame.ts';
import { choiceAt, colour, parseHex, sanitiseCustomTheme, themeOf, themeVariables, withBackdrop } from '../src/meter/theme.ts';
import { engine, meter, param } from './engine.ts';

test('the frame is laid out as the engine fills it', () => {
  const reader = frameReader(meter.frame);
  const { engine: exports } = engine();

  assert.equal(reader.size, exports.frameSize());
  assert.equal(reader.bandsOffset, meter.frame.scalars.length);
  assert.equal(reader.peaksOffset, reader.bandsOffset + meter.frame.numBands);
  assert.equal(reader.pointsOffset, reader.peaksOffset + meter.frame.numBands);
  assert.equal(reader.waveformOffset, reader.pointsOffset + 2 * meter.frame.numPoints);

  // With nothing played, the engine's frame is the silent one
  const frame = new Float32Array(exports.memory.buffer, exports.frame(0), reader.size);
  assert.deepEqual([...frame], [...reader.silent()]);
  assert.equal(reader.scalar(frame, 'integrated'), meter.frame.noLoudness);
});

test('a frame sent as base64 comes back as its floats', () => {
  const sent = new Float32Array([0, -23.5, 1e-3, -100]);
  const base64 = Buffer.from(sent.buffer).toString('base64');

  assert.deepEqual([...decodeFrame(base64)], [...sent]);
});

test('a theme is a built-in palette, or the custom one\'s base with its accents', () => {
  // The same cases as Tests/EditorTests.cpp
  const { themes } = meter;
  const custom = { base: 1, primary: '112233', secondary: '445566', hot: '778899' };

  assert.equal(themeOf(themes, 1, custom), themes[1]);
  assert.deepEqual(themeOf(themes, themes.length, custom), { ...themes[1], primary: '112233', secondary: '445566', hot: '778899' });

  // Before the user has changed it, the first theme
  assert.deepEqual(themeOf(themes, themes.length, meter.customTheme), themes[0]);

  // Over the green backdrop, and over none
  assert.deepEqual(withBackdrop(themes[1], meter.backdrops, 2), { ...themes[1], background: '00ff00', panel: '00ff00' });
  assert.equal(withBackdrop(themes[1], meter.backdrops, 0), themes[1]);
  assert.equal(meter.backdrops.length, param('backdrop').choices.length - 1);

  assert.equal(parseHex('19e6ff'), '19e6ff');
  assert.equal(parseHex('#FF2BD6'), 'ff2bd6');
  assert.equal(parseHex('12345'), null);
  assert.equal(parseHex('zzzzzz'), null);

  // Anything missing or that isn't a colour keeps its default: the same case as Tests/ProcessorTests.cpp
  assert.deepEqual(sanitiseCustomTheme({ base: 99, primary: 'not a colour', hot: '00FF80' }, meter.customTheme, themes.length), {
    ...meter.customTheme,
    base: themes.length - 1,
    hot: '00ff80',
  });
});

test('there is a theme for each of the parameter\'s choices but the custom one, and it sets the editor\'s colours', () => {
  const { themes } = meter;

  assert.equal(themes.length, param('theme').choices.length - 1);
  assert.deepEqual(themes[0], { background: '070a14', panel: '0c1120', line: '1d2a44', text: 'e8f4ff', dim: '6f84a8', primary: '19e6ff', secondary: 'ff2bd6', hot: 'ff3d5a' });

  assert.equal(choiceAt(themes, 99), themes[themes.length - 1]);
  assert.equal(choiceAt(themes, -1), themes[0]);

  assert.equal(colour('19e6ff'), 'rgb(25, 230, 255)');
  assert.equal(colour('19e6ff', 0.5), 'rgba(25, 230, 255, 0.5)');
  assert.equal(themeVariables(themes[0])['--color-thumb'], 'rgb(25, 230, 255)');
});
