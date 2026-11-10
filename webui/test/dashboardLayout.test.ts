// The same cases as the layout tests in Tests/EditorTests.cpp
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { dashboard, ease, expanding } from '../src/meter/dashboardLayout.ts';

const rect = (left: number, top: number, width: number, height: number) => ({ left, top, width, height });

test('the dashboard puts the spectrum over the stereo field and the loudness', () => {
  assert.deepEqual(dashboard(rect(0, 44, 960, 556)), {
    spectrum: rect(10, 54, 940, 305),
    stereo: rect(10, 369, 372, 221),
    loudness: rect(392, 369, 558, 221),
  });
});

test('an expanding panel grows from its place to fill the dashboard', () => {
  const bounds = rect(0, 44, 960, 556);

  assert.deepEqual(expanding(bounds, 'stereo', 0), dashboard(bounds));
  assert.deepEqual(expanding(bounds, 'stereo', 1).stereo, rect(10, 54, 940, 536));
  assert.deepEqual(expanding(bounds, 'stereo', 0.25).stereo, rect(10, 290, 514, 300));

  // The others stay where they were, to fade out
  assert.deepEqual(expanding(bounds, 'stereo', 1).spectrum, dashboard(bounds).spectrum);

  assert.equal(ease(0), 0);
  assert.ok(Math.abs(ease(0.25) - 0.15625) < 1e-9);
  assert.equal(ease(0.5), 0.5);
  assert.equal(ease(1), 1);
});
