export const DEFAULT_MASK_PALETTE_16 = [
  "#ff0000",
  "#ff4500",
  "#ff8c00",
  "#ffd700",
  "#00ff00",
  "#00ffff",
  "#1e90ff",
  "#0000ff",
  "#9370db",
  "#ff1493",
  "#ff69b4",
  "#ffffff",
  "#cccccc",
  "#808080",
  "#404040",
  "#000000",
] as const;

export function pickMaskColor16(index: number): string {
  const len = DEFAULT_MASK_PALETTE_16.length;
  const i = ((index % len) + len) % len;
  return DEFAULT_MASK_PALETTE_16[i];
}
