"use strict";
// Общие DSP-примитивы анализа Intra: фильтры, огибающие, окна уровней,
// центы/дБ, снятие тренда. Всё, что раньше копипастилось в каждый зонд.

export const db = (v) => 20 * Math.log10(Math.max(v, 1e-15));

/// Биквадрат RBJ: kind = "lp" | "hp" | "bp" (bp — полосовой по центру fc).
export function biquad(x, fc, sampleRate, kind = "bp", q = 0.7071) {
  const w0 = 2 * Math.PI * fc / sampleRate;
  const c = Math.cos(w0), s = Math.sin(w0), alpha = s / (2 * q);
  let b0, b1, b2;
  const a0 = 1 + alpha, a1 = -2 * c, a2 = 1 - alpha;
  if (kind === "lp") { b0 = (1 - c) / 2; b1 = 1 - c; b2 = (1 - c) / 2; }
  else if (kind === "hp") { b0 = (1 + c) / 2; b1 = -(1 + c); b2 = (1 + c) / 2; }
  else { b0 = alpha; b1 = 0; b2 = -alpha; }
  const y = new Float64Array(x.length);
  let x1 = 0, x2 = 0, y1 = 0, y2 = 0;
  for (let i = 0; i < x.length; i++) {
    const v = (b0 * x[i] + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2) / a0;
    x2 = x1; x1 = x[i];
    y2 = y1; y1 = v;
    y[i] = v;
  }
  return y;
}

/// Каскад biquad'ов с одинаковыми fc/Q (для крутых скатов).
export function biquadCascade(x, fc, sampleRate, kind, q, passes) {
  let y = x;
  for (let i = 0; i < Math.max(passes, 1); i++) y = biquad(y, fc, sampleRate, kind, q);
  return y;
}

/// Полосовой фильтр вокруг fc шириной ±halfWidth Гц (4-й порядок).
export function bandpass(x, fc, halfWidth, sampleRate) {
  const lo = Math.max(fc - halfWidth, 10);
  const hi = Math.min(fc + halfWidth, sampleRate / 2 - 100);
  let y = biquadCascade(x, lo, sampleRate, "hp", 0.7071, 2);
  y = biquadCascade(y, hi, sampleRate, "lp", 0.7071, 2);
  return y;
}

/// Огибающая: выпрямление + однополюсный ФНЧ (fc Гц).
export function envelope(x, sampleRate, fc = 30) {
  const a = 1 - Math.exp(-2 * Math.PI * fc / sampleRate);
  const env = new Float64Array(x.length);
  let e = 0;
  for (let i = 0; i < x.length; i++) { e += a * (Math.abs(x[i]) - e); env[i] = e; }
  return env;
}

/// Снять медленный тренд: в окне ±winSec/2 строится МНК-прямая и вычитается её
/// значение в текущем отсчёте. Линейный тренд исчезает точно во всех точках,
/// включая края (простое скользящее среднее оставляет на краях ±половину
/// размаха, а зеркалирование — ±четверть; прежняя версия к тому же делила на
/// неверную длину окна и давала мусор ×1e5). O(n) на префиксных суммах.
export function detrend(series, sampleRate, winSec = 0.25) {
  const n = series.length;
  if (n < 4) return Float64Array.from(series);
  const W = Math.max(2, Math.min(Math.round(winSec * sampleRate), n - 1));
  const H = Math.floor(W / 2);
  const P = new Float64Array(n + 1); // Σ x
  const Q = new Float64Array(n + 1); // Σ x·j
  for (let j = 0; j < n; j++) { P[j + 1] = P[j] + series[j]; Q[j + 1] = Q[j] + series[j] * j; }
  const out = new Float64Array(n);
  for (let i = 0; i < n; i++) {
    const lo = Math.max(0, i - H), hi = Math.min(n - 1, i + H);
    const N = hi - lo + 1;
    const S = P[hi + 1] - P[lo];
    const Sx = Q[hi + 1] - Q[lo];
    const iMean = (lo + hi) / 2; // средний индекс окна (НЕ путать со средним уровнем)
    // Σ j² по [lo, hi] — замкнутая формула, без ещё одного префиксного массива
    const j2 = (hi * (hi + 1) * (2 * hi + 1) - (lo - 1) * lo * (2 * lo - 1)) / 6;
    const den = j2 - N * iMean * iMean;
    const slope = den > 1e-9 ? (Sx - iMean * S) / den : 0;
    out[i] = series[i] - (S / N + slope * (i - iMean));
  }
  return out;
}

export function rms(series) {
  const n = series.length;
  if (!n) return 0;
  let m = 0;
  for (let i = 0; i < n; i++) m += series[i];
  m /= n;
  let s = 0;
  for (let i = 0; i < n; i++) s += (series[i] - m) ** 2;
  return Math.sqrt(s / n);
}

/// Мгновенная частота (Гц) из накопленной фазы аналитического сигнала.
export function instFrequency(phase, sampleRate) {
  const n = phase.length, f = new Float64Array(n);
  f[0] = f[1] = 0;
  for (let i = 1; i < n; i++) f[i] = (phase[i] - phase[i - 1]) * sampleRate / (2 * Math.PI);
  return f;
}

/// Ряд уровней RMS по окнам; t — мс от startSeconds.
export function levelWindows(x, sampleRate, { lenMs, hopMs, fromSeconds = 0, toSeconds = 1e9 }) {
  const len = Math.max(1, Math.round(lenMs * sampleRate / 1000));
  const hop = Math.max(1, Math.round(hopMs * sampleRate / 1000));
  const start = Math.round(fromSeconds * sampleRate);
  const end = Math.min(x.length, Math.round(toSeconds * sampleRate));
  const out = [];
  for (let s = start; s + len <= end; s += hop) {
    let acc = 0;
    for (let i = 0; i < len; i++) acc += x[s + i] * x[s + i];
    out.push({ t: (s - start) * 1000 / sampleRate, db: 10 * Math.log10(Math.max(acc / len, 1e-20)) });
  }
  return out;
}

/// Опорный уровень ноты (RMS в окне refSec…refSec+refLen).
export function referenceLevel(x, sampleRate, refSec = 2.0, refLen = 0.5) {
  const seg = x.subarray(Math.round(refSec * sampleRate), Math.round((refSec + refLen) * sampleRate));
  let acc = 0;
  for (let i = 0; i < seg.length; i++) acc += seg[i] * seg[i];
  return 10 * Math.log10(Math.max(acc / Math.max(seg.length, 1), 1e-20));
}
