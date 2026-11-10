// Mirrors Source/ui/meter/MeterText.h, with the same tests
export const NONE = '--.-';

export const hasReading = (value: number, noLoudness: number) => value > noLoudness + 1;

// LUFS or LU to one place
export const loudnessText = (value: number, noLoudness: number) => (hasReading(value, noLoudness) ? value.toFixed(1) : NONE);

// dBTP to one place, with its sign
export const truePeakText = (value: number, noLoudness: number) => {
  if (!hasReading(value, noLoudness)) return NONE;

  const tenths = Math.round(value * 10);
  const text = `${Math.trunc(Math.abs(tenths) / 10)}.${Math.abs(tenths) % 10}`;
  return tenths > 0 ? `+${text}` : tenths < 0 ? `-${text}` : text;
};

export const clipsText = (count: number) => (count > 999 ? '999+' : `${Math.round(count)}`);
