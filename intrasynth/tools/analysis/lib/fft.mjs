"use strict";
// БПФ и спектральные строительные блоки для анализа звука Intra.
//
// ВАЖНО (Update 64): в прежних одноразовых зондах бабочка БПФ считалась как
// `vi = im*ci + re*cr` вместо `vi = im*cr + re*ci`, из-за чего комплексные
// спектры были неверны, а сделанные по ним выводы — недействительны. Здесь
// только одна реализация, и её проверяет `cli.mjs selftest` (round-trip
// сравнение с наивным ДПФ). Все новые зонды должны брать БПФ отсюда.

/// Радикс-2 БПФ на месте. Прямое: X_k = Σ x_n·e^(−i2πkn/N).
export function fftInPlace(re, im) {
  const n = re.length;
  if (n & (n - 1)) throw new Error(`fftInPlace: длина ${n} не степень двойки`);
  for (let i = 1, j = 0; i < n; i++) {
    let bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) {
      let t = re[i]; re[i] = re[j]; re[j] = t;
      t = im[i]; im[i] = im[j]; im[j] = t;
    }
  }
  for (let len = 2; len <= n; len <<= 1) {
    const ang = -2 * Math.PI / len, wr = Math.cos(ang), wi = Math.sin(ang);
    for (let i = 0; i < n; i += len) {
      let cr = 1, ci = 0;
      for (let k = 0; k < len / 2; k++) {
        const ur = re[i + k], ui = im[i + k];
        const xr = re[i + k + len / 2], xi = im[i + k + len / 2];
        const vr = xr * cr - xi * ci;
        const vi = xr * ci + xi * cr;
        re[i + k] = ur + vr; im[i + k] = ui + vi;
        re[i + k + len / 2] = ur - vr; im[i + k + len / 2] = ui - vi;
        const nr = cr * wr - ci * wi;
        ci = cr * wi + ci * wr;
        cr = nr;
      }
    }
  }
}

/// Обратное БПФ на месте (через сопряжение), результат уже поделён на N.
export function ifftInPlace(re, im) {
  const n = re.length;
  for (let i = 0; i < n; i++) im[i] = -im[i];
  fftInPlace(re, im);
  for (let i = 0; i < n; i++) { re[i] /= n; im[i] = -im[i] / n; }
}

/// Наивное ДПФ — эталон для selftest (медленно, только для проверок).
export function naiveDft(x) {
  const n = x.length, re = new Float64Array(n), im = new Float64Array(n);
  for (let k = 0; k < n; k++) {
    let sr = 0, si = 0;
    for (let i = 0; i < n; i++) {
      const a = -2 * Math.PI * k * i / n;
      sr += x[i] * Math.cos(a);
      si += x[i] * Math.sin(a);
    }
    re[k] = sr; im[k] = si;
  }
  return { re, im };
}

/// Огибающая и (накопленная) фаза аналитического сигнала: одна реализация
/// для всех «треков частоты». IF[i]-IF[i-1] даёт мгновенную частоту в Гц.
export function analytic(x) {
  const n = x.length;
  const m = 1 << Math.ceil(Math.log2(Math.max(n, 2)));
  const re = new Float64Array(m), im = new Float64Array(m);
  for (let i = 0; i < n; i++) re[i] = x[i];
  fftInPlace(re, im);
  for (let k = 1; k < m / 2; k++) { re[k] *= 2; im[k] *= 2; }
  for (let k = m / 2 + 1; k < m; k++) { re[k] = 0; im[k] = 0; }
  ifftInPlace(re, im);
  const env = new Float64Array(n), phase = new Float64Array(n);
  let prev = Math.atan2(im[0], re[0]), acc = 0;
  for (let i = 0; i < n; i++) {
    const ph = Math.atan2(im[i], re[i]);
    env[i] = Math.hypot(re[i], im[i]);
    if (i > 0) {
      let d = ph - prev;
      while (d > Math.PI) d -= 2 * Math.PI;
      while (d < -Math.PI) d += 2 * Math.PI;
      acc += d;
    }
    prev = ph;
    phase[i] = acc;
  }
  return { env, phase };
}

/// Модуляционный спектр ряда: пик в полосе [fLo, fHi] Гц → {f, mag}.
/// mag — амплитуда синусоиды этой частоты в единицах ряда (окно Ханна).
export function modPeak(series, sampleRate, fLo, fHi) {
  const n = series.length;
  if (n < 64) return { f: 0, mag: 0 };
  let mean = 0;
  for (let i = 0; i < n; i++) mean += series[i];
  mean /= n;
  const m = 1 << Math.ceil(Math.log2(n * 2));
  const re = new Float64Array(m), im = new Float64Array(m);
  for (let i = 0; i < n; i++) re[i] = (series[i] - mean) * (0.5 - 0.5 * Math.cos(2 * Math.PI * i / (n - 1)));
  fftInPlace(re, im);
  let best = { f: 0, mag: 0 };
  for (let k = 1; k < m / 2; k++) {
    const f = k * sampleRate / m;
    if (f < fLo || f > fHi) continue;
    const mag = 2 * Math.hypot(re[k], im[k]) / n / 0.5;
    if (mag > best.mag) best = { f, mag };
  }
  return best;
}

/// Линейная автокорреляция ряда через БПФ (нулевое дополнение до 2N, поэтому
/// без кругового заворота) + НОРМИРОВКА по энергии перекрытия — иначе затухание
/// ноты само даёт «высокую» корреляцию на больших лагах.
/// r[0] == 1; r[lag] = нормированная корреляция x[i] и x[i+lag].
export function autocorrelation(x) {
  const n = x.length;
  const m = 1 << Math.ceil(Math.log2(Math.max(2 * n, 4)));
  const re = new Float64Array(m), im = new Float64Array(m);
  for (let i = 0; i < n; i++) re[i] = x[i];
  fftInPlace(re, im);
  for (let k = 0; k < m; k++) { const p = re[k] * re[k] + im[k] * im[k]; re[k] = p; im[k] = 0; }
  ifftInPlace(re, im);
  const csum = new Float64Array(n + 1);
  for (let i = 0; i < n; i++) csum[i + 1] = csum[i] + x[i] * x[i];
  const out = new Float64Array(n);
  for (let lag = 0; lag < n; lag++) {
    const e0 = csum[n - lag], e1 = csum[n] - csum[lag];
    out[lag] = e0 > 0 && e1 > 0 ? re[lag] / Math.sqrt(e0 * e1) : 0;
  }
  return out;
}

/// Топ-N пиков модуляционного спектра (для поиска «левых» периодов).
export function modPeaks(series, sampleRate, fLo, fHi, top = 6) {
  const n = series.length;
  if (n < 64) return [];
  let mean = 0;
  for (let i = 0; i < n; i++) mean += series[i];
  mean /= n;
  const m = 1 << Math.ceil(Math.log2(n * 2));
  const re = new Float64Array(m), im = new Float64Array(m);
  for (let i = 0; i < n; i++) re[i] = (series[i] - mean) * (0.5 - 0.5 * Math.cos(2 * Math.PI * i / (n - 1)));
  fftInPlace(re, im);
  const out = [];
  for (let k = 1; k < m / 2; k++) {
    const f = k * sampleRate / m;
    if (f < fLo || f > fHi) continue;
    out.push({ f, mag: 2 * Math.hypot(re[k], im[k]) / n / 0.5 });
  }
  out.sort((a, b) => b.mag - a.mag);
  return out.slice(0, top);
}

/// Update 70: «эффективная скорость» модуляции — спектральный центроид
/// (по мощности) плюс RMS глубины. Ровный синус 4 Гц даёт 4.0 Гц; он же с
/// третьей гармоникой ТОЙ ЖЕ амплитуды — ~7.4 Гц. Именно поэтому импульсный
/// модулятор банка слышится «чаще» при той же основной частоте: сравнивать
/// рендеры по одной только основной линии (`modPeaks`) недостаточно.
/// `series` — та же детрендированная огибающая в дБ, что идёт в modPeaks.
/// Глубина считается ТОЛЬКО по полосе [fLo,fHi] (амплитуды тех же линий, что
/// печатает modPeaks): если брать полный RMS остатка, у быстро затухающих
/// нот в него протекает под-герцовый остаток затухания и число теряет смысл.
export function modCentroid(series, sampleRate, fLo, fHi) {
  const n = series.length;
  if (n < 64) return null;
  let mean = 0;
  for (let i = 0; i < n; i++) mean += series[i];
  mean /= n;
  const m = 1 << Math.ceil(Math.log2(n * 2));
  const re = new Float64Array(m), im = new Float64Array(m);
  for (let i = 0; i < n; i++) re[i] = (series[i] - mean) * (0.5 - 0.5 * Math.cos(2 * Math.PI * i / (n - 1)));
  fftInPlace(re, im);
  let num = 0, den = 0;
  for (let k = 1; k < m / 2; k++) {
    const f = k * sampleRate / m;
    if (f < fLo || f > fHi) continue;
    const p = re[k] * re[k] + im[k] * im[k];
    num += f * p; den += p;
  }
  // A_k = 2·|X_k|/(n·0.5) — как в modPeaks; RMS суммы синусов = √(ΣA_k²/2).
  const depthDb = Math.sqrt(8 * den) / n;
  return { f: den > 0 ? num / den : 0, depthDb };
}
