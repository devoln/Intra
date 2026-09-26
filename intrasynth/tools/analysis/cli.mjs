"use strict";
// CLI анализа Intra. Всё, что раньше было одноразовыми зондами с копипастой
// (и с однажды проехавшим БПФ), теперь идёт через общие блоки в ./lib.
//
//   node intrasynth/tools/analysis/cli.mjs selftest
//   node intrasynth/tools/analysis/cli.mjs vib 75:60 43:72 [--harm 4] [--wasm путь] [--no-bank]
//   node intrasynth/tools/analysis/cli.mjs env 74:72 [--len 25] [--to 6000]
//   node intrasynth/tools/analysis/cli.mjs attack 74:72 [--len 2] [--to 150]
//   node intrasynth/tools/analysis/cli.mjs mod 75:60 [--band 0.2,15]
//   node intrasynth/tools/analysis/cli.mjs bands 75:72 [--from 1.4] [--to 4.0]
//   node intrasynth/tools/analysis/cli.mjs timbre 43:60 [--to 0.5] [--len 0.06] [--harm 6]
//   node intrasynth/tools/analysis/cli.mjs cost 50:60 52:60 [--sec 3] [--chord 5] [--poly 16]
//
// prog:key — как везде: программа GM и MIDI-клавиша.
// Update 74: `prog` — программа СИНТЕЗАТОРА, а НЕ пресет банка. Для банка
// применяется BANK_PROGRAM (lib/render.mjs): 43/115 → пресет 73 «Flute»,
// 77 → 78. Подмена печатается строкой «(банк: программа синтезатора 43 →
// пресет банка 73)» — раньше её не было, и замеры флейты семь апдейтов шли
// по пресету 43 (Contra Bass).
//
// ВСЁ ВРЕМЯ В КОМАНДАХ — ОТ NOTE-ON (`--noteon`, по умолчанию 0.2 с), а не от
// начала рендера: note-on стоит не на нуле, и окна «атаки» раньше читали
// тишину до ноты (все окна выходили в полу шума). Если нужно от начала
// рендера — `--noteon 0`.
import path from "node:path";
import { SR, renderPair, renderOurs, renderBank, DEFAULT_WASM_JS, DEFAULT_SF2, DEFAULT_NOTE_ON } from "./lib/render.mjs";
import { fftInPlace, ifftInPlace, naiveDft, modPeaks, modCentroid, autocorrelation } from "./lib/fft.mjs";
import { db, levelWindows, referenceLevel, detrend, envelope, bandpass, rms } from "./lib/dsp.mjs";
import { audit, harmonicBands, tremorTrack, wideAmTrack, interHarmonicFloor, bandNoiseFloor, harmonicLevels, timbreTrack, pitchTrack } from "./lib/harmonics.mjs";

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

// ---------------------------------------------------------------------------
// `cost` measures the instrument cost in milliseconds.
//
// Three separate things that are easy to confuse, and all three matter:
//
// 1. Table build (cold note): additive presets build the table once per (region, frequency) and cache it, so the cost is cold note minus warm note. This is where a preset gets more expensive from a profile or wide air harmonics: the number of partials before the inverse FFT grows, not the playback cost. 2. NoteOn (warm note) creates the samplers: the wavetable layer plus a WaveFormSampler per ensemble singer. 3. Render reports milliseconds per second of sound (percent of realtime); it does not depend on the partial count, since a wavetable singer costs the same per sample as the layer alone.
//
// One WASM instance and the live source (_SourceCreateLive) rather than renderOurs: renderOurs starts a second instance, so the probe's handles and state never reach it.
//
// Usage: node intrasynth/tools/analysis/cli.mjs cost 50:60 52:60 [--wasm path]
//         [--sec 3] [--chord 5] [--reps 7] [--json]
async function costCommand(opts, notes) {
  const wasmJs = opts.wasm || DEFAULT_WASM_JS;
  const sec = Number(opts.sec || 3);
  const reps = Number(opts.reps || 7);
  const chordN = opts.chord ? Number(opts.chord) : 0;
  // the path is resolved here: the import is relative to this file, the path is from the repository root
  const { default: Factory } = await import(path.resolve(wasmJs));
  const Module = await Factory();
  const BLOCK = 256;
  const src = Module._SourceCreateLive(SR, 2);
  const ptr = Module._malloc(BLOCK * 2 * 4);
  const drain = (blocks) => { for (let i = 0; i < blocks; i++) Module._SourceGetUninterleavedSamples(src, ptr, BLOCK, BLOCK); };
  const ev = (s, d0, d1) => Module._SourceSendMidiEvent(src, s, d0, d1);
  const med = (a) => { const b = [...a].sort((x, y) => x - y); return b[b.length >> 1]; };
  const min = (a) => Math.min(...a);
  const rows = [];
  for (const note of notes) {
    const [program, key] = note.split(":").map(Number);
    ev(0xC0, program, 0);
    ev(0x80, key, 0);
    drain(8);
    const cold = [], warm = [], chord = [];
    for (let k = 0; k < reps; k++) {
      ev(0x80, key, 0);
      drain(6);
      let t = performance.now();
      ev(0x90, key, 90);
      (k === 0 ? cold : warm).push(performance.now() - t);
      drain(6);
    }
    // Render: the note already sounds and the table is cached, so this measures the per-sample cost only.
    const blocks = Math.round(sec * SR / BLOCK);
    const r = [];
    for (let k = 0; k < 3; k++) {
      ev(0x80, key, 0);
      drain(6);
      ev(0x90, key, 90);
      drain(8);
      const t = performance.now();
      drain(blocks);
      r.push(performance.now() - t);
      ev(0x80, key, 0);
      drain(6);
    }
    const renderMs = min(r);
    // Polyphony: the same number of seconds, but with --poly notes at once. One voice is not enough to answer "is this expensive?" when a hand plays chords.
    const poly = opts.poly ? Number(opts.poly) : 0;
    let polyMs = null, polyPct = null;
    if (poly > 1) {
      const keys = Array.from({ length: poly }, (_, i) => key + (i % 12) + Math.floor(i / 12) * 12);
      const pr = [];
      for (let k = 0; k < 3; k++) {
        for (const kk of keys) ev(0x80, kk, 0);
        drain(8);
        for (const kk of keys) ev(0x90, kk, 90);
        drain(blocks);
        const t = performance.now();
        drain(blocks);
        pr.push(performance.now() - t);
        for (const kk of keys) ev(0x80, kk, 0);
        drain(8);
      }
      polyMs = min(pr);
      polyPct = polyMs / (sec * 1000) * 100;
    }
    if (chordN > 1) {
      const keys = Array.from({ length: chordN }, (_, i) => key + i * 2);
      for (let k = 0; k < 4; k++) {
        for (const kk of keys) ev(0x80, kk, 0);
        drain(8);
        const t = performance.now();
        for (const kk of keys) ev(0x90, kk, 90);
        chord.push(performance.now() - t);
        for (const kk of keys) ev(0x80, kk, 0);
        drain(8);
      }
    }
    rows.push({
      note, program, key,
      coldMs: cold[0],
      warmMs: med(warm || [0]),
      tableMs: cold[0] - med(warm || [0]),
      renderMs, renderPct: renderMs / (sec * 1000) * 100,
      chordMs: chord.length ? min(chord) : null,
      chordPerNoteMs: chord.length ? min(chord) / chordN : null,
      poly, polyMs, polyPct,
      polyPerNotePct: polyPct !== null ? polyPct / poly : null,
    });
  }
  Module._SourceFree(src);
  if (opts.json) { console.log(JSON.stringify(rows, null, 2)); return; }
  console.log(`--- цена, ${wasmJs} (${SR} Гц, блок ${BLOCK})`);
  console.log("  нота   сборка таблицы   NoteOn(тёплая)   рендер: мс на " + sec + " с   % реалтайма"
    + (chordN > 1 ? `   аккорд ${chordN} нот (на ноту)` : "")
    + (rows[0].poly > 1 ? `   ${rows[0].poly} нот разом: % реалтайма (на ноту)` : ""));
  for (const r of rows)
    console.log(`  ${pad(r.note, 7)} ${pad(r.tableMs.toFixed(2), 8)} мс ${pad(r.warmMs.toFixed(2), 12)} мс ${pad(r.renderMs.toFixed(1), 15)} мс ${pad(r.renderPct.toFixed(2), 10)} %`
      + (chordN > 1 ? `   ${r.chordMs.toFixed(2)} мс (${r.chordPerNoteMs.toFixed(2)})` : "")
      + (r.polyMs !== null ? `   ${r.polyPct.toFixed(1)} % (${r.polyPerNotePct.toFixed(3)})` : ""));
}

function selftest() {
  let fail = 0;
  const check = (name, got, want, tol) => {
    const ok = Math.abs(got - want) <= tol;
    if (!ok) fail++;
    console.log(`  ${ok ? "OK  " : "ОШИБ"} ${name}: получено ${got.toFixed(4)}, ожидалось ${want.toFixed(4)} ±${tol}`);
  };
  // 1) БПФ против наивного ДПФ
  const n = 16, x = Float64Array.from({ length: n }, () => Math.random() - 0.5);
  const re = Float64Array.from(x), im = new Float64Array(n);
  fftInPlace(re, im);
  const { re: nr, im: ni } = naiveDft(x);
  let maxErr = 0;
  for (let i = 0; i < n; i++) maxErr = Math.max(maxErr, Math.abs(re[i] - nr[i]), Math.abs(im[i] - ni[i]));
  console.log(`  ${maxErr < 1e-9 ? "OK  " : "ОШИБ"} fft/наивное ДПФ: max|Δ| = ${maxErr.toExponential(2)}`);
  if (maxErr >= 1e-9) fail++;
  // 2) round-trip БПФ→ОБПФ
  const rr = Float64Array.from(x), ii = new Float64Array(n);
  fftInPlace(rr, ii);
  ifftInPlace(rr, ii);
  let rtErr = 0;
  for (let i = 0; i < n; i++) rtErr = Math.max(rtErr, Math.abs(rr[i] - x[i]));
  console.log(`  ${rtErr < 1e-9 ? "OK  " : "ОШИБ"} round-trip БПФ: max|Δ| = ${rtErr.toExponential(2)}`);
  if (rtErr >= 1e-9) fail++;
  // 3) тракт «аналитический сигнал → центы» на синтетической ЧМ
  const F = 44100, f = 261.63, N = F * 3, rate = 6, peakCents = 7;
  let ph = 0;
  const y = new Float64Array(N);
  for (let i = 0; i < N; i++) {
    const t = i / F;
    const freq = f * Math.pow(2, (peakCents / 1200) * Math.sin(2 * Math.PI * rate * t));
    ph += 2 * Math.PI * freq / F;
    y[i] = Math.sin(ph);
  }
  const rows = audit(y, f, { bands: harmonicBands(f, { kmax: 1 }), windows: { sustain: [0.5, 2.5] } });
  const w = rows[0].windows.sustain;
  check("синтетика: частота вибрато, Гц", w.fm.f, rate, 0.2);
  check("синтетика: глубина ЧМ, центы", w.fm.mag, peakCents, 0.4);
  // 4) detrend: чистый линейный тренд должен уйти в ноль (регрессия Update 65 —
  //    прежняя реализация делила на неверную длину окна и давала «×100000»)
  const M = 44100;
  const ramp = new Float64Array(M);
  for (let i = 0; i < M; i++) ramp[i] = i / M * 30 - 15;
  const rampOut = detrend(ramp, M, 0.25);
  let rampMax = 0;
  for (const v of rampOut) rampMax = Math.max(rampMax, Math.abs(v));
  console.log(`  ${rampMax < 1e-9 ? "OK  " : "ОШИБ"} detrend(линейный тренд): max|остаток| = ${rampMax.toExponential(2)}`);
  if (rampMax >= 1e-9) fail++;
  // 5) detrend не съедает быструю синусоиду в середине ряда
  const both = new Float64Array(M);
  for (let i = 0; i < M; i++) both[i] = i / M * 30 - 15 + 5 * Math.sin(2 * Math.PI * 9 * i / M);
  const mid = detrend(both, M, 0.25).slice(3000, M - 3000);
  check("detrend: rms быстрой синусоиды, дБ", rms(mid), 5 / Math.SQRT2, 0.6);
  console.log(fail ? `\nSELFTEST FAILED (${fail})` : "\nSELFTEST PASSED");
  process.exit(fail ? 1 : 0);
}

function printVib(tag, rows) {
  console.log(`--- ${tag}`);
  console.log("  h# |          раннее окно          |         сустейн");
  console.log("     |  ЧМ Гц/ц  |  АМ Гц/дБ  | rms дБ |  ЧМ Гц/ц  |  АМ Гц/дБ  | rms дБ");
  for (const r of rows) {
    const cell = (w) => w ? `${pad(w.fm.f.toFixed(2), 5)}/${pad(w.fm.mag.toFixed(1), 5)} | ${pad(w.am.f.toFixed(2), 5)}/${pad(w.am.mag.toFixed(2), 5)} | ${pad(w.amRms.toFixed(2), 6)}` : "        —";
    console.log(`  h${r.k} | ${cell(r.windows.early)} | ${cell(r.windows.sustain)}`);
  }
}

async function main() {
  const opts = parseArgs(process.argv.slice(2));
  const cmd = opts._[0];
  const notes = opts._.slice(1);
  const wasmJs = opts.wasm || DEFAULT_WASM_JS;
  const noteOn = opts.noteon !== undefined ? Number(opts.noteon) : DEFAULT_NOTE_ON;
  if (cmd === "selftest") return selftest();
  if (!["vib", "env", "attack", "mod", "trem", "period", "floor", "bands", "spec", "timbre", "pitch", "cost"].includes(cmd)) {
    console.log("Команды: selftest | vib | env | attack | mod | trem | period | floor | bands | spec | timbre | pitch | cost   (prog:key ...)");
    process.exit(2);
  }
  if (!notes.length) { console.log("Укажите ноты вида 75:60"); process.exit(2); }
  if (cmd === "cost") return costCommand(opts, notes);
  for (const note of notes) {
    const [program, key] = note.split(":").map(Number);
    const pair = await renderPair({ program, key, wasmJs, sf2: opts.sf2 || DEFAULT_SF2 });
    // Update 74: программа синтезатора и пресет банка — НЕ одно и то же
    // (43/115 — свои свободные слоты, см. BANK_PROGRAM в lib/render.mjs).
    // Печатаем подмену, чтобы «банк» в отчёте нельзя было прочитать неверно.
    if (pair.bank !== null && pair.bankProgram !== program)
      console.log(`(банк: программа синтезатора ${program} → пресет банка ${pair.bankProgram})`);
    if (cmd === "vib") {
      const kmax = Number(opts.harm || 6);
      const win = { early: [noteOn + 0.25, noteOn + 0.75], sustain: [noteOn + 1.5, noteOn + 4.2] };
      printVib(`наш  prog ${program} key ${key} (f0 ${f0of(key).toFixed(1)} Гц)`, audit(pair.ours, f0of(key), { kmax, windows: win }));
      if (pair.bank && !opts["no-bank"]) printVib(`банк prog ${program} key ${key}`, audit(pair.bank, f0of(key), { kmax, windows: win }));
      continue;
    }
    if (cmd === "env" || cmd === "attack") {
      const lenMs = Number(opts.len || (cmd === "attack" ? 2 : 25));
      const hopMs = Number(opts.hop || lenMs);
      const toMs = Number(opts.to || (cmd === "attack" ? 150 : 6000));
      const refLen = Number(opts.ref || 500);
      const refAt = noteOn + Number(opts.refat || 2.0);
      const show = (tag, x) => {
        const ref = referenceLevel(x, SR, refAt, refLen / 1000);
        const prof = levelWindows(x, SR, { lenMs, hopMs, fromSeconds: noteOn, toSeconds: noteOn + toMs / 1000 });
        console.log(`--- ${tag} (мс от note-on +${noteOn.toFixed(2)} с; дБ отн. ${(Number(opts.refat || 2.0)).toFixed(2)}-${(Number(opts.refat || 2.0) + refLen / 1000).toFixed(2)} с), окно ${lenMs} мс`);
        console.log("  " + prof.map((p) => `${Math.round(p.t)}:${(p.db - ref).toFixed(1)}`).join(" "));
      };
      show("наш ", pair.ours);
      if (pair.bank && !opts["no-bank"]) show("банк", pair.bank);
      continue;
    }
    if (cmd === "period") {
      // «Левый период»: автокорреляция сустейна. Лупнутая таблица длиной N
      // семплов даёт ВЫСОКУЮ корреляцию на лаге ровно N — _в этом и есть
      // периодический артефакт, который не объясняется ни ЧМ, ни АМ.
      const fromSec = noteOn + Number(opts.from || 1.5), toSec = noteOn + Number(opts.to || 4.0);
      const maxLag = Number(opts.maxlag || 96000);
      const show = (tag, x) => {
        const a = Math.round(fromSec * SR), b = Math.min(x.length, Math.round(toSec * SR));
        const seg = x.slice(a, b - Math.max(1, Math.min(maxLag, Math.floor((b - a) / 3))));
        const r = autocorrelation(seg);
        const minLag = Number(opts.minlag || 128);
        const peaks = [];
        for (let lag = minLag + 1; lag < Math.min(r.length - 1, maxLag); lag++)
          if (r[lag] > r[lag - 1] && r[lag] >= r[lag + 1]) peaks.push({ lag, r: r[lag] });
        peaks.sort((p, q) => q.r - p.r);
        const list = `32768:${r[32768]?.toFixed(4) ?? "—"} 65536:${r[65536]?.toFixed(4) ?? "—"}`;
        console.log(`--- ${tag} сустейн ${fromSec}-${(b / SR).toFixed(2)} с, `
          + `r@лаге таблицы: ${list}`);
        console.log("  топ лагов: " + peaks.slice(0, 10)
          .map((p) => `${(p.lag / SR * 1000).toFixed(1)}мс/${p.lag}/${p.r.toFixed(4)}`).join("  "));
      };
      show("наш ", pair.ours);
      if (pair.bank && !opts["no-bank"]) show("банк", pair.bank);
      continue;
    }
    if (cmd === "floor") {
      // Межгармонический пол (шум между партиалами) по окнам и срезам.
      const show = (tag, x) => {
        const rows = interHarmonicFloor(x, f0of(key), { sampleRate: SR });
        console.log(`--- ${tag} межгармонический пол, дБ отн. h1 (${f0of(key).toFixed(1)} Гц)`);
        for (const r of rows) console.log(`  ${(r.from * 1000).toFixed(0)}-${(r.to * 1000).toFixed(0)} мс: `
          + `всё ${r.floorDb.toFixed(1)} | ` + r.slices.map((s) => `${s.lo}-${s.hi}: ${s.db === null ? "—" : s.db.toFixed(1)}`).join(" | "));
      };
      show("наш ", pair.ours);
      if (pair.bank && !opts["no-bank"]) show("банк", pair.bank);
      continue;
    }
    if (cmd === "spec") {
      // Уровни гармоник, дБ отн. h1 — для «тембр не тот» (см. lib).
      const fromSec = noteOn + Number(opts.from || 1.4), toSec = noteOn + Number(opts.to || 4.0);
      const kmax = Number(opts.kmax || 16);
      const show = (tag, x) => {
        const r = harmonicLevels(x, f0of(key), { sampleRate: SR, fromSec, toSec, kmax });
        if (!r) { console.log(`--- ${tag}: мало данных`); return; }
        console.log(`--- ${tag} гармоники ${(fromSec - noteOn).toFixed(1)}-${(toSec - noteOn).toFixed(1)} с от note-on, дБ отн. h1`);
        console.log("  " + r.db.map((v, i) => `h${i + 1} ${v.toFixed(1)}`).join(" | "));
      };
      show("наш ", pair.ours);
      if (pair.bank && !opts["no-bank"]) show("банк", pair.bank);
      continue;
    }
    if (cmd === "bands") {
      // Спектр воздуха по абсолютным полосам, дБ отн. пика h1 (см. lib).
      const fromSec = noteOn + Number(opts.from || 1.4), toSec = noteOn + Number(opts.to || 4.0);
      const show = (tag, x) => {
        const r = bandNoiseFloor(x, f0of(key), { sampleRate: SR, fromSec, toSec });
        if (!r) { console.log(`--- ${tag}: мало данных`); return; }
        console.log(`--- ${tag} воздух ${(fromSec - noteOn).toFixed(1)}-${(toSec - noteOn).toFixed(1)} с от note-on, дБ отн. пика h1 (кадров ${r.frames})`);
        console.log("  " + r.bands.map((b) => `${b.lo}-${b.hi}Гц: ${b.db === null ? "—" : b.db.toFixed(1)}`).join(" | "));
      };
      show("наш ", pair.ours);
      if (pair.bank && !opts["no-bank"]) show("банк", pair.bank);
      continue;
    }
    if (cmd === "timbre") {
      // Тембр и дыхание ПО ВРЕМЕНИ: короткие окна от note-on. Видно и «выдох»
      // (пол между партиалами над полкой сустейна), и эволюцию тембра атаки.
      const kmax = Number(opts.harm || 6);
      const lenSec = Number(opts.len || 0.06);
      const hopSec = Number(opts.hop || lenSec / 3);
      const fromSec = noteOn + Number(opts.from || 0);
      const toSec = noteOn + Number(opts.to || 0.6);
      const showT = (tag, x) => {
        const rows = timbreTrack(x, f0of(key), { sampleRate: SR, kmax, lenSec, hopSec, fromSec, toSec });
        if (!rows) { console.log(`--- ${tag}: мало данных`); return; }
        console.log(`--- ${tag} тембр/дыхание, окно ${(lenSec * 1000).toFixed(0)} мс, дБ отн. h1 сустейна (время от note-on, мс)`);
        console.log(`  мс   ` + Array.from({ length: kmax }, (_, i) => `h${i + 1}`.padStart(7)).join(" ")
          + "  |" + rows[0].bands.map((b) => `${b.lo}-${b.hi}Гц`.padStart(12)).join(" "));
        for (const r of rows)
          console.log(`  ${(r.t0 * 1000).toFixed(0).padStart(4)} ` + r.h.map((v) => v.toFixed(1).padStart(7)).join(" ")
            + "  |" + r.bands.map((b) => (b.db === null ? "—" : b.db.toFixed(1)).padStart(12)).join(" "));
      };
      showT("наш ", pair.ours);
      if (pair.bank && !opts["no-bank"]) showT("банк", pair.bank);
      continue;
    }
    if (cmd === "pitch") {
      // Высота по коротким окнам: видно «подъезд» к ноте и провалы в атаке.
      const lenSec = Number(opts.len || 0.06);
      const hopSec = Number(opts.hop || lenSec / 3);
      const kmax = Number(opts.harm || 6);
      const fromSec = noteOn + Number(opts.from || 0);
      const toSec = noteOn + Number(opts.to || 0.6);
      const showP = (tag, x) => {
        const rows = pitchTrack(x, f0of(key), { sampleRate: SR, kmax, lenSec, hopSec, fromSec, toSec });
        console.log(`--- ${tag} высота тона, окно ${(lenSec * 1000).toFixed(0)} мс, центы отн. номинала (время от note-on, мс)`);
        console.log("  " + rows.map((r) => `${(r.t0 * 1000).toFixed(0)}:${r.cents >= 0 ? "+" : ""}${r.cents.toFixed(1)}(h${r.lead},${r.used})`).join(" "));
      };
      showP("наш ", pair.ours);
      if (pair.bank && !opts["no-bank"]) showP("банк", pair.bank);
      continue;
    }
    if (cmd === "trem") {
      // --wide lo,hi — ШИРОКАЯ полоса (в Гц): видит только настоящую АМ.
      // Узкая полоса вокруг гармоники превращает ЧМ в АМ (см. wideAmTrack).
      if (opts.wide) {
        const [wlo, whi] = String(opts.wide).split(",").map(Number);
        const blockSec = Number(opts.block || 0.4), stepSec = Number(opts.step || blockSec / 2);
        const toSec = Number(opts.to || 3.0);
        const [fLo, fHi] = String(opts.band || "1.5,12").split(",").map(Number);
        const showWide = (tag, x) => {
          const tr = wideAmTrack(x, { lo: wlo, hi: whi },
            { sampleRate: SR, blockSec, stepSec, fromSec: noteOn, toSec: noteOn + toSec, fLo, fHi });
          console.log(`--- ${tag} АМ широкой полосы ${wlo}-${whi} Гц, блок ${blockSec} с (время от note-on)`);
          console.log("  " + tr.map((r) =>
            `${r.t0.toFixed(2)}с: rms ${r.amRms.toFixed(2)}дБ / ${r.peak.f.toFixed(2)}Гц ${r.peak.mag.toFixed(2)}дБ`).join("\n  "));
        };
        showWide("наш ", pair.ours);
        if (pair.bank && !opts["no-bank"]) showWide("банк", pair.bank);
        continue;
      }
      // Когда ПРИХОДИТ тремоло: rms АМ по блокам + когерентный пик
      const k = Number(opts.harm || 1);
      const blockSec = Number(opts.block || 0.25);
      const stepSec = Number(opts.step || blockSec / 2);
      const toSec = Number(opts.to || 3.0);
      const [fLo, fHi] = String(opts.band || "1.5,12").split(",").map(Number);
      const show = (tag, x) => {
        const band = harmonicBands(f0of(key), { kmax: k })[k - 1];
        const tr = tremorTrack(x, band, { sampleRate: SR, blockSec, stepSec, fromSec: noteOn, toSec: noteOn + toSec, fLo, fHi });
        console.log(`--- ${tag} h${k} (${band.fc.toFixed(0)} Гц), блок ${blockSec} с / шаг ${stepSec} с, полоса АМ ${fLo}-${fHi} Гц (время от note-on)`);
        console.log("  " + tr.map((r) => `${r.t0.toFixed(2)}с: rms ${r.amRms.toFixed(2)}дБ / ${r.peak.f.toFixed(2)}Гц ${r.peak.mag.toFixed(2)}дБ`).join("\n  "));
      };
      show("наш ", pair.ours);
      if (pair.bank && !opts["no-bank"]) show("банк", pair.bank);
      continue;
    }
    if (cmd === "mod") {
      // «Левые» периоды: модуляционный спектр огибающей ноты (0.05-15 Гц по умолчанию)
      const [lo, hi] = String(opts.band || "0.05,15").split(",").map(Number);
      // `--from` — начало окна в секундах от note-on. По умолчанию 0.25 с, но
      // атака пан-флейты длится дольше, и на окне с атакой RMS уезжает в
      // десятки дБ (атака — это НЕ тремоло). Для скорости модуляции брать
      // `--from 0.9`, чтобы окно целиком лежало в сустейне.
      const fromSec = noteOn + Number(opts.from || 0.25);
      const show = (tag, x) => {
        const amp = envelope(x, SR, 60);
        const relDb = new Float64Array(amp.length);
        for (let i = 0; i < amp.length; i++) relDb[i] = db(amp[i]);
        const det = detrend(relDb, SR, 0.5);
        const b = Math.min(x.length, Math.round(noteOn * SR) + Math.round(4.5 * SR));
        const seg = det.slice(Math.round(fromSec * SR), b);
        const ps = modPeaks(seg, SR, lo, hi, 8);
        const c = modCentroid(seg, SR, lo, hi);
        console.log(`--- ${tag}: модуляция огибающей ${(fromSec - noteOn).toFixed(2)}-${((b - Math.round(noteOn * SR)) / SR).toFixed(2)} с от note-on, полоса ${lo}-${hi} Гц`);
        console.log(`  эффективная скорость (центроид) ${c ? c.f.toFixed(2) : "—"} Гц, глубина в полосе ${c ? c.depthDb.toFixed(3) : "—"} дБ`);
        console.log("  " + ps.map((p) => `${p.f.toFixed(2)}Гц/${p.mag.toFixed(3)}дБ`).join("  "));
      };
      show("наш ", pair.ours);
      if (pair.bank && !opts["no-bank"]) show("банк", pair.bank);
      continue;
    }
  }
}

main().catch((e) => { console.error(e); process.exit(1); });
