import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { test } from 'node:test';
import { localEnginePath } from '../webui/localEngine.ts';
import { readUiInfo, stubImports } from '../webui/src/demo/workletBackend.ts';

const sampleRate = 48000;
const blockSize = 128;

const wasm = new WebAssembly.Module(readFileSync(localEnginePath()));

function engine(numChannels = 2) {
  const { exports } = new WebAssembly.Instance(wasm, stubImports());
  exports._initialize?.();
  exports.prepare(sampleRate, numChannels);

  const ids = readUiInfo(exports).params.map((p) => p.id);
  const set = (id, value) => {
    assert.ok(ids.includes(id), `no parameter ${id}`);
    exports.setParam(ids.indexOf(id), value);
  };

  const view = (channel, length) => new Float32Array(exports.memory.buffer, exports.channel(channel), length);

  // As an AudioWorklet: one block at a time, written in and read back from the engine's channels
  const processBlocks = (inputs) => {
    const outputs = inputs.map((input) => new Float32Array(input.length));
    for (let start = 0; start < inputs[0].length; start += blockSize) {
      const length = Math.min(blockSize, inputs[0].length - start);
      inputs.forEach((input, channel) => view(channel, length).set(input.subarray(start, start + length)));
      exports.render(length);
      outputs.forEach((output, channel) => output.set(view(channel, length), start));
    }
    return outputs;
  };

  return { ...exports, set, view, processBlocks };
}

const sine = (seconds, amplitude, frequency = 440) =>
  Float32Array.from({ length: Math.round(seconds * sampleRate) }, (_, i) => amplitude * Math.sin((2 * Math.PI * frequency * i) / sampleRate));

const maxError = (actual, expected) => Math.max(...actual.map((sample, i) => Math.abs(sample - expected[i])));

test('the ui info carries the version, the commit, the build type and every parameter as ParameterSpecs defines it', () => {
  const info = readUiInfo(engine());

  assert.match(info.version, /^\d+\.\d+\.\d+$/);
  assert.match(info.commit, /^([0-9a-f]{7,}(-local)?|unknown)$/);
  assert.match(info.build, /^(Debug|Release)$/);
  assert.deepEqual(info.params, [
    { id: 'theme', name: 'Theme', kind: 'choice', min: 0, max: 4, defaultValue: 0, step: 1, skew: 1, unit: '', choices: ['Neon', 'Ember', 'Phosphor', 'Ultraviolet', 'Ice'] },
    { id: 'reactivity', name: 'Reactivity', kind: 'choice', min: 0, max: 2, defaultValue: 2, step: 1, skew: 1, unit: '', choices: ['Off', 'Subtle', 'Full'] },
    { id: 'view', name: 'View', kind: 'choice', min: 0, max: 3, defaultValue: 0, step: 1, skew: 1, unit: '', choices: ['Dashboard', 'Spectrum', 'Stereo', 'Loudness'] },
  ]);
});

// The frame's scalars by name, read after a time
const frameOf = (e, seconds = 0) => {
  const { scalars } = readUiInfo(e).frame;
  const frame = new Float32Array(e.memory.buffer, e.frame(seconds), e.frameSize());
  return Object.fromEntries(scalars.map((name, i) => [name, frame[i]]));
};

const near = (actual, expected, tolerance, what) => assert.ok(Math.abs(actual - expected) <= tolerance, `${what}: ${actual} is not ${expected}`);

test('the audio comes out exactly as it went in, whatever the parameters', () => {
  const e = engine();
  e.set('theme', 3);
  e.set('view', 2);
  const input = [sine(0.1, 0.5), sine(0.1, 0.25, 220)];

  const output = e.processBlocks(input);

  output.forEach((channel, i) => assert.equal(maxError(channel, input[i]), 0, `channel ${i}`));
});

test('a stereo 1 kHz sine at -23 dBFS reads -23 LUFS, and its level as true peak', () => {
  const e = engine();
  e.processBlocks([sine(10, 10 ** (-23 / 20), 1000), sine(10, 10 ** (-23 / 20), 1000)]);
  const frame = frameOf(e, 0.016);

  near(frame.momentary, -23, 0.1, 'momentary');
  near(frame.shortTerm, -23, 0.1, 'short-term');
  near(frame.integrated, -23, 0.1, 'integrated');
  near(frame.maxTruePeak, -23, 0.1, 'true peak');
  near(frame.correlation, 1, 1e-3, 'correlation');
  assert.equal(frame.clipCount, 0);
});

test('a mono layout meters only the first channel', () => {
  const e = engine(1);
  e.processBlocks([sine(3, 0.5, 1000), sine(3, 1.5, 1000)]);
  const frame = frameOf(e, 0.016);

  near(frame.maxTruePeak, -6.02, 0.1, 'true peak');
  near(frame.truePeakRight, frame.truePeakLeft, 0, 'the right peak');
  assert.equal(frame.clipCount, 0);
});

test('starting the transport starts the integrated loudness again, and a reset starts everything again', () => {
  const e = engine();
  const loud = [sine(3, 1.5, 100), sine(3, 1.5, 100)];
  const quiet = [sine(3, 0.1, 1000), sine(3, 0.1, 1000)];

  // With no transport set, the engine always counts
  e.processBlocks(loud);
  assert.ok(frameOf(e).integrated > -5);
  assert.equal(frameOf(e).clipCount, 1);

  e.resetMeters();
  e.processBlocks(quiet);
  assert.equal(frameOf(e).clipCount, 0);
  near(frameOf(e).maxTruePeak, -20, 0.1, 'true peak');
  near(frameOf(e).integrated, -20, 0.1, 'integrated');

  e.setTransport(120, false, 0);
  e.processBlocks(quiet);
  assert.equal(frameOf(e).integrating, 0);

  e.setTransport(120, true, 4);
  e.processBlocks(quiet);
  near(frameOf(e).integrated, -20, 0.1, 'integrated');
  assert.equal(frameOf(e).integrating, 1);
});
