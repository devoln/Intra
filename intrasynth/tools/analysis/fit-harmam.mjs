"use strict";
// ФИТТЕР ПЕР-ГАРМОНИЧЕСКОЙ АМ (Update 212). Один вызов даёт таблицу, из которой
// заполняются ядра голосовых пресетов (kVoiceCore*: глубина, скорость, фаза АМ
// на h1..h8 по областям банка).
//
//   node intrasynth/tools/analysis/fit-harmam.mjs 53:48 53:51 53:55 ... --bank
//   node intrasynth/tools/analysis/fit-harmam.mjs 54:60 54:72 --ours
//   node intrasynth/tools/analysis/fit-harmam.mjs 53:48 --out .scratch/harmam53.json
//
// ПОЧЕМУ ТАК, А НЕ ПО `movementByHarmonic`. Тот считает глубину как сумму
// амплитуд ВСЕХ линий модуляции в полосе (modCentroid.depthDb) и потому не
// различает «одна глубокая модуляция» и «много мелких»: у банка 53:60 гл. линия
// h1 всего 4.6 дБ, а суммарная глубина 9.6 дБ. Здесь измеряется НАСТОЯЩАЯ
// огибающая гармоники (полосовой фильтр ±1.5 % вокруг k·f0 + выпрямление с ФНЧ
// 30 Гц), и по ней:
//   * размах — пик-в-пик в дБ по сустейну (то, что слышно как «качает»);
//   * главная линия — частота и амплитуда сильнейшей линии модуляции;
//   * φ(note-on) — фаза этой линии В МОМЕНТ NOTE-ON, в долях оборота: по ней
//     ядро получает AmPhase, и входной участок получает ту же ФАЗУ, что у банка
//     (владелец: «у оригинала двухфазная атака, у нас слишком ровно»).
//
// Окно скорости: сустейн note-on+0.8…+4.4 с (атака в глубину не попадает).
// Размах и φ считаются по РАЗНЫМ окнам: размах — сустейн, φ — фит синусоиды на
// всём устойчивом участке note-on+0.25…+4.4 с, приведённый к note-on.
import fs from "node:fs";
import { SR, renderPair, DEFAULT_NOTE_ON, DEFAULT_SF2 } from "./lib/render.mjs";
import { bandpass, envelope, detrend } from "./lib/dsp.mjs";
import { modPeaks, modCentroid } from "./lib/fft.mjs";

function parseArgs(argv) {
  const o = { _: [] };
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a.startsWith("--")) {
      if (argv[i + 1] !== undefined && !argv[i + 1].startsWith("--")) o[a.slice(2)] = argv[++i];
      else o[a.slice(2)] = true;
    } else o._.push(a);
  }
  return o;
}

const opts = parseArgs(process.argv.slice(2));
const notes = opts._;
if (!notes.length) {
  console.log("укажите ноты вида prog:key, например 53:48 53:51");
  process.exit(2);
}
const wasmJs = opts.wasm || "intrasynth/web/generated/IntraSynth.js";
const sf2 = opts.sf2 || DEFAULT_SF2;
const NOTE_ON = Number(opts.noteon ?? DEFAULT_NOTE_ON);
const KMAX = Number(opts.harm ?? 8);
const FROM = Number(opts.from ?? 0.8), TO = Number(opts.to ?? 4.4);
const DEC = Number(opts.dec ?? 200);          // fs = 44100/200 = 220.5 Гц
const FS = SR / DEC;
const ampDb = (v) => 20 * Math.log10(Math.max(v, 1e-12));
const f0of = (key) => 440 * Math.pow(2, (key - 69) / 12);

// Синусоидальный фит по МНК на фиксированной частоте: амплитуда и фаза.
function fitAt(seg, rate, fs, t0Sec) {
  let sc = 0, ss = 0, cc = 0, cs = 0, sss = 0;
  for (let i = 0; i < seg.length; i++) {
    const a = 2 * Math.PI * rate * (t0Sec + i / fs);
    const c = Math.cos(a), s = Math.sin(a);
    sc += seg[i] * c; ss += seg[i] * s;
    cc += c * c; cs += c * s; sss += s * s;
  }
  const det = cc * sss - cs * cs;
  if (Math.abs(det) < 1e-12) return null;
  const re = (sc * sss - ss * cs) / det;
  const im = (ss * cc - sc * cs) / det;
  const mag = Math.hypot(re, im);
  const phi = Math.atan2(im, re);          // фаза cos-компоненты в момент t0Sec
  // привести фазу к t = 0 (note-on): φ(0) = φ(t0) − 2π·f·t0
  let phi0 = phi - 2 * Math.PI * rate * t0Sec;
  phi0 = ((phi0 % (2 * Math.PI)) + 2 * Math.PI) % (2 * Math.PI);
  return { mag, phi0 };
}

function track(x, f0, k) {
  const fc = k * f0;
  const hw = Math.max(0.015 * fc, 12);
  const env = envelope(bandpass(x, fc, hw, SR), SR, 30);
  const n = Math.floor(env.length / DEC);
  const arr = new Float64Array(n);
  for (let i = 0; i < n; i++) arr[i] = ampDb(env[i * DEC]);
  const a = Math.max(0, Math.round((NOTE_ON + FROM) * FS));
  const b = Math.min(n, Math.round((NOTE_ON + TO) * FS));
  const seg = arr.slice(a, b);
  const det = detrend(seg, FS, 0.5);
  let mn = Infinity, mx = -Infinity, r = 0;
  for (const v of seg) { if (v < mn) mn = v; if (v > mx) mx = v; }
  for (const v of det) r += v * v;
  const peaks = modPeaks(det, FS, Number(opts.fmin ?? 0.3), Number(opts.fmax ?? 20), 3);
  // ЭФФЕКТИВНАЯ скорость — спектральный центроид по мощности (`modCentroid`):
  // у банка линия не одна, и «слуховая» скорость ближе к центроиду, чем к
  // сильнейшей линии (у 54:72 h1 линия 1.72 Гц, а центроид 4.43).
  const cen = modCentroid(det, FS, Number(opts.fmin ?? 0.3), Number(opts.fmax ?? 20));
  // Фаза: фит на всём устойчивом участке note-on+0.25…+4.4 с.
  const fa = Math.max(0, Math.round((NOTE_ON + 0.25) * FS));
  const fb = Math.min(n, Math.round((NOTE_ON + TO) * FS));
  const fitSeg = Float64Array.from(arr.slice(fa, fb), (v) => v - rmsMean(arr, fa, fb));
  const phaseOf = (rate) => {
    const f = fitAt(fitSeg, rate, FS, 0.25);
    return f ? f.phi0 / (2 * Math.PI) : null;
  };
  return {
    k, p2p: mx - mn, rms: Math.sqrt(r / det.length),
    rate: peaks.length ? peaks[0].f : null,
    rateC: cen ? cen.f : null,
    lineDb: peaks.length ? peaks[0].mag : null,
    phase: peaks.length ? phaseOf(peaks[0].f) : null,
    phaseC: cen && cen.f > 0 ? phaseOf(cen.f) : null,
    lines: peaks,
  };
}

function rmsMean(arr, a, b) {
  let s = 0;
  for (let i = a; i < b; i++) s += arr[i];
  return s / Math.max(b - a, 1);
}

const rows = [];
console.log(`фиттер пер-гармонической АМ: ${opts.bank ? "банк" : "наш"} ${wasmJs}`);
console.log(`сустейн ${FROM}-${TO} с от note-on, окно огибающей ФНЧ 30 Гц, прореживание ${DEC}`);
console.log("нота    сторона  h  размах,дБ  rms,дБ  линия,Гц  центроид,Гц  линия,дБ  φ(линия)  φ(центр)  линии");
for (const note of notes) {
  const [program, key] = note.split(":").map(Number);
  const f0 = f0of(key);
  const pair = await renderPair({ program, key, wasmJs, sf2 });
  const sides = [];
  if (opts.bank) sides.push(["банк", pair.bank]);
  else if (opts.ours) sides.push(["наш", pair.ours]);
  else sides.push(["наш", pair.ours], ["банк", pair.bank]);
  for (const [name, x] of sides) {
    if (!x) { console.log(`${note}  ${name}: нет сигнала`); continue; }
    const row = { note, program, key, side: name, harmonics: [] };
    for (let k = 1; k <= KMAX; k++) {
      const t = track(x, f0, k);
      row.harmonics.push(t);
      const nb = (v, d = 2) => (v === null ? "—" : v.toFixed(d));
      console.log(`${note.padEnd(7)} ${name.padEnd(6)}  h${t.k}  ${t.p2p.toFixed(1).padStart(8)}  ${t.rms.toFixed(2).padStart(6)}  `
        + `${nb(t.rate).padStart(8)}  ${nb(t.rateC).padStart(11)}  ${nb(t.lineDb).padStart(8)}  `
        + `${nb(t.phase, 3).padStart(8)}  ${nb(t.phaseC, 3).padStart(8)}  ${t.lines.map((p) => `${p.f.toFixed(2)}/${p.mag.toFixed(2)}`).join(" ")}`);
    }
    rows.push(row);
  }
}
if (opts.out) {
  fs.writeFileSync(opts.out, JSON.stringify({ wasmJs, noteOn: NOTE_ON, rows }, null, 2));
  console.log(`\nзаписано: ${opts.out}`);
}
