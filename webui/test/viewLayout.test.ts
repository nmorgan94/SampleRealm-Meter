// The same cases as the view layout tests in Tests/EditorTests.cpp
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { contentBounds, expandButtonBounds, loudnessLayout, spectrumPlot, stereoLayout, waveformColumnX } from '../src/meter/viewLayout.ts';

const rect = (left: number, top: number, width: number, height: number) => ({ left, top, width, height });

test('a panel\'s content is inside its box, under its title, and its expand button is in the corner', () => {
  // MeterPanel::getContentBounds and getExpandButtonBounds, for the dashboard's stereo panel
  assert.deepEqual(contentBounds(rect(10, 369, 372, 221)), rect(22, 397, 348, 181));
  assert.deepEqual(expandButtonBounds(rect(10, 369, 372, 221)), rect(355, 374, 22, 22));
  assert.deepEqual(spectrumPlot(rect(22, 82, 916, 265)), rect(22, 82, 916, 249));
});

test('the waveform\'s newest column is at the right, and they all move left as the next one fills', () => {
  // A plot a pixel wide for each column
  const numColumns = 480;
  const plot = rect(10, 0, numColumns, 100);

  assert.equal(waveformColumnX(plot, numColumns - 1, 0, numColumns), 490);
  assert.equal(waveformColumnX(plot, numColumns - 1, 0.5, numColumns), 489.5);
  assert.equal(waveformColumnX(plot, 0, 0, numColumns), 11);
  assert.equal(waveformColumnX(plot, 0, 1, numColumns), 10);
});

test('the stereo field\'s scope is square, with its bars under it or beside it in a wide panel', () => {
  assert.deepEqual(stereoLayout(rect(12, 28, 300, 400)), {
    scope: rect(12, 49, 300, 300),
    correlation: rect(12, 376, 300, 26),
    balance: rect(12, 402, 300, 26),
  });

  assert.deepEqual(stereoLayout(rect(12, 28, 348, 181)), {
    scope: rect(12, 28, 181, 181),
    correlation: rect(207, 89.5, 153, 26),
    balance: rect(207, 121.5, 153, 26),
  });
});

test('the loudness panel has its bars at the left and its readings in a grid', () => {
  const layout = loudnessLayout(rect(12, 28, 534, 181));

  assert.deepEqual(layout.scale, rect(12, 28, 22, 167));
  assert.deepEqual(layout.bars, [rect(39, 28, 35, 181), rect(79, 28, 35, 181), rect(128, 28, 35, 181), rect(168, 28, 35, 181)]);
  assert.deepEqual(layout.integrated, rect(220, 28, 326, 83));
  assert.deepEqual(layout.cells.momentary, rect(220, 111, 108, 49));
  assert.deepEqual(layout.cells.clips, rect(328, 160, 108, 49));
  assert.equal(layout.compact, false);
});

test('with little room the loudness panel has narrower bars and three readings', () => {
  // In one row at the editor's smallest: one reading over the next
  const narrow = loudnessLayout(rect(12, 28, 182, 116));

  assert.equal(narrow.compact, true);
  assert.deepEqual(narrow.scale, rect(12, 28, 22, 102));
  assert.deepEqual(narrow.bars[0], rect(39, 28, 11, 116));
  assert.deepEqual(narrow.bars[3], rect(96, 28, 11, 116));
  assert.deepEqual(narrow.integrated, rect(120, 28, 74, 38));
  assert.deepEqual(narrow.cells.shortTerm, rect(120, 66, 74, 38));
  assert.deepEqual(narrow.cells.truePeak, rect(120, 104, 74, 38));
  assert.deepEqual(narrow.cells.momentary, rect(0, 0, 0, 0));

  // Short but wide: side by side
  const wide = loudnessLayout(rect(12, 28, 438, 100));

  assert.deepEqual(wide.integrated, rect(154, 28, 98, 100));
  assert.deepEqual(wide.cells.shortTerm, rect(252, 28, 98, 100));
  assert.deepEqual(wide.cells.truePeak, rect(350, 28, 98, 100));
});
