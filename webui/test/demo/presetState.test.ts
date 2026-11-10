// This plugin's presets in the demo: its factory files and its state
import assert from 'node:assert/strict';
import { readdirSync, readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { test } from 'node:test';
import { STORAGE_KEY } from '../../src/demo/presetState.ts';
import { workletBackend } from '../../src/demo/workletBackend.ts';
import { info } from '../engine.ts';

const presetFolder = resolve(import.meta.dirname, '../../../Assets/Presets');
const factory = readdirSync(presetFolder)
  .filter((file) => file.endsWith('.srmeter'))
  .map((file) => readFileSync(resolve(presetFolder, file), 'utf8'));

// The same order as Tests/PresetTests.cpp
const FACTORY_NAMES = ['Ember Pulse', 'Ice Subtle', 'Phosphor Calm', 'Ember Loudness', 'Phosphor Scope', 'Ultraviolet Spectrum'];

const storageWith = (stored: unknown) => {
  const items = new Map([[STORAGE_KEY, JSON.stringify(stored)]]);
  return { getItem: (key: string) => items.get(key) ?? null, setItem: (key: string, value: string) => void items.set(key, value) };
};

test('factory presets load into the engine in the plugin\'s order', async () => {
  const { presets, getValue, param } = workletBackend(info, factory);
  const list = await presets.list();
  assert.deepEqual(list.map((preset) => preset.name), FACTORY_NAMES);

  await presets.load(list.find((preset) => preset.name === 'Ultraviolet Spectrum')!);
  assert.equal(getValue('theme'), 3);
  assert.equal(getValue('reactivity'), 2);
  assert.equal(getValue('view'), 1);

  await presets.loadInit();
  assert.equal(getValue('theme'), param('theme').defaultValue);
  assert.equal(getValue('view'), param('view').defaultValue);
});

test('a stored preset loads past ids this engine doesn\'t have', async () => {
  const { presets, getValue } = workletBackend(info, factory, storageWith([{ name: 'Other', category: '', values: { cutoff: 1, theme: 2 } }]));

  await presets.load({ name: 'Other', category: '', isFactory: false });

  assert.equal(getValue('theme'), 2);
  assert.equal(presets.status().current.name, 'Other');
});
