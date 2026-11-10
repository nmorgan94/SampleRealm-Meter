// The same cases as the readings test in Tests/EditorTests.cpp
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { clipsText, loudnessText, truePeakText } from '../src/meter/text.ts';
import { info } from './engine.ts';

const { noLoudness } = info.frame;

test('readings are written to one place, or as dashes with nothing to show', () => {
  assert.equal(loudnessText(-23.04, noLoudness), '-23.0');
  assert.equal(loudnessText(-8.96, noLoudness), '-9.0');
  assert.equal(loudnessText(4.2, noLoudness), '4.2');
  assert.equal(loudnessText(noLoudness, noLoudness), '--.-');

  assert.equal(truePeakText(-1.23, noLoudness), '-1.2');
  assert.equal(truePeakText(0.36, noLoudness), '+0.4');
  assert.equal(truePeakText(-0.04, noLoudness), '0.0');
  assert.equal(truePeakText(12, noLoudness), '+12.0');
  assert.equal(truePeakText(noLoudness, noLoudness), '--.-');

  assert.equal(clipsText(0), '0');
  assert.equal(clipsText(37), '37');
  assert.equal(clipsText(1000), '999+');
});
