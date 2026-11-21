// The same cases as the readings test in Tests/EditorTests.cpp
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { clipsText, frequencyText, levelText, loudnessText, truePeakText } from '../src/meter/text.ts';
import { meter } from './engine.ts';

const { noLoudness } = meter.frame;

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

  assert.equal(frequencyText(440.4), '440 Hz');
  assert.equal(frequencyText(999.7), '1.00 kHz');
  assert.equal(frequencyText(1250), '1.25 kHz');
  assert.equal(frequencyText(12540), '12.5 kHz');
  assert.equal(levelText(-32.44), '-32.4 dB');

  assert.equal(clipsText(0), '0');
  assert.equal(clipsText(37), '37');
  assert.equal(clipsText(1000), '999+');
});
