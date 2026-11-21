// The same cases as the layout tests in Tests/EditorTests.cpp
import assert from 'node:assert/strict';
import { test } from 'node:test';
import {
  ALL_SHOWN,
  dashboard,
  defaultSizes,
  dividers,
  dragged,
  ease,
  expanding,
  MIN_PANEL_HEIGHT,
  MIN_PANEL_WIDTH,
  reset,
  ROW_BELOW_HEIGHT,
  sanitiseSizes,
  type Shown,
} from '../src/meter/dashboardLayout.ts';

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

test("the gaps between the dashboard's panels are its dividers", () => {
  const bounds = rect(0, 44, 960, 556);

  assert.deepEqual(dividers(bounds), [
    { group: 'rows', before: 0, after: 1, across: false, area: rect(10, 293, 940, 10) },
    { group: 'rows', before: 1, after: 2, across: false, area: rect(10, 406, 940, 10) },
    { group: 'lower', before: 0, after: 1, across: true, area: rect(382, 416, 10, 174) },
  ]);

  // Between the neighbours that are shown
  assert.deepEqual(dividers(bounds, noWaveform), [
    { group: 'rows', before: 0, after: 2, across: false, area: rect(10, 359, 940, 10) },
    { group: 'lower', before: 0, after: 1, across: true, area: rect(382, 369, 10, 221) },
  ]);

  assert.deepEqual(dividers(rect(0, 44, 800, 176)), [
    { group: 'oneRow', before: 0, after: 1, across: true, area: rect(214, 54, 10, 156) },
    { group: 'oneRow', before: 1, after: 2, across: true, area: rect(428, 54, 10, 156) },
    { group: 'oneRow', before: 2, after: 3, across: true, area: rect(574, 54, 10, 156) },
  ]);

  assert.deepEqual(dividers(bounds, { ...hidden, loudness: true }), []);
});

test("dragging a divider shares its two neighbours' space again, and moves no other panel", () => {
  const bounds = rect(0, 44, 960, 556);

  assert.deepEqual(dashboard(bounds, ALL_SHOWN, dragged(bounds, ALL_SHOWN, defaultSizes(), dividers(bounds)[0], 30)), {
    spectrum: rect(10, 54, 940, 269),
    waveform: rect(10, 333, 940, 73),
    stereo: rect(10, 416, 372, 174),
    loudness: rect(392, 416, 558, 174),
  });
});

test('a drag stops at the smallest a panel can be', () => {
  const bounds = rect(0, 44, 960, 556);
  const [underSpectrum, , betweenLower] = dividers(bounds);
  const after = (divider: typeof underSpectrum, pixels: number) => dashboard(bounds, ALL_SHOWN, dragged(bounds, ALL_SHOWN, defaultSizes(), divider, pixels));

  assert.deepEqual(after(underSpectrum, 500).waveform, rect(10, 342, 940, MIN_PANEL_HEIGHT));
  assert.deepEqual(after(underSpectrum, -500).spectrum, rect(10, 54, 940, MIN_PANEL_HEIGHT));
  assert.deepEqual(after(betweenLower, 500).stereo, rect(10, 416, 810, 174));
  assert.deepEqual(after(betweenLower, 500).loudness, rect(830, 416, MIN_PANEL_WIDTH, 174));
});

test("a panel that isn't showing keeps its share through a drag, and a reset puts one group back", () => {
  const bounds = rect(0, 44, 960, 556);

  const withoutWaveform = dragged(bounds, noWaveform, defaultSizes(), dividers(bounds, noWaveform)[0], -5);
  assert.equal(dashboard(bounds, noWaveform, withoutWaveform).spectrum.height, 300);
  assert.equal(dashboard(bounds, ALL_SHOWN, withoutWaveform).waveform.height, 103);

  const lowerDragged = dragged(bounds, ALL_SHOWN, defaultSizes(), dividers(bounds)[2], 500);
  const both = dragged(bounds, ALL_SHOWN, lowerDragged, dividers(bounds)[0], 30);
  assert.deepEqual(reset(both, 'rows'), lowerDragged);
  assert.deepEqual(reset(lowerDragged, 'lower'), defaultSizes());
});

test("panel sizes that aren't valid keep their defaults", () => {
  // The same case as the state test in Tests/ProcessorTests.cpp, as a look's XML has them
  assert.deepEqual(sanitiseSizes({ rows: '1 2', lower: '3 0', oneRow: '4 3 2 1' }), { ...defaultSizes(), oneRow: [4, 3, 2, 1] });
  assert.deepEqual(sanitiseSizes(undefined), defaultSizes());
});

test('panel sizes edited by hand read as the plugin reads them', () => {
  assert.deepEqual(sanitiseSizes({ rows: ' 300  131.5 226 ', lower: '40abc 60' }), { ...defaultSizes(), rows: [300, 131.5, 226] });
});

test('a drag that moves nothing leaves the sizes as they were', () => {
  const bounds = rect(0, 44, 960, 556);

  assert.deepEqual(dragged(bounds, ALL_SHOWN, defaultSizes(), dividers(bounds)[0], 0), defaultSizes());
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
