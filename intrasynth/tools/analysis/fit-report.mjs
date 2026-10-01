"use strict";
// ЕДИНЫЙ ОТЧЁТ ФИТТИНГА — один вызов на любой инструмент, все главные приборы
// сразу. Это тот самый «обобщённый вид», в котором скрипты фиттинга живут в
// репозитории: раньше такие таблицы собирались копипастой из зондов
// (`.scratch/*.mjs`), и каждый инструмент мерился своим набором команд.
//
//   node intrasynth/tools/analysis/fit-report.mjs 50:72 52:60 94:60
//   node intrasynth/tools/analysis/fit-report.mjs 52:48 52:60 --wasm .scratch/ab-builds/u185-approved/IntraSynth.js
//   node intrasynth/tools/analysis/fit-report.mjs 53:60 --json
//
// ЧТО ПЕЧАТАЕТ (на каждую ноту, наш → банк, все окна ОТ NOTE-ON):
//   уровень  — RMS сустейна 0.8-4.4 с, дБ разницы с банком. Нормировка таблицы
//              (`BuildWaveTable` делит на сумму амплитуд) сдвигает уровень при
//              любой правке профиля, поэтому это первая колонка, а не последняя;
//   профиль  — расхождение по гармоникам h2..h12 (`harmonicLevels`): среднее |Δ|
//              и худшая гармоника. Это «тембр» в цифрах;
//   ширина   — медиана ширины гармоник w-20 в центах (`harmonicWidths`).
//              У банка она примерно ПОСТОЯННА в центах (σ в герцах растёт с k),
//              а у нас зависит от формулы `b(k)` — см. решение по хору;
//   АМ       — глубина и эффективная скорость модуляции огибающей 0.2-15 Гц.
//              Однодоминантная линия = «биение» (жалоба владельца по 50
//              SynthStrings), разложенные линии = энсамбль;
//   луп      — автокорреляция сустейна на лаге таблицы (по умолчанию 16384
//              отсчётов = 371 мс). r > 0.8 на этом лаге = «робот» от повтора
//              вейвтейбла; у сэмплового банка такого пика нет.
//
// ЧЕГО НЕ ДЕЛАЕТ: не решает и не подгоняет. Отчёт сужает выбор, решение
// принимается ушами (панель A/B), а числа идут в ворклог.
//
// ОБЯЗАТЕЛЬНО перед первым запуском: `node intrasynth/tools/analysis/cli.mjs selftest`.
import { SR, renderPair, DEFAULT_NOTE_ON, DEFAULT_SF2 } from "./lib/render.mjs";
import { db } from "./lib/dsp.mjs";
import { harmonicLevels, harmonicWidths, wideAmTrack, movementByHarmonic } from "./lib/harmonics.mjs";
import { autocorrelation, modPeaks, modCentroid } from "./lib/fft.mjs";
import { envelope, detrend } from "./lib/dsp.mjs";

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
const notes = opts._.length ? opts._ : ["52:60"];
const wasmJs = opts.wasm || "intrasynth/web/generated/IntraSynth.js";
const sf2 = opts.sf2 || DEFAULT_SF2;
const NOTE_ON = Number(opts.noteon ?? DEFAULT_NOTE_ON);
const FROM = Number(opts.from ?? 0.8), TO = Number(opts.to ?? 4.4);
// Громкость банковского рендера. Синтезатор откалиброван по -g 0.6 (программа 0
// читается в 0), поэтому колонка «уровень» для ПОДГОНКИ ГРОМКОСТИ считается
// с `--bank-gain 0.6`; исторические числа лога мерились при 0.2 (см. lib/render.mjs).
const BANK_GAIN = Number(opts["bank-gain"] ?? 0.2);
const LAG = Number(opts.lag ?? 16384);
const f0of = (key) => 440 * Math.pow(2, (key - 69) / 12);

const rmsDb = (x, fromSec, toSec) => {
  const a = Math.max(0, Math.round(fromSec * SR)), b = Math.min(x.length, Math.round(toSec * SR));
  if (b <= a) return null;
  let s = 0;
  for (let i = a; i < b; i++) s += x[i] * x[i];
  return 20 * Math.log10(Math.sqrt(s / (b - a)) + 1e-12);
};

// АМ огибающей в полосе 0.2-15 Гц: глубина (по центроиду) и топ-линии.
function motion(x, fromSec, toSec) {
  const amp = envelope(x, SR, 60);
  const rel = new Float64Array(amp.length);
  for (let i = 0; i < amp.length; i++) rel[i] = db(amp[i]);
  const det = detrend(rel, SR, 0.5);
  const a = Math.max(0, Math.round(fromSec * SR)), b = Math.min(det.length, Math.round(toSec * SR));
  const seg = det.slice(a, b);
  const c = modCentroid(seg, SR, 0.2, 15);
  const peaks = modPeaks(seg, SR, 0.2, 15, 3);
  return { depthDb: c ? c.depthDb : null, centroidHz: c ? c.f : null, peaks };
}

// Периодика сустейна: r на лаге таблицы (и топ-пики, чтобы увидеть настоящий
// период, если он не равен ожидаемому).
function loopness(x, fromSec, toSec) {
  const a = Math.max(0, Math.round(fromSec * SR)), b = Math.min(x.length, Math.round(toSec * SR));
  const seg = x.slice(a, b - Math.max(1, Math.min(96000, Math.floor((b - a) / 3))));
  if (seg.length < 4096) return null;
  const r = autocorrelation(seg);
  const at = r[LAG] ?? null;
  const peaks = [];
  for (let lag = 129; lag < Math.min(r.length - 1, 96000); lag++)
    if (r[lag] > r[lag - 1] && r[lag] >= r[lag + 1]) peaks.push({ lag, r: r[lag] });
  peaks.sort((p, q) => q.r - p.r);
  return { at, top: peaks.slice(0, 3) };
}

const pad = (s, n) => String(s).padStart(n);
const rows = [];
for (const note of notes) {
  const [program, key] = note.split(":").map(Number);
  if (!Number.isFinite(program) || !Number.isFinite(key)) {
    console.error(`не понял ноту «${note}», жду prog:key`);
    process.exit(1);
  }
  const f0 = f0of(key);
  const pair = await renderPair({ program, key, wasmJs, sf2, gain: BANK_GAIN });
  const bank = opts["no-bank"] ? null : pair.bank;

  const lvlO = rmsDb(pair.ours, NOTE_ON + FROM, NOTE_ON + TO);
  const lvlB = bank ? rmsDb(bank, NOTE_ON + FROM, NOTE_ON + TO) : null;

  const hO = harmonicLevels(pair.ours, f0, { kmax: 12, fromSec: NOTE_ON + 1.4, toSec: NOTE_ON + 4.0 });
  const hB = bank ? harmonicLevels(bank, f0, { kmax: 12, fromSec: NOTE_ON + 1.4, toSec: NOTE_ON + 4.0 }) : null;
  let profMean = null, profWorst = null;
  if (hB) {
    const ds = [];
    for (let k = 2; k <= 12; k++) {
      // harmonicLevels отдаёт массив «дБ отн. h1», индекс 0 = h1.
      const a = hO?.db?.[k - 1], b = hB?.db?.[k - 1];
      if (a === undefined || b === undefined) continue;
      // Уровни ниже -60 дБ отн. h1 — это уже шум замера, а не тембр.
      if (a < -60 || b < -60) continue;
      ds.push({ k, d: a - b });
    }
    if (ds.length) {
      profMean = ds.reduce((s, v) => s + Math.abs(v.d), 0) / ds.length;
      profWorst = ds.reduce((w, v) => (Math.abs(v.d) > Math.abs(w.d) ? v : w), ds[0]);
    }
  }

  const wO = harmonicWidths(pair.ours, f0, { kmax: 12, fromSec: NOTE_ON + 1.4, toSec: NOTE_ON + 4.0 });
  const wB = bank ? harmonicWidths(bank, f0, { kmax: 12, fromSec: NOTE_ON + 1.4, toSec: NOTE_ON + 4.0 }) : null;
  const mO = motion(pair.ours, NOTE_ON + 0.9, NOTE_ON + 4.5);
  const mB = bank ? motion(bank, NOTE_ON + 0.9, NOTE_ON + 4.5) : null;
  // Движение КАЖДОЙ гармоники: колонка `АМ` выше — СУММА амплитуд всех линий
  // в полосе, она не отличает «одно тремоло на ноту» от «у каждой гармоники
  // своя модуляция». Здесь видно и то, и другое: скорость k-й гармоники,
  // растущая как k, — это биение пары расстроенных копий (или луп сэмпла),
  // не связанные между собой скорости — независимые источники на гармонику.
  const mvOpts = { sampleRate: SR, kmax: 6, lenSec: 0.08, hopSec: 0.02,
    fromSec: NOTE_ON + 0.8, toSec: NOTE_ON + 4.4, fLo: 0.3, fHi: 10 };
  const mvO = movementByHarmonic(pair.ours, f0, mvOpts);
  const mvB = bank ? movementByHarmonic(bank, f0, mvOpts) : null;
  const lO = loopness(pair.ours, NOTE_ON + 1.7, NOTE_ON + 4.2);
  const lB = bank ? loopness(bank, NOTE_ON + 1.7, NOTE_ON + 4.2) : null;

  rows.push({
    note, program, key,
    levelDb: lvlB === null ? null : lvlO - lvlB,
    profileMeanDb: profMean, profileWorst: profWorst,
    widthOursCents: wO.medianW20, widthBankCents: wB ? wB.medianW20 : null,
    lineOursDb: wO.medianLineDb, lineBankDb: wB ? wB.medianLineDb : null,
    amOurs: mO, amBank: mB,
    moveOurs: mvO ? mvO.perHarmonic : null, moveBank: mvB ? mvB.perHarmonic : null,
    loopOurs: lO, loopBank: lB,
  });
}

if (opts.json) {
  console.log(JSON.stringify({ wasmJs, noteOn: NOTE_ON, lag: LAG, rows }, null, 2));
} else {
  console.log(`отчёт фиттинга: ${wasmJs}`);
  console.log(`банк: ${sf2}; сустейн ${FROM}-${TO} с ОТ NOTE-ON (${NOTE_ON} с)`);
  console.log("нота    уровень  профиль h2..h12   ширина w-20,цент   АМ ноты 0.2-15 Гц  луп r@" + LAG + ` (банк -g ${BANK_GAIN})`);
  console.log("        наш-банк  средн. / худшая   наш  /  банк      СУММА линий, дБ / Гц  наш   /  банк");
  for (const r of rows) {
    const num = (v, d = 2) => (v === null || v === undefined ? "  —  " : pad(v.toFixed(d), 6));
    const am = (m) => (m.depthDb === null ? "   —   " : `${pad(m.depthDb.toFixed(2), 5)} ${pad(m.centroidHz.toFixed(2), 5)}`);
    console.log(
      `${r.note.padEnd(7)} ${num(r.levelDb, 2)}   ${num(r.profileMeanDb, 1)} / ${num(r.profileWorst?.d, 1)}`
      + `   ${num(r.widthOursCents, 0)} / ${num(r.widthBankCents, 0)}`
      + `   ${am(r.amOurs)} / ${am(r.amBank)}`
      + `   ${num(r.loopOurs?.at, 4)} / ${num(r.loopBank?.at, 4)}`,
    );
  }
  // Что именно стоит смотреть глазами: списки линий и лагов (их не видно в строке).
  for (const r of rows) {
    const pk = (m, n) => (m && m.peaks ? m.peaks.slice(0, n).map((p) => `${p.f.toFixed(2)}Гц/${p.mag.toFixed(2)}дБ`).join(" ") : "—");
    const top = (l) => (l && l.top ? l.top.map((p) => `${(p.lag / SR * 1000).toFixed(0)}мс/${p.r.toFixed(3)}`).join(" ") : "—");
    console.log(`\n${r.note}:`);
    console.log(`  АМ наш  : ${pk(r.amOurs, 3)}   | банк: ${pk(r.amBank, 3)}`);
    console.log(`  луп наш : ${top(r.loopOurs)}   | банк: ${top(r.loopBank)}`);
    if (r.profileWorst) console.log(`  худшая гармоника: h${r.profileWorst.k} ${r.profileWorst.d >= 0 ? "+" : ""}${r.profileWorst.d.toFixed(1)} дБ`);
    // Эквивалентная расстройка копий: k-я гармоника бьётся на k*f0*d/1731, значит
    // d = rate*1731/(k*f0) центов. По ней видно ЗАКОН движения по клавишам: постоянные
    // герцы (как у банка 54: 1.86/1.66/1.66 Гц на C3/C4/C5) дают d ~ 1/f, а фиксированные
    // центы дают скорость, растущую с нотой. Именно так поймано, что у 54 расстройка была
    // в центах, а у банка — в герцах (журнал, §26).
    const f0 = 440 * Math.pow(2, (Number(String(r.note).split(":")[1]) - 69) / 12);
    const hm = (arr, n) => (arr ? arr.slice(0, n)
      .map((h) => `h${h.k} ${h.depth === null ? "—" : h.depth.toFixed(1)}@${h.rate === null ? "—" : h.rate.toFixed(2)}`
        + (h.rate === null || !f0 ? "" : `=${(h.rate * 1731 / (h.k * f0)).toFixed(1)}ц`)).join("  ") : "—");
    if (r.moveOurs || r.moveBank)
      console.log(`  движение по гармоникам (0.3-10 Гц, окно 80 мс):\n    наш  ${hm(r.moveOurs, 6)}\n    банк ${hm(r.moveBank, 6)}`);
    if (r.widthOursCents !== null && r.widthBankCents !== null)
      console.log(`  ширина: наш ${r.widthOursCents.toFixed(0)} цент, банк ${r.widthBankCents.toFixed(0)}`
        + `, доля линии ${r.lineOursDb === null ? "—" : r.lineOursDb.toFixed(1)} / ${r.lineBankDb === null ? "—" : r.lineBankDb.toFixed(1)} дБ`);
  }
}
