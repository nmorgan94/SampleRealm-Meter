// The same cases as the layout tests in Tests/EditorTests.cpp
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { ALL_SHOWN, dashboard, ease, expanding, ROW_BELOW_HEIGHT, type Shown } from '../src/meter/dashboardLayout.ts';

const rect = (left: number, top: number, width: number, height: number) => ({ left, top, width, height });
const empty = rect(0, 0, 0, 0);

const noWaveform: Shown = { ...ALL_SHOWN, waveform: false };
const hidden: Shown = { spectrum: false, stereo: false, loudness: false, waveform: false };

test('the dashboard puts the spectrum over the waveform, over the stereo field and the loudness', () => {
  assert.deepEqual(dashboard(rect(0, 44, 960, 556)), {
    spectrum: rect(10, 54, 940, 239),
    waveform: rect(10, 303, 940, 103),
    stereo: rect(10, 416, 372, 174),
    loudness: rect(392, 416, 558, 174),
  });
});

test('the panels that are shown fill the dashboard', () => {
  const bounds = rect(0, 44, 960, 556);

  assert.deepEqual(dashboard(bounds, noWaveform), {
    spectrum: rect(10, 54, 940, 305),
    stereo: rect(10, 369, 372, 221),
    loudness: rect(392, 369, 558, 221),
    waveform: empty,
  });

  assert.deepEqual(dashboard(bounds, { ...hidden, spectrum: true, waveform: true }), {
    spectrum: rect(10, 54, 940, 367),
    waveform: rect(10, 431, 940, 159),
    stereo: empty,
    loudness: empty,
  });

  assert.deepEqual(dashboard(bounds, { ...hidden, loudness: true }).loudness, rect(10, 54, 940, 536));
});

test('a short dashboard is one row', () => {
  // The editor at its smallest
  const bounds = rect(0, 44, 800, 176);

  assert.deepEqual(dashboard(bounds), {
    spectrum: rect(10, 54, 204, 156),
    waveform: rect(224, 54, 204, 156),
    stereo: rect(438, 54, 136, 156),
    loudness: rect(584, 54, 206, 156),
  });

  const two = dashboard(bounds, { ...hidden, stereo: true, loudness: true });
  assert.deepEqual(two.stereo, rect(10, 54, 308, 156));
  assert.deepEqual(two.loudness, rect(328, 54, 462, 156));

  // Two rows from the height where the lower one has room
  assert.equal(dashboard(rect(0, 44, 800, ROW_BELOW_HEIGHT - 1)).spectrum.width, 204);
  assert.equal(dashboard(rect(0, 44, 800, ROW_BELOW_HEIGHT)).spectrum.width, 780);
});

test('an expanding panel grows from its place to fill the dashboard', () => {
  const bounds = rect(0, 44, 960, 556);

  assert.deepEqual(expanding(bounds, noWaveform, 'stereo', 0), dashboard(bounds, noWaveform));
  assert.deepEqual(expanding(bounds, noWaveform, 'stereo', 1).stereo, rect(10, 54, 940, 536));
  assert.deepEqual(expanding(bounds, noWaveform, 'stereo', 0.25).stereo, rect(10, 290, 514, 300));

  // The others stay where they were, to fade out
  assert.deepEqual(expanding(bounds, noWaveform, 'stereo', 1).spectrum, dashboard(bounds, noWaveform).spectrum);

  // One the dashboard doesn't show grows from the middle
  assert.deepEqual(expanding(bounds, noWaveform, 'waveform', 0.5).waveform, rect(245, 188, 470, 268));

  assert.equal(ease(0), 0);
  assert.ok(Math.abs(ease(0.25) - 0.15625) < 1e-9);
  assert.equal(ease(0.5), 0.5);
  assert.equal(ease(1), 1);
});
