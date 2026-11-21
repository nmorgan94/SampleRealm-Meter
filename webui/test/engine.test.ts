import assert from 'node:assert/strict';
import { test } from 'node:test';
import { maxError, sine } from '@samplerealm/modules/testing/engine.ts';
import { engine, meter } from './engine.ts';

type Played = ReturnType<typeof engine>;

test('the ui info carries the version, the commit, the build type and every parameter as ParameterSpecs defines it', () => {
  const { info } = engine();

  assert.match(info.version, /^\d+\.\d+\.\d+$/);
  assert.match(info.commit, /^([0-9a-f]{7,}(-local)?|unknown)$/);
  assert.match(info.build, /^(Debug|Release)$/);
  assert.deepEqual(info.params, [
    { id: 'theme', name: 'Theme', kind: 'choice', min: 0, max: 5, defaultValue: 0, step: 1, skew: 1, unit: '', choices: ['Neon', 'Ember', 'Phosphor', 'Ultraviolet', 'Ice', 'Custom'] },
    { id: 'reactivity', name: 'Reactivity', kind: 'choice', min: 0, max: 2, defaultValue: 2, step: 1, skew: 1, unit: '', choices: ['Off', 'Subtle', 'Full'] },
    { id: 'view', name: 'View', kind: 'choice', min: 0, max: 4, defaultValue: 0, step: 1, skew: 1, unit: '', choices: ['Dashboard', 'Spectrum', 'Stereo', 'Loudness', 'Waveform'] },
    ...['Spectrum', 'Waveform', 'Stereo', 'Loudness'].map((panel) => ({ id: `show${panel}`, name: `Show ${panel}`, kind: 'toggle', min: 0, max: 1, defaultValue: 1, step: 1, skew: 1, unit: '', choices: [] })),
  ]);
});

// The frame's scalars by name, read after a time
const frameOf = ({ engine: exports }: Played, seconds = 0) => {
  const frame = new Float32Array(exports.memory.buffer, exports.frame(seconds), exports.frameSize());
  return Object.fromEntries(meter.frame.scalars.map((name, index) => [name, frame[index]!]));
};

const near = (actual: number, expected: number, tolerance: number, what: string) =>
  assert.ok(Math.abs(actual - expected) <= tolerance, `${what}: ${actual} is not ${expected}`);

test('the audio comes out exactly as it went in, whatever the parameters', () => {
  const played = engine();
  played.set('theme', 3);
  played.set('view', 2);
  const input = [sine(0.1, 0.5), sine(0.1, 0.25, 220)];

  const output = played.processBlocks(input);

  output.forEach((channel, index) => assert.equal(maxError(channel, input[index]!), 0, `channel ${index}`));
});

test('a stereo 1 kHz sine at -23 dBFS reads -23 LUFS, and its level as true peak', () => {
  const played = engine();
  played.processBlocks([sine(10, 10 ** (-23 / 20), 1000), sine(10, 10 ** (-23 / 20), 1000)]);
  const frame = frameOf(played, 0.016);

  near(frame.momentary!, -23, 0.1, 'momentary');
  near(frame.shortTerm!, -23, 0.1, 'short-term');
  near(frame.integrated!, -23, 0.1, 'integrated');
  near(frame.maxTruePeak!, -23, 0.1, 'true peak');
  near(frame.correlation!, 1, 1e-3, 'correlation');
  assert.equal(frame.clipCount, 0);
});

test('a mono layout meters only the first channel', () => {
  const played = engine(1);
  played.processBlocks([sine(3, 0.5, 1000), sine(3, 1.5, 1000)]);
  const frame = frameOf(played, 0.016);

  near(frame.maxTruePeak!, -6.02, 0.1, 'true peak');
  near(frame.truePeakRight!, frame.truePeakLeft!, 0, 'the right peak');
  assert.equal(frame.clipCount, 0);
});

test('starting the transport starts the integrated loudness again, and a reset starts everything again', () => {
  const played = engine();
  const loud = [sine(3, 1.5, 100), sine(3, 1.5, 100)];
  const quiet = [sine(3, 0.1, 1000), sine(3, 0.1, 1000)];

  // With no transport set, the engine always counts
  played.processBlocks(loud);
  assert.ok(frameOf(played).integrated! > -5);
  assert.equal(frameOf(played).clipCount, 1);

  played.engine.resetMeters();
  played.processBlocks(quiet);
  assert.equal(frameOf(played).clipCount, 0);
  near(frameOf(played).maxTruePeak!, -20, 0.1, 'true peak');
  near(frameOf(played).integrated!, -20, 0.1, 'integrated');

  // A transport that has never played still counts
  played.engine.setTransport(120, false, 0);
  played.processBlocks(quiet);
  assert.equal(frameOf(played).integrating, 1);

  played.engine.setTransport(120, true, 4);
  played.processBlocks(quiet);
  near(frameOf(played).integrated!, -20, 0.1, 'integrated');
  assert.equal(frameOf(played).integrating, 1);

  // Once it has, stopping waits
  played.engine.setTransport(120, false, 8);
  played.processBlocks(quiet);
  assert.equal(frameOf(played).integrating, 0);
});
