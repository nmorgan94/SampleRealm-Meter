import assert from 'node:assert/strict';
import { test } from 'node:test';
import { formatValue, snap } from '../src/params.ts';
import { info, param } from './engine.ts';

// From the Catch2 snapshot in Tests/ProcessorTests.cpp
const pluginText: [id: string, proportion: number, text: string][] = [
  ['theme', 0, 'Neon'],
  ['theme', 0.5, 'Phosphor'],
  ['theme', 1, 'Ice'],
  ['reactivity', 0, 'Off'],
  ['reactivity', 0.5, 'Subtle'],
  ['reactivity', 1, 'Full'],
  ['view', 0, 'Dashboard'],
  ['view', 0.5, 'Stereo'],
  ['view', 1, 'Loudness'],
];

test('every parameter has a row in pluginText', () => {
  for (const { id } of info.params) assert.ok(pluginText.some(([row]) => row === id), id);
});

test('every value reads as the plugin shows it, at the same knob position', () => {
  for (const [id, proportion, text] of pluginText) {
    assert.equal(formatValue(param(id), snap(param(id), param(id).min + proportion * (param(id).max - param(id).min))), text, `${id} at ${proportion}`);
  }
});
