// This plugin's factory preset files in the demo
import assert from 'node:assert/strict';
import { readdirSync, readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { test } from 'node:test';
import { workletBackend } from '@samplerealm/modules/demo/workletBackend.ts';
import { info } from '../engine.ts';

const presetFolder = resolve(import.meta.dirname, '../../../Assets/Presets');
const factoryPresets = readdirSync(presetFolder)
  .filter((file) => file.endsWith('.srmeter'))
  .map((file) => readFileSync(resolve(presetFolder, file), 'utf8'));

// The same order as Tests/PresetTests.cpp
const FACTORY_NAMES = ['Ember Pulse', 'Ice Subtle', 'Phosphor Calm', 'Ember Loudness', 'Phosphor Scope', 'Ultraviolet Spectrum'];

test('factory presets load into the engine in the plugin\'s order', async () => {
  const { presets, getValue, param } = workletBackend(info, { storageKey: 'test-presets', factoryPresets });
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
