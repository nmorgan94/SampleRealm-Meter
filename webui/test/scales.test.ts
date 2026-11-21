// The same cases as the scales test in Tests/EditorTests.cpp
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { acrossSpectrum, hzAcross, proportion } from '../src/meter/scales.ts';
import { meter } from './engine.ts';

const near = (actual: number, expected: number, tolerance = 1e-5) => assert.ok(Math.abs(actual - expected) < tolerance, `${actual} is not ${expected}`);

test('values sit on the scales where the plugin puts them', () => {
  const { spectrumBottomDb, spectrumTopDb, levelBottom, levelTop } = meter.scales;

  near(proportion(-45, spectrumBottomDb, spectrumTopDb), 0.5);
  assert.equal(proportion(40, spectrumBottomDb, spectrumTopDb), 1);
  assert.equal(proportion(-200, levelBottom, levelTop), 0);
  near(proportion(-14, levelBottom, levelTop), 0.666667);

  const { minHz, maxHz } = meter.frame;
  near(acrossSpectrum(20, minHz, maxHz), 0);
  near(acrossSpectrum(1000, minHz, maxHz), 0.566323);
  near(acrossSpectrum(20000, minHz, maxHz), 1);

  near(hzAcross(0, minHz, maxHz), 20, 1e-3);
  near(hzAcross(0.5, minHz, maxHz), 632.4555, 1e-2);
  near(hzAcross(1, minHz, maxHz), 20000, 1);
});
