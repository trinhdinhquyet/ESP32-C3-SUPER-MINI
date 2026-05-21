/**
 * Quy tắc hiển thị % trên route …/kh (đồng bộ gauge + Average Availability Rate).
 */
export function applyKhScoreAdjust(s) {
  const v = Math.min(100, Math.max(0, s))
  if (v < 20) return Math.min(100, Math.max(v + 15, 20))
  if (v <= 70) return Math.min(100, v + 18)
  return v
}
