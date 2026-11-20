// juce::Colour's hue, saturation and brightness, which the custom theme's picker works in. Each is from 0 to 1
export interface Hsv {
  hue: number;
  saturation: number;
  value: number;
}

// juce::roundToInt: a half goes to the even number
const round = (x: number) => {
  const nearest = Math.round(x);
  return Math.abs(x - Math.trunc(x)) === 0.5 && nearest % 2 !== 0 ? nearest - 1 : nearest;
};

const hexOf = (red: number, green: number, blue: number) => [red, green, blue].map((part) => part.toString(16).padStart(2, '0')).join('');

// juce::Colour::fromHSV, as "rrggbb"
export function hsvToHex({ hue, saturation, value }: Hsv): string {
  const v = Math.min(255, Math.max(0, value * 255));
  const full = round(v);
  if (saturation <= 0) return hexOf(full, full, full);

  const s = Math.min(1, saturation);
  const h = (hue - Math.floor(hue)) * 6;
  const f = h - Math.floor(h);
  const low = round(v * (1 - s));
  const rising = round(v * (1 - s * (1 - f)));
  const falling = round(v * (1 - s * f));

  if (h < 1) return hexOf(full, rising, low);
  if (h < 2) return hexOf(falling, full, low);
  if (h < 3) return hexOf(low, full, rising);
  if (h < 4) return hexOf(low, falling, full);
  if (h < 5) return hexOf(rising, low, full);
  return hexOf(full, low, falling);
}

// juce::Colour::getHSB. A grey has no hue and a black no saturation, and both read 0
export function hexToHsv(hex: string): Hsv {
  const colour = parseInt(hex, 16);
  const red = (colour >> 16) & 255;
  const green = (colour >> 8) & 255;
  const blue = colour & 255;
  const high = Math.max(red, green, blue);
  const low = Math.min(red, green, blue);

  if (high === 0) return { hue: 0, saturation: 0, value: 0 };

  const saturation = (high - low) / high;
  let hue = 0;

  if (saturation > 0) {
    const r = (high - red) / (high - low);
    const g = (high - green) / (high - low);
    const b = (high - blue) / (high - low);
    hue = (red === high ? b - g : green === high ? 2 + r - b : 4 + g - r) / 6;
    if (hue < 0) hue += 1;
  }

  return { hue, saturation, value: high / 255 };
}
