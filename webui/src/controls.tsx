// Mirrors juce::TextButton and how LookAndFeel_V4 draws it
import { useLayoutEffect, useRef, useState, type CSSProperties, type MouseEvent as ReactMouseEvent } from 'react';
import { juceHeight } from './fonts.ts';
import type { Rect } from './layout.ts';

export type { Rect };

const MIN_SQUEEZE = 0.7;

// As drawFittedText on one line: squeezed to fit, as far as 70% wide, then cut short with "…"
export function FittedText({ text, width }: { text: string; width: number }) {
  const span = useRef<HTMLSpanElement>(null);
  const [squeeze, setSqueeze] = useState(1);

  useLayoutEffect(() => {
    const measure = () => {
      const natural = span.current?.scrollWidth ?? 0;
      setSqueeze(natural > width ? Math.max(MIN_SQUEEZE, width / natural) : 1);
    };
    measure();
    document.fonts?.ready.then(measure);
  }, [text, width]);

  return (
    <span
      ref={span}
      className="inline-block shrink-0 origin-left overflow-hidden text-ellipsis whitespace-nowrap"
      style={{ maxWidth: width / squeeze, transform: `scaleX(${squeeze})` }}
    >
      {text}
    </span>
  );
}

interface TextButtonProps {
  text: string;
  style: CSSProperties;
  // For screen readers, where the text alone doesn't say what the button is for
  label?: string;
  onClick(e: ReactMouseEvent<HTMLButtonElement>): void;
}

// LookAndFeel_V4::drawButtonBackground lightens by 5% on hover and 20% while down
export function TextButton({ text, style, label, onClick }: TextButtonProps) {
  return (
    <button
      type="button"
      className="absolute cursor-pointer overflow-hidden rounded-md border border-line bg-widget px-1 text-ellipsis whitespace-nowrap text-fg hover:bg-[color-mix(in_srgb,var(--color-widget),white_5%)] active:bg-[color-mix(in_srgb,var(--color-widget),white_20%)]"
      style={{ fontSize: juceHeight(12), ...style }}
      aria-label={label}
      onClick={onClick}
    >
      {text}
    </button>
  );
}
