// The functions of Source/shared/MeterScales.h, with the same tests. Its numbers arrive in the UI info
export const proportion = (value: number, bottom: number, top: number) => Math.min(1, Math.max(0, (value - bottom) / (top - bottom)));

export const acrossSpectrum = (hz: number, minHz: number, maxHz: number) => Math.log(hz / minHz) / Math.log(maxHz / minHz);
