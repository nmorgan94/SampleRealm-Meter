// The same cases as the custom theme's layout test in Tests/EditorTests.cpp
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { customThemeLayout, PANEL_HEIGHT, PANEL_WIDTH } from '../src/meter/customThemeLayout.ts';

const rect = (left: number, top: number, width: number, height: number) => ({ left, top, width, height });

test('the custom theme\'s editor has its controls at the left and the picker at the right', () => {
  assert.deepEqual(customThemeLayout(rect(120, 16, PANEL_WIDTH, PANEL_HEIGHT)), {
    title: rect(136, 28, 458, 24),
    close: rect(594, 28, 70, 24),
    baseLabel: rect(136, 60, 250, 12),
    base: rect(136, 72, 250, 24),
    coloursLabel: rect(136, 104, 250, 12),
    swatches: [rect(136, 116, 78, 44), rect(222, 116, 78, 44), rect(308, 116, 78, 44)],
    hex: rect(136, 168, 250, 24),
    square: rect(406, 60, 258, 108),
    hue: rect(406, 176, 258, 16),
  });
});
