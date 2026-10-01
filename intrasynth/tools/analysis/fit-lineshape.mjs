"use strict";
// Форма спектральной линии гармоники: наш синтез против банка.
//
//   node intrasynth/tools/analysis/fit-lineshape.mjs 54:72 54:84
//   node intrasynth/tools/analysis/fit-lineshape.mjs 54:72 --harm 6 --cents 80
//   node intrasynth/tools/analysis/fit-lineshape.mjs 54:72 --wasm dist/IntraSynth.js
//
// Зачем отдельная метрика. fit-report даёт УРОВЕНЬ гармоник (spec) и ШИРИНУ
// (w-6/w-20), но не форму линии. Работы по 54 SynthVoice и 53 VoiceOohs (и
// извлечение из чатов ChatGPT, см. docs/tasks/active/20260927-ChatGptHandoffExtraction.md)
// опирались именно на форму: у банка низкая гармоника — не линия, а ядро с
// юбкой и боковыми «плечами», и одна только ширина по -20 дБ этого не видит.
//
// Метод. Берём окно 8192 сэмпла с Hann, считаем БПФ, находим реальный пик
// гармоники в пределах ±cents вокруг k*f0 (у банка строй плавает), приводим
// форму к центовой сетке относительно k*f0, нормируем каждую кривую по её
// максимуму и считаем RMS разницы в дБ только там, где форма банка не ниже
// -40 дБ. Это ровно метрика, которой мерили агенты: «ошибка формы h1..h3».
//
// Всё время — ОТ NOTE-ON (--noteon, по умолчанию 0.2 с), как в остальном CLI.
import { SR, renderPair, DEFAULT_WASM_JS, DEFAULT_SF2, DEFAULT_NOTE_ON } from "./lib/render.mjs";
import { fftInPlace } from "./lib/fft.mjs";

function parseArgs(argv) {
  const opts = { _: [] };
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a.startsWith("--")) {
      const eq = a.indexOf("=");
      if (eq > 0) opts[a.slice(2, eq)] = a.slice(eq + 1);
      else if (argv[i + 1] !== undefined && !argv[i + 1].startsWith("--")) opts[a.slice(2)] = argv[++i];
      else opts[a.slice(2)] = true;
    } else opts._.push(a);
  }
  return opts;
}
const pad = (s, n) => String(s).padStart(n);
const f0of = (key) => 440 * Math.pow(2, (key - 69) / 12);

// Магнитуда БПФ окна с Hann, начиная с from (секунды от начала файла).
function spectrum(x, from, len) {
  const re = new Float64Array(len);
  const im = new Float64Array(len);
  const start = Math.round(from * SR);
  for (let i = 0; i < len; i++) {
    const v = x[start + i] || 0;
    re[i] = v * (0.5 - 0.5 * Math.cos((2 * Math.PI * i) / len));
  }
  fftInPlace(re, im);
  const half = len >> 1;
  const mag = new Float64Array(half);
  for (let k = 0; k < half; k++) mag[k] = Math.hypot(re[k], im[k]);
  return { mag, binHz: SR / len };
}

function binAt({ mag, binHz }, f) {
  const k = f / binHz;
  const k0 = Math.floor(k);
  if (k0 < 1 || k0 + 1 >= mag.length) return 0;
  return mag[k0] * (1 - (k - k0)) + mag[k0 + 1] * (k - k0);
}

// Пик гармоники: реальная частота внутри ±cents от k*f0.
function peakHz(sp, fc, cents) {
  let best = fc;
  let bestV = -1;
  const lo = fc * Math.pow(2, -cents / 1200);
  const hi = fc * Math.pow(2, cents / 1200);
  const kLo = Math.max(1, Math.floor(lo / sp.binHz));
  const kHi = Math.min(sp.mag.length - 2, Math.ceil(hi / sp.binHz));
  for (let k = kLo; k <= kHi; k++) if (sp.mag[k] > bestV) { bestV = sp.mag[k]; best = k * sp.binHz; }
  return best;
}

// Форма линии в дБ относительно её максимума, по центовой сетке от fc.
function lineShape(sp, fc, cents, grid) {
  const out = new Float64Array(grid);
  let peak = 1e-30;
  for (let i = 0; i < grid; i++) {
    const c = -cents + (2 * cents * i) / (grid - 1);
    const v = binAt(sp, fc * Math.pow(2, c / 1200));
    out[i] = v;
    if (v > peak) peak = v;
  }
  for (let i = 0; i < grid; i++) out[i] = 20 * Math.log10(Math.max(out[i], 1e-30) / peak);
  return out;
}

async function main() {
  const opts = parseArgs(process.argv.slice(2));
  const notes = opts._;
  if (!notes.length) {
    console.error("usage: fit-lineshape.mjs prog:key [prog:key ...] [--harm N] [--cents C] [--len N] [--from S] [--wasm path]");
    process.exit(1);
  }
  const wasmJs = opts.wasm || DEFAULT_WASM_JS;
  const harm = Number(opts.harm || 3);
  const cents = Number(opts.cents || 60);
  const len = Number(opts.len || 8192);
  // --from — секунды ОТ NOTE-ON (как в остальном CLI); note-on стоит не на нуле файла.
  const from = Number(opts.from ?? 0.8) + (opts.noteon === "0" ? 0 : DEFAULT_NOTE_ON);
  const grid = 121;
  console.log(`форма линии h1..h${harm}: ${wasmJs}`);
  console.log(`банк: ${opts.sf2 || DEFAULT_SF2}; окно ${len} сэмплов (~${(len / SR * 1000).toFixed(0)} мс) через ${from.toFixed(2)} с от note-on`);
  console.log("нота   " + Array.from({ length: harm }, (_, i) => pad(`h${i + 1}`, 7)).join("") + pad("средн.", 8));
  for (const spec of notes) {
    const [progS, keyS] = spec.split(":");
    const program = Number(progS);
    const key = Number(keyS);
    const { ours, bank } = await renderPair({ program, key, wasmJs, sf2: opts.sf2 || DEFAULT_SF2 });
    if (!bank) { console.log(spec + "  банк недоступен (нет SF2)"); continue; }
    const spO = spectrum(ours, from, len);
    const spB = spectrum(bank, from, len);
    const f0 = f0of(key);
    const errs = [];
    for (let h = 1; h <= harm; h++) {
      const fc = f0 * h;
      if (fc > SR * 0.45) { errs.push(null); continue; }
      const pO = peakHz(spO, fc, cents);
      const pB = peakHz(spB, fc, cents);
      const so = lineShape(spO, pO, cents, grid);
      const sb = lineShape(spB, pB, cents, grid);
      let sum = 0;
      let n = 0;
      for (let i = 0; i < grid; i++) if (sb[i] >= -40) { sum += (so[i] - sb[i]) ** 2; n++; }
      errs.push(n ? Math.sqrt(sum / n) : null);
    }
    const good = errs.filter((e) => e !== null);
    const mean = good.length ? good.reduce((a, b) => a + b, 0) / good.length : NaN;
    console.log(spec + " " + errs.map((e) => pad(e === null ? "—" : e.toFixed(2), 7)).join("") + pad(mean.toFixed(2), 8));
  }
}
main().catch((e) => { console.error(e); process.exit(1); });
