"use strict";
// По-гармонический аудит ноты: чем и насколько «дышит» каждый партиал.
// Это тот самый блок, которым Update 64 нашёл, что у флейты банка качается
// АМПЛИТУДА (2.7-3.4 дБ), а не высота.
import { analytic, modPeak, fftInPlace } from "./fft.mjs";
import { bandpass, instFrequency, db, rms, detrend, envelope } from "./dsp.mjs";

/// Полосы вокруг гармоник k·f0: {k, fc, halfWidth, lo, hi}.
export function harmonicBands(f0, { kmax = 6, maxHz = 16000, widthRatio = 0.08, minWidthHz = 30 } = {}) {
  const bands = [];
  for (let k = 1; k <= kmax; k++) {
    const fc = k * f0;
    if (fc > maxHz) break;
    const hw = Math.max(widthRatio * fc, minWidthHz);
    bands.push({ k, fc, halfWidth: hw, lo: Math.max(fc - hw, 10), hi: fc + hw });
  }
  return bands;
}

/// Трек одной гармоники: фаза (для ЧМ), огибающая (для АМ) и её уровень.
function trackBand(x, band, sampleRate) {
  const z = bandpass(x, band.fc, band.halfWidth, sampleRate);
  const { env, phase } = analytic(z);
  const ifHz = instFrequency(phase, sampleRate);
  const cents = new Float64Array(x.length);
  for (let i = 1; i < x.length; i++) cents[i] = 1200 * Math.log2(Math.max(ifHz[i], 1e-6) / band.fc);
  const envDb = new Float64Array(env.length);
  for (let i = 0; i < env.length; i++) envDb[i] = db(env[i]);
  let lvl = 0;
  for (let i = 0; i < env.length; i++) lvl += env[i];
  return { cents, envDb, level: db(lvl / Math.max(env.length, 1)) };
}

/// Срез ряда в окне [from, to] секунд.
function slice(series, sampleRate, from, to) {
  return series.slice(Math.max(2, Math.round(from * sampleRate)), Math.min(series.length, Math.round(to * sampleRate)));
}

/// Временнáя дорожка ТРЕМОЛО (АМ) одной полосы: по блокам blockSec с шагом
/// stepSec — rms отклонения огибающей в дБ и когерентный пик в [fLo, fHi].
/// Тренд снимается МНК-прямой по окну 0.4 с, поэтому спад ноты в rms не входит.
/// Нужен там, где важно КОГДА приходит модуляция, а не только её глубина
/// (владелец: «тремоло почти сразу начинается, не хватает раскачки»).
export function tremorTrack(x, band, {
  sampleRate = 44100, blockSec = 0.25, stepSec = 0.125, fromSec = 0, toSec = 3.5, fLo = 1.5, fHi = 12,
} = {}) {
  const z = bandpass(x, band.fc, band.halfWidth, sampleRate);
  const env = envelope(z, sampleRate, 40);
  const envDb = new Float64Array(env.length);
  for (let i = 0; i < env.length; i++) envDb[i] = db(env[i]);
  const det = detrend(envDb, sampleRate, 0.4);
  const blockLen = Math.max(64, Math.round(blockSec * sampleRate));
  const stepLen = Math.max(1, Math.round(stepSec * sampleRate));
  const start = Math.max(0, Math.round(fromSec * sampleRate));
  const end = Math.min(det.length, Math.round(toSec * sampleRate));
  const out = [];
  for (let s = start; s + blockLen <= end; s += stepLen) {
    const seg = det.slice(s, s + blockLen);
    out.push({ t0: (s - start) / sampleRate, amRms: rms(seg), peak: modPeak(seg, sampleRate, fLo, fHi) });
  }
  return out;
}

/// ШИРОКОПОЛОСНАЯ амплитудная модуляция (Update 66).
/// Зачем: узкополосная огибающая вокруг одной гармоники ПРЕВРАЩАЕТ ЧМ в АМ
/// (гармоника выезжает за край полосы) — именно так замер Update 64 «нашёл»
/// у флейты банка когерентную АМ 2.7-3.4 дБ, которой в широкой полосе нет
/// (владелец: «ищи ошибку в измерениях»). Широкая полоса не меняет энергию
/// от ЧМ (все партиалы уезжают вместе), поэтому видит ТОЛЬКО настоящую АМ.
/// band: {lo, hi} — диапазон в Гц (относительные полосы не годятся: у ЧМ
/// сдвиг растёт с номером гармоники).
export function wideAmTrack(x, { lo = 300, hi = 4000 }, {
  sampleRate = 44100, blockSec = 0.4, stepSec = 0.2, fromSec = 0, toSec = 3.4, fLo = 1.5, fHi = 12,
} = {}) {
  const fc = (lo + hi) / 2, halfWidth = (hi - lo) / 2;
  const z = bandpass(x, fc, halfWidth, sampleRate);
  const env = envelope(z, sampleRate, 40);
  const envDb = new Float64Array(env.length);
  for (let i = 0; i < env.length; i++) envDb[i] = db(env[i]);
  const det = detrend(envDb, sampleRate, 0.4);
  const blockLen = Math.max(64, Math.round(blockSec * sampleRate));
  const stepLen = Math.max(1, Math.round(stepSec * sampleRate));
  const start = Math.max(0, Math.round(fromSec * sampleRate));
  const end = Math.min(det.length, Math.round(toSec * sampleRate));
  const out = [];
  for (let s = start; s + blockLen <= end; s += stepLen) {
    const seg = det.slice(s, s + blockLen);
    out.push({ t0: (s - start) / sampleRate, amRms: rms(seg), peak: modPeak(seg, sampleRate, fLo, fHi) });
  }
  return out;
}

/// Межгармонический пол (Уровень 66): медиана магнитуд бинов, отстоящих от
/// любой гармоники дальше 15 % — то есть уровень ШУМА между партиалами,
/// в дБ отн. пика h1. По окнам времени и по срезам частоты (где именно шумит).
export function interHarmonicFloor(x, f0, {
  sampleRate = 44100, kmax = 24, winSec = 0.4,
  windows = [[0.3, 0.6], [1.0, 1.4], [2.0, 2.4], [3.0, 3.4]],
  slices = [[600, 1500], [1500, 3500], [3500, 8000]],
} = {}) {
  const N = 1 << Math.ceil(Math.log2(Math.max(winSec * sampleRate, 1024)));
  const binHz = sampleRate / N;
  const rows = [];
  for (const [a, b] of windows) {
    const s = Math.round(a * sampleRate);
    const len = Math.min(N, Math.max(0, Math.round(b * sampleRate) - s));
    if (len < 1024) continue;
    const re = new Float64Array(N), im = new Float64Array(N);
    for (let i = 0; i < len; i++) re[i] = x[s + i] * (0.5 - 0.5 * Math.cos(2 * Math.PI * i / (len - 1)));
    fftInPlace(re, im);
    const mag = (k) => Math.hypot(re[k], im[k]);
    // пики гармоник (для нормировки — h1)
    const peakOf = (fc) => {
      const c = Math.round(fc / binHz), half = Math.max(2, Math.round(0.05 * fc / binHz));
      let m = 0;
      for (let i = Math.max(1, c - half); i <= Math.min(N / 2 - 1, c + half); i++) m = Math.max(m, mag(i));
      return m;
    };
    const h1 = Math.max(peakOf(f0), 1e-20);
    // какой гармонике принадлежит бин (для исключения пиков)
    const nearHarmonic = (f) => {
      const k = Math.round(f / f0);
      return k >= 1 && Math.abs(f - k * f0) < 0.15 * f;
    };
    const slice = (lo, hi) => {
      const v = [];
      for (let i = Math.max(1, Math.round(lo / binHz)); i < Math.min(N / 2, Math.round(hi / binHz)); i++) {
        const f = i * binHz;
        if (nearHarmonic(f)) continue;
        v.push(mag(i));
      }
      if (!v.length) return null;
      v.sort((p, q) => p - q);
      return 20 * Math.log10(Math.max(v[Math.floor(v.length / 2)], 1e-20) / h1);
    };
    rows.push({
      from: a, to: b,
      floorDb: slice(600, Math.min(sampleRate / 2 - 200, 15000)),
      slices: slices.map(([lo, hi]) => ({ lo, hi, db: slice(lo, hi) })),
    });
  }
  return rows;
}

/// Основной вызов: { k, fm:{f,mag}, am:{f,mag}, amRms } по каждой гармонике и окну.
/// windows: { early: [с, с], sustain: [с, с] }; fLo/fHi — полоса поиска модуляции.
export function audit(x, f0, {
  sampleRate = 44100, bands, windows = { early: [0.25, 0.75], sustain: [1.5, 4.2] },
  fLo = 1.5, fHi = 12,
} = {}) {
  const use = bands ?? harmonicBands(f0);
  const rows = [];
  for (const band of use) {
    const t = trackBand(x, band, sampleRate);
    const row = { k: band.k, fc: band.fc, level: t.level, windows: {} };
    for (const [name, w] of Object.entries(windows)) {
      const fmSeg = slice(t.cents, sampleRate, w[0], w[1]);
      const amSeg = slice(t.envDb, sampleRate, w[0], w[1]);
      if (fmSeg.length < 64) { row.windows[name] = null; continue; }
      // медленный тренд (нота затухает) снимается, иначе АМ-rms — это спад
      const amDet = detrend(amSeg, sampleRate, 0.25);
      row.windows[name] = {
        fm: modPeak(fmSeg, sampleRate, fLo, fHi),
        fmRms: rms(fmSeg),
        am: modPeak(amDet, sampleRate, fLo, fHi),
        amRms: rms(amDet),
      };
    }	rows.push(row);
  }
  return rows;
}

/// Спектр ШУМА (воздуха) по абсолютным полосам, дБ отн. пика h1 (Update 66b).
/// `interHarmonicFloor` для высоких f0 бесполезен: допуск 0.15·f при f0 = 523 Гц
/// исключает ПОЧТИ ВСЕ бины выше 1 кГц, и срезы выходят пустыми («—» в отчёте) —
/// именно из-за этого прошлые «замеры дыхания» на C5/C6 ничего не показывали.
/// Здесь толерантность АБСОЛЮТНАЯ и узкая: убираются только сами пики гармоник,
/// а юбки и пол между ними остаются; уровень берётся МЕДИАНОЙ бинов полосы
/// (устойчиво к остаткам пиков). Информативен там, где нужно отличить
/// «воздух банка» (струйная полоса 0.5-3 кГц) от шипящего пола наверху.
/// Уровни гармоник в дБ отн. h1 (усреднённый амплитудный спектр по сустейну).
/// Пик ищется в пределах tolCents вокруг k·f0, а не в бине k·f0: у семплов банка
/// строй плавает, и «гармоника ровно в бине» даёт провал на 6-10 дБ. Этим
/// сравнением ловится «тембр не тот» — когда гармоники стоят не на своих местах
/// по уровню, а не по частоте.
export function harmonicLevels(x, f0, {
  sampleRate = 44100, N = 8192, fromSec = 1.4, toSec = 4.0, kmax = 16, tolCents = 60,
} = {}) {
  const start = Math.max(0, Math.round(fromSec * sampleRate));
  const end = Math.min(x.length, Math.round(toSec * sampleRate));
  if (end - start < N * 2) return null;
  const hop = N >> 1;
  const win = new Float64Array(N);
  for (let i = 0; i < N; i++) win[i] = 0.5 - 0.5 * Math.cos(2 * Math.PI * i / (N - 1));
  const acc = new Float64Array(N / 2);
  let frames = 0;
  for (let s = start; s + N <= end; s += hop) {
    const re = new Float64Array(N), im = new Float64Array(N);
    for (let i = 0; i < N; i++) re[i] = x[s + i] * win[i];
    fftInPlace(re, im);
    for (let k = 1; k < N / 2; k++) acc[k] += Math.sqrt(re[k] * re[k] + im[k] * im[k]);
    frames++;
  }
  if (!frames) return null;
  for (let k = 1; k < acc.length; k++) acc[k] /= frames;
  const binHz = sampleRate / N;
  const ratio = Math.pow(2, tolCents / 1200);
  const peakNear = (fc) => {
    const lo = Math.max(1, Math.ceil(fc / ratio / binHz));
    const hi = Math.min(acc.length - 1, Math.floor(fc * ratio / binHz));
    let m = 1e-20;
    for (let k = lo; k <= hi; k++) m = Math.max(m, acc[k]);
    return m;
  };
  const h1 = Math.max(peakNear(f0), 1e-20);
  const out = [];
  for (let k = 1; k <= kmax && k * f0 < sampleRate * 0.45; k++)
    out.push(20 * Math.log10(Math.max(peakNear(k * f0), 1e-20) / h1));
  return { frames, db: out };
}

// --- Короткие окна (60 мс) для атаки: общие для дорожки тембра и высоты ---
function shortWindows(lenSec, sampleRate) {
  const len = Math.max(256, Math.round(lenSec * sampleRate));
  const N = 1 << Math.ceil(Math.log2(len));
  const win = new Float64Array(N);
  for (let i = 0; i < len; i++) win[i] = 0.5 - 0.5 * Math.cos(2 * Math.PI * i / (len - 1));
  return { len, N, win, binHz: sampleRate / N };
}

function windowPower(x, s, { len, N, win }) {
  const re = new Float64Array(N), im = new Float64Array(N);
  for (let i = 0; i < len; i++) {
    const j = s + i;
    re[i] = (j >= 0 && j < x.length ? x[j] : 0) * win[i];
  }
  fftInPlace(re, im);
  const p = new Float64Array(N / 2);
  for (let k = 1; k < N / 2; k++) p[k] = re[k] * re[k] + im[k] * im[k];
  return p;
}

/// Пик партиала: максимум спектра в допуске вокруг k·f0 (строй семпла плавает).
function peakNearBin(p, fc, binHz, minTolHz, tolRatio) {
  const tol = Math.max(minTolHz, tolRatio * fc);
  const lo = Math.max(1, Math.ceil((fc - tol) / binHz));
  const hi = Math.min(p.length - 1, Math.floor((fc + tol) / binHz));
  let m = 1e-30, best = lo;
  for (let k = lo; k <= hi; k++) if (p[k] > m) { m = p[k]; best = k; }
  return { mag: m, bin: best };
}

/// Частота пика с квадратичной интерполяцией по трём бинам: без неё шаг сетки
/// (на 60-мс окне 16 Гц) даёт до ±30 центов ошибки — больше, чем любое вибрато.
function interpPeakHz(p, bin, binHz) {
  if (bin <= 0 || bin >= p.length - 1) return bin * binHz;
  const a = p[bin - 1], b = p[bin], c = p[bin + 1];
  const den = a - 2 * b + c;
  const d = Math.abs(den) < 1e-30 ? 0 : 0.5 * (a - c) / den;
  return (bin + Math.max(-1, Math.min(1, d))) * binHz;
}

/// Временнáя дорожка тембра и дыхания (Update 73).
/// Зачем: владелец — «в атаке нет выдоха, сразу общий тембр, как в сустейне»,
/// и «переход из атаки в сустейн у оригинала даёт более высокий звук, а у нас
/// гудение». Спектр по СУСТЕЙНУ этого не видит: нужны КОРОТКИЕ окна (60 мс),
/// чтобы поймать, что происходит в первые 200 мс. Здесь по каждому окну:
///   h[0..kmax)  — уровни партиалов k·f0, дБ отн. h1 из опорного окна сустейна;
///   bands[]     — межгармонический пол (шум МЕЖДУ партиалами) по абсолютным
///                 полосам, тем же отсчётом. Именно он и есть «выдох»: тон в
///                 полосах шума не участвует, потому что окрестности k·f0
///                 исключены.
/// Опорный уровень снят ТЕМ ЖЕ окном и той же нормировкой, что и рабочие
/// (иначе короткое окно и усреднённый спектр несравнимы по масштабу).
export function timbreTrack(x, f0, {
  sampleRate = 44100, kmax = 8, lenSec = 0.06, hopSec = 0.02,
  fromSec = 0, toSec = 0.8, refFromSec = 1.4, refToSec = 4.0,
  bands = [[600, 1500], [1500, 3500], [3500, 8000], [8000, 16000]],
  tolRatio = 0.06, minTolHz = 80,
} = {}) {
  const sw = shortWindows(lenSec, sampleRate);
  const { binHz } = sw;
  const spectrum = (s) => windowPower(x, s, sw);
  const peakNear = (p, fc) => peakNearBin(p, fc, binHz, minTolHz, tolRatio).mag;
  let refSum = 0, refN = 0;
  const refEnd = Math.min(x.length, Math.round(refToSec * sampleRate));
  for (let s = Math.round(refFromSec * sampleRate); s + sw.len <= refEnd; s += Math.max(1, sw.len >> 1)) {
    refSum += peakNear(spectrum(s), f0);
    refN++;
  }
  if (!refN) return null;
  const ref = refSum / refN;
  // Окрестности гармоник исключаем: их утечка (даже при Hann) на 30-40 дБ
  // выше пола и целиком определила бы медиану.
  const nearHarmonic = (f) => {
    const k = Math.round(f / f0);
    return k >= 1 && Math.abs(f - k * f0) < Math.max(minTolHz, 8 * binHz);
  };
  const rows = [];
  const hop = Math.max(1, Math.round(hopSec * sampleRate));
  const start = Math.round(fromSec * sampleRate);
  const end = Math.min(x.length, Math.round(toSec * sampleRate));
  for (let s = start; s + sw.len <= end; s += hop) {
    const p = spectrum(s);
    const h = [];
    for (let k = 1; k <= kmax && k * f0 < sampleRate * 0.45; k++)
      h.push(10 * Math.log10(Math.max(peakNear(p, k * f0), 1e-30) / ref));
    const bs = bands.map(([lo, hi]) => {
      const v = [];
      for (let k = Math.max(1, Math.ceil(lo / binHz)); k <= Math.min(p.length - 1, Math.floor(hi / binHz)); k++) {
        if (nearHarmonic(k * binHz)) continue;
        v.push(p[k]);
      }
      if (!v.length) return { lo, hi, db: null };
      v.sort((a, b) => a - b);
      return { lo, hi, db: 10 * Math.log10(Math.max(v[Math.floor(v.length / 2)], 1e-30) / ref) };
    });
    rows.push({ t0: (s - start) / sampleRate, h, bands: bs });
  }
  return rows;
}

/// Дорожка высоты тона по КОРОТКИМ окнам (Update 73).
/// Зачем: владелец — «переход из атаки в сустейн у оригинала даёт более
/// высокий звук, а у нас гудение». Одного партиала мало: у пан-флейты в атаке
/// h1 приглушён, ведут h3/h4/h5, и высота по h1 в начале читается неверно.
/// Поэтому f0 оценивается МНК по пикам НЕСКОЛЬКИХ гармоник (f(k) = k·f0),
/// а вклад каждой — по её магнитуде. Возвращает центы отклонения от номинала.
export function pitchTrack(x, f0, {
  sampleRate = 44100, kmax = 6, lenSec = 0.06, hopSec = 0.02,
  fromSec = 0, toSec = 0.6, tolRatio = 0.06, minTolHz = 80, floorDb = 30,
} = {}) {
  const sw = shortWindows(lenSec, sampleRate);
  const { binHz } = sw;
  const rows = [];
  const hop = Math.max(1, Math.round(hopSec * sampleRate));
  const start = Math.round(fromSec * sampleRate);
  const end = Math.min(x.length, Math.round(toSec * sampleRate));
  for (let s = start; s + sw.len <= end; s += hop) {
    const p = windowPower(x, s, sw);
    const peaks = [];
    for (let k = 1; k <= kmax && k * f0 < sampleRate * 0.45; k++) {
      const { mag, bin } = peakNearBin(p, k * f0, binHz, minTolHz, tolRatio);
      peaks.push({ k, mag, hz: interpPeakHz(p, bin, binHz) });
    }
    // Слабые партиалы (на 30 дБ ниже ведущего) в оценку не берём: у них
    // «пик» — это шум, и он тянет оценку в сторону.
    let top = 1e-30, lead = 1;
    for (const q of peaks) if (q.mag > top) { top = q.mag; lead = q.k; }
    const lim = top * Math.pow(10, -floorDb / 10);
    let num = 0, den = 0, used = 0;
    for (const q of peaks) {
      if (q.mag < lim) continue;
      num += q.mag * (q.hz / q.k);
      den += q.mag;
      used++;
    }
    const fEst = den > 0 ? num / den : f0;
    rows.push({
      t0: (s - start) / sampleRate,
      cents: 1200 * Math.log2(Math.max(fEst, 1e-6) / f0),
      lead, used,
    });
  }
  return rows;
}

export function bandNoiseFloor(x, f0, {
  sampleRate = 44100, N = 8192,
  fromSec = 1.4, toSec = 4.0,
  bands = [[400, 600], [600, 1200], [1200, 2400], [2400, 4800], [4800, 9600], [9600, 16000]],
  tolHz = 25,
} = {}) {
  const start = Math.max(0, Math.round(fromSec * sampleRate));
  const end = Math.min(x.length, Math.round(toSec * sampleRate));
  if (end - start < N * 2) return null;
  const hop = N >> 1;
  const win = new Float64Array(N);
  for (let i = 0; i < N; i++) win[i] = 0.5 - 0.5 * Math.cos(2 * Math.PI * i / (N - 1));
  const acc = new Float64Array(N / 2);
  let frames = 0;
  for (let s = start; s + N <= end; s += hop) {
    const re = new Float64Array(N), im = new Float64Array(N);
    for (let i = 0; i < N; i++) re[i] = x[s + i] * win[i];
    fftInPlace(re, im);
    for (let k = 1; k < N / 2; k++) acc[k] += re[k] * re[k] + im[k] * im[k];
    frames++;
  }
  if (!frames) return null;
  for (let k = 1; k < acc.length; k++) acc[k] /= frames;
  const binHz = sampleRate / N;
  let h1 = 0;
  const c = Math.round(f0 / binHz);
  for (let k = Math.max(1, c - 3); k <= Math.min(acc.length - 1, c + 3); k++) h1 = Math.max(h1, acc[k]);
  h1 = Math.max(h1, 1e-20);
  const nearHarmonic = (f) => {
    const k = Math.round(f / f0);
    return k >= 1 && Math.abs(f - k * f0) < tolHz;
  };
  return {
    frames,
    bands: bands.map(([lo, hi]) => {
      const v = [];
      for (let k = Math.max(1, Math.ceil(lo / binHz)); k <= Math.min(acc.length - 1, Math.floor(hi / binHz)); k++) {
        if (nearHarmonic(k * binHz)) continue;
        v.push(acc[k]);
      }
      if (!v.length) return { lo, hi, db: null };
      v.sort((p, q) => p - q);
      return { lo, hi, db: 10 * Math.log10(Math.max(v[Math.floor(v.length / 2)], 1e-20) / h1) };
    }),
  };
}

// ___________________________________________________________________________
// Harmonic width (the u183-width probe moved into lib).
//
// The owner asked to measure the width of every harmonic and its distribution rather than a single fitter. Width here is in cents at -6 and -20 dB around the local peak k*f0, plus the share of energy within +-30 cents against +-0.45*f0 (how much of it is a line, not a hump). The result is the median over h2..h12, the single figure used to compare our instrument with the bank (52 ChoirAahs: 91 cents against the bank's 93).
//
// Why average over frames instead of one FFT: the skirt of a wide harmonic is a sum of narrow terms that interfere inside a single frame, so the width jumps from frame to frame. A 16384 window (2.69 Hz grid) with 50% overlap averages that out.
export function harmonicWidths(x, f0, {
  kmax = 12, sampleRate = 44100, fromSec = 1.4, toSec = 4.0, maxHz = 16000,
} = {}) {
  const N = 16384, hop = N >> 1;
  const s0 = Math.round(fromSec * sampleRate), s1 = Math.min(x.length, Math.round(toSec * sampleRate));
  if (s1 - s0 < N * 2) return { perHarmonic: [], medianW6: null, medianW20: null, medianLineDb: null };
  const win = new Float64Array(N);
  for (let i = 0; i < N; i++) win[i] = 0.5 - 0.5 * Math.cos(2 * Math.PI * i / (N - 1));
  const acc = new Float64Array(N / 2);
  let frames = 0;
  for (let s = s0; s + N <= s1; s += hop) {
    const re = new Float64Array(N), im = new Float64Array(N);
    for (let i = 0; i < N; i++) re[i] = x[s + i] * win[i];
    fftInPlace(re, im);
    for (let k = 1; k < N / 2; k++) acc[k] += Math.hypot(re[k], im[k]);
    frames++;
  }
  if (!frames) return { perHarmonic: [], medianW6: null, medianW20: null, medianLineDb: null };
  for (let k = 1; k < acc.length; k++) acc[k] /= frames;
  const binHz = sampleRate / N;

  const width = (fc, dropDb) => {
    const c = Math.max(2, Math.min(acc.length - 2, Math.round(fc / binHz)));
    const half = Math.max(4, Math.round(0.35 * fc / binHz));
    let bi = c, bv = 0;
    for (let k = Math.max(1, c - half); k <= Math.min(acc.length - 2, c + half); k++)
      if (acc[k] > bv) { bv = acc[k]; bi = k; }
    const thr = bv * Math.pow(10, dropDb / 20);
    let lo = bi, hi = bi;
    while (lo > 1 && acc[lo] > thr) lo--;
    while (hi < acc.length - 2 && acc[hi] > thr) hi++;
    // Linear interpolation of the threshold crossing: without it the 2.69 Hz grid gives tens of cents of error on low harmonics.
    const cross = (iIn, iOut) => {
      const a = acc[iIn], b = acc[iOut];
      const u = a === b ? 0.5 : (a - thr) / (a - b);
      return (iIn + (iOut - iIn) * Math.max(0, Math.min(1, u))) * binHz;
    };
    const fLo = lo >= bi ? fc : cross(lo + 1, lo);
    const fHi = hi <= bi ? fc : cross(hi - 1, hi);
    const cents = Math.max(0, 1200 * Math.log2(Math.max(fHi, 1) / Math.max(fLo, 1)));
    const energy = (mult) => {
      const lo2 = Math.max(1, Math.ceil(fc / mult / binHz));
      const hi2 = Math.min(acc.length - 1, Math.floor(fc * mult / binHz));
      let e = 0;
      for (let k = lo2; k <= hi2; k++) e += acc[k] * acc[k];
      return e;
    };
    const eLine = energy(Math.pow(2, 30 / 1200)), eBand = energy(Math.pow(2, 450 / 1200));
    return { cents, lineDb: 10 * Math.log10(Math.max(eLine, 1e-24) / Math.max(eBand, 1e-24)) };
  };

  const perHarmonic = [];
  for (let k = 1; k <= kmax; k++) {
    const fc = k * f0;
    if (fc > maxHz || fc > sampleRate / 2 - 100) break;
    perHarmonic.push({ k, hz: fc, w6: width(fc, -6).cents, w20: width(fc, -20).cents, lineDb: width(fc, -6).lineDb });
  }
  const median = (vals) => {
    if (!vals.length) return null;
    const a = vals.slice().sort((p, q) => p - q);
    return a[Math.floor(a.length / 2)];
  };
  const audible = perHarmonic.filter((h) => h.k >= 2 && h.k <= 12);
  return {
    perHarmonic,
    medianW6: median(audible.map((h) => h.w6)),
    medianW20: median(audible.map((h) => h.w20)),
    medianLineDb: median(audible.map((h) => h.lineDb)),
  };
}
