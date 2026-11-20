// juce::Colour makes these natively. 400080 is the colour the custom theme's editor test in Tests/EditorTests.cpp picks
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { hexToHsv, hsvToHex } from '../src/meter/colour.ts';

test('a hue, saturation and brightness make the colour juce::Colour makes', () => {
  assert.equal(hsvToHex({ hue: 0, saturation: 1, value: 1 }), 'ff0000');
  assert.equal(hsvToHex({ hue: 1 / 3, saturation: 1, value: 1 }), '00ff00');
  assert.equal(hsvToHex({ hue: 0.5, saturation: 0.5, value: 1 }), '80ffff');
  assert.equal(hsvToHex({ hue: 0.75, saturation: 1, value: 0.5 }), '400080');

  // One from each sixth of the hues
  const vivid = (hue: number) => hsvToHex({ hue, saturation: 1, value: 1 });
  assert.deepEqual([0.1, 0.25, 0.4, 0.6, 0.7, 0.9].map(vivid), ['ff9900', '80ff00', '00ff66', '0066ff', '3300ff', 'ff0099']);

  // Round to red again, and a grey whatever the hue
  assert.equal(hsvToHex({ hue: 1, saturation: 1, value: 1 }), 'ff0000');
  assert.equal(hsvToHex({ hue: 0.3, saturation: 0, value: 0.5 }), '808080');
});

test('a colour reads back as the hue, saturation and brightness that made it', () => {
  for (const hex of ['19e6ff', 'ff2bd6', 'ff3d5a', '400080', '00ff80']) assert.equal(hsvToHex(hexToHsv(hex)), hex, hex);

  assert.deepEqual(hexToHsv('400080'), { hue: 0.75, saturation: 1, value: 128 / 255 });
  // A grey has no hue, and black no saturation either
  assert.deepEqual(hexToHsv('808080'), { hue: 0, saturation: 0, value: 128 / 255 });
  assert.deepEqual(hexToHsv('000000'), { hue: 0, saturation: 0, value: 0 });
});
