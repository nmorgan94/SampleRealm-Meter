// Mirrors Source/ui/meter/Glow.h: a neon line is the line itself over wider, fainter strokes of the same colour
export interface Pass {
  width: number;
  alpha: number;
}

// The strokes for a line of a thickness, the widest first. More amount, from 0 to 1, glows more
export const passes = (thickness: number, amount: number): Pass[] => [
  { width: thickness * (4 + 2.5 * amount), alpha: 0.1 + 0.06 * amount },
  { width: thickness * (2.2 + amount), alpha: 0.22 + 0.08 * amount },
  { width: thickness, alpha: 1 },
];

export function strokeGlow(ctx: CanvasRenderingContext2D, path: Path2D, style: string | CanvasGradient, thickness: number, amount: number) {
  ctx.save();
  ctx.strokeStyle = style;
  ctx.lineJoin = 'round';
  ctx.lineCap = 'round';

  for (const pass of passes(thickness, amount)) {
    ctx.globalAlpha = pass.alpha;
    ctx.lineWidth = pass.width;
    ctx.stroke(path);
  }

  ctx.restore();
}
