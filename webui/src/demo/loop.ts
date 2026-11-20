import type { Transport } from '@samplerealm/modules/demo/workletBackend.ts';

// A placeholder for the demo's input: four bars of drum and bass
const BPM = 174;
const BEATS = 16;
// Leaves the true peak about 1 dB under full scale
const PEAK = 0.8;

// How far the limiter turns the loudest peaks down, for about -8 LUFS
const LIMIT_DB = 6;
const RISE_SECONDS = 0.002;
// Longer than the kick's cycle, so the limiter doesn't follow its wave
const FALL_SECONDS = 0.08;

const NOISE_HZ = 9000;

// Each way, between the bass's two sides
const DETUNE = 0.006;

// The bass riff, a bar of each in turn. In beats and semitones over the bar's root; slide is how far above
// its pitch a note starts, and wobbles how many times a beat its filter opens (0 for a stab)
const CALL = [
  { step: 0, beats: 1, semis: 0, slide: 0, wobbles: 3 },
  { step: 1.5, beats: 0.5, semis: 0, slide: 0, wobbles: 0 },
  { step: 2.5, beats: 1, semis: 0, slide: 12, wobbles: 3 },
  { step: 3.5, beats: 0.5, semis: 3, slide: 0, wobbles: 0 },
];
const ANSWER = [
  { step: 0, beats: 0.5, semis: 0, slide: 0, wobbles: 0 },
  { step: 0.75, beats: 0.5, semis: 0, slide: 0, wobbles: 0 },
  { step: 1.5, beats: 0.5, semis: 12, slide: 0, wobbles: 0 },
  { step: 2.5, beats: 1.5, semis: 0, slide: 24, wobbles: 4 },
];

const TAU = 2 * Math.PI;

export const loopTransport = (playing: boolean, seconds: number): Transport => ({ bpm: BPM, playing, ppq: (seconds * BPM) / 60 });

const midiToHz = (note: number) => 440 * 2 ** ((note - 69) / 12);

const saw = (phase: number) => 2 * (phase - Math.floor(phase + 0.5));

// Fades in and out, so a held note doesn't click
const hold = (t: number, seconds: number, fade: number) => Math.min(1, t / fade, (seconds - t) / fade);

export function makeLoop(context: BaseAudioContext): AudioBuffer {
  const rate = context.sampleRate;
  const beat = 60 / BPM;
  const length = Math.round(BEATS * beat * rate);
  const left = new Float32Array(length);
  const right = new Float32Array(length);

  // Two one-pole low-passes in a row
  const lowPass = () => {
    let first = 0;
    let second = 0;
    return (sample: number, hz: number) => {
      const k = 1 - Math.exp((-TAU * hz) / rate);
      first += k * (sample - first);
      second += k * (first - second);
      return second;
    };
  };

  let seed = 1;
  const air = lowPass();
  // Its top is taken off: noise up to half the sample rate peaks well over its samples
  const noise = () => air(((seed = (seed * 1664525 + 1013904223) >>> 0) / 2 ** 32) * 2 - 1, NOISE_HZ) * 1.5;

  let last = 0;
  const hiss = () => {
    const n = noise();
    const high = n - last;
    last = n;
    return high;
  };

  const add = (start: number, seconds: number, pan: number, voice: (t: number) => number) => {
    const from = Math.round(start * rate);
    for (let i = 0; i < seconds * rate; i++) {
      const sample = voice(i / rate);
      // Tails wrap, so the loop has no seam
      const index = (from + i) % length;
      left[index] += sample * (1 - pan);
      right[index] += sample * (1 + pan);
    }
  };

  // A long, overdriven body under a click
  const kick = (seconds: number) =>
    add(seconds, 0.45, 0, (t) => {
      const body = Math.sin(TAU * (42 * t + (170 * (1 - Math.exp(-t * 40))) / 40)) * Math.exp(-t * 7);
      return Math.tanh(2.5 * body) + 0.3 * hiss() * Math.exp(-t * 400);
    });

  // The crack is a burst of bright noise over a body that drops from 420 Hz to 200
  const snare = (at: number, gain: number) => {
    // The overdrive's own top comes off too
    const smooth = lowPass();
    add(at * beat, 0.28, 0.05, (t) => {
      const body = 0.7 * Math.sin(TAU * (200 * t + (220 * (1 - Math.exp(-t * 60))) / 60)) * Math.exp(-t * 30);
      const crack = 0.9 * hiss() * Math.exp(-t * 90);
      return gain * smooth(Math.tanh(2 * (body + crack + 0.6 * noise() * Math.exp(-t * 16))), NOISE_HZ);
    });
  };

  const bars = Array.from({ length: BEATS / 4 }, (_, bar) => bar);
  const kicks = bars.flatMap((bar) => (bar % 2 === 1 ? [0, 2.5, 3.5] : [0, 2.5]).map((step) => (bar * 4 + step) * beat));

  // The bass drops out of each kick's way
  const duck = (seconds: number) => {
    let since = Infinity;
    for (const at of kicks) since = Math.min(since, (seconds - at + BEATS * beat) % (BEATS * beat));
    return 1 - 0.85 * Math.exp(-since * 14);
  };

  // B minor: Bm, Bm, D, A
  const roots = [35, 35, 38, 33];
  const chords = [[59, 62, 66], [59, 62, 66], [62, 66, 69], [57, 61, 64]];

  kicks.forEach(kick);

  for (const bar of bars) {
    const at = bar * 4;

    snare(at + 1, 0.9);
    snare(at + 3, 0.9);
    if (bar % 2 === 1) snare(at + 1.75, 0.25);

    const root = midiToHz(roots[bar]);
    for (const note of bar % 2 === 1 ? ANSWER : CALL) {
      const start = (at + note.step) * beat;
      const seconds = note.beats * beat;
      const pitch = root * 2 ** (note.semis / 12);

      // Cycles of the note so far, as it slides down to its pitch
      const cycles = () => {
        let phase = 0;
        return (t: number) => (phase += (pitch * 2 ** ((note.slide * Math.exp(-t * 18)) / 12)) / rate);
      };
      // A wobble starts closed; a stab starts open and closes
      const cutoff = (t: number) =>
        note.wobbles ? 250 + 2200 * (0.5 - 0.5 * Math.cos((TAU * note.wobbles * t) / beat)) : 300 + 1400 * Math.exp(-t * 6);
      const level = (t: number) => hold(t, seconds, 0.01) * duck(start + t);

      const sub = cycles();
      add(start, seconds, 0, (t) => 0.45 * Math.sin(TAU * sub(t)) * level(t));

      for (const side of [-1, 1]) {
        const filter = lowPass();
        const phase = cycles();
        add(start, seconds, side * 0.4, (t) => {
          const p = phase(t);
          // A saw an octave up over a square, overdriven
          return 0.2 * Math.tanh(4 * filter(saw(2 * (1 + side * DETUNE) * p) + 0.4 * Math.sign(saw(p)), cutoff(t))) * level(t);
        });
      }
    }

    chords[bar].forEach((note, voice) => {
      const f = midiToHz(note);
      const seconds = 4 * beat;
      add(at * beat, seconds, 0.8 * (voice - 1), (t) => 0.05 * (Math.sin(TAU * f * t) + 0.3 * Math.sin(2 * TAU * f * t)) * hold(t, seconds, 0.15));
    });
  }

  for (let step = 0; step < BEATS * 4; step++) {
    const off = step % 2 === 1;
    // The quiet ones come a little late, for shuffle
    add((step + (off ? 0.15 : 0)) * (beat / 4), 0.06, off ? 0.3 : -0.3, (t) => (off ? 0.07 : 0.16) * hiss() * Math.exp(-t * 70));
  }

  // A limiter, not a clipper: the envelope follows the peaks, rising ahead of each one and falling after it,
  // and what is over the threshold is turned down
  const envelope = new Float32Array(length);
  const rise = Math.exp(-1 / (RISE_SECONDS * rate));
  const fall = Math.exp(-1 / (FALL_SECONDS * rate));
  // Twice round, so it joins up across the loop's seam
  for (let i = 0; i < 2 * length; i++) {
    const at = i % length;
    envelope[at] = Math.max(Math.abs(left[at]), Math.abs(right[at]), envelope[(at + length - 1) % length] * fall);
  }
  for (let i = 2 * length - 1; i >= 0; i--) {
    const at = i % length;
    envelope[at] = Math.max(envelope[at], envelope[(at + 1) % length] * rise);
  }

  const threshold = envelope.reduce((max, e) => Math.max(max, e), 0) / 10 ** (LIMIT_DB / 20);
  const buffer = context.createBuffer(2, length, rate);
  [left, right].forEach((channel, i) => buffer.copyToChannel(channel.map((s, at) => (s * PEAK) / Math.max(envelope[at], threshold)), i));
  return buffer;
}
