"use strict";
// Рендер ноты нашим WASM и банком (fluidsynth) + чтение WAV. Единая точка для
// всех зондов: раньше MIDI-билдер и чтение WAV копипастились в каждый файл.
import fs from "node:fs";
import path from "node:path";
import { execFileSync } from "node:child_process";

export const SR = 44100;
export const DEFAULT_SF2 = "/tmp/sf2extract/Titanic 200 GM-GS v1.2.sf2";
export const DEFAULT_WASM_JS = "web/generated/IntraSynth.js";
export const DEFAULT_NOTE_ON = 0.2;

/// Update 74: СООТВЕТСТВИЕ программы синтезатора программе БАНКА.
///
/// Грабли, на которые ушли Updates 67-73: «FluteClean» и «FluteHybrid» живут
/// на СВОБОДНЫХ слотах 43 и 115, а в банке Titanic это совсем другие пресеты
/// (43 = Contra Bass, 115 = Woodblock). Зонды же рендерили `renderBank` с ТОЙ ЖЕ
/// программой, что и синтезатор, — то есть все замеры «эталона» для флейты
/// делались по контрабасу. Владелец услышал это в панели A/B («звук
/// посторонний») и подтвердил эталон: вкладка «Flute» первого спойлера = банк
/// prog 73.
///
/// Здесь то же соответствие задано в ОДНОМ месте, чтобы зонд, CLI и панель
/// A/B никогда не расходились. Проверка: `samples/Flute/C4.wav` (prog 73)
/// совпадает с `renderBank({program: 73})` по h2..h12 в пределах 0.5 дБ.
export const BANK_PROGRAM = {
  43: 73,  // FluteClean  (Titanic «Flute»)
  115: 73, // FluteHybrid (тот же эталон)
  77: 78,  // у синтезатора 77 = Whistle, в банке 77 = Shakuhachi, 78 = Whistle
};
export const bankProgramFor = (program) => BANK_PROGRAM[program] ?? program;

function vlq(v) {
  const o = [v & 0x7f];
  while ((v >>= 7) > 0) o.push(0x80 | (v & 0x7f));
  return Buffer.from(o.reverse());
}
function eventBytes(events) {
  const body = [Buffer.from([0, 0xff, 0x51, 3, 0x0f, 0x42, 0x40])];
  let p = 0;
  for (const e of events) {
    const t = Math.round(e.t * 480);
    body.push(Buffer.concat([vlq(t - p), Buffer.from(e.m)]));
    p = t;
  }
  body.push(Buffer.from([0, 0xff, 0x2f, 0]));
  const tb = Buffer.concat(body);
  const h = Buffer.alloc(14);
  h.write("MThd", 0);
  h.writeUInt32BE(6, 4);
  h.writeUInt16BE(0, 8);
  h.writeUInt16BE(1, 10);
  h.writeUInt16BE(480, 12);
  const tr = Buffer.alloc(8 + tb.length);
  tr.write("MTrk", 0);
  tr.writeUInt32BE(tb.length, 4);
  tb.copy(tr, 8);
  return Buffer.concat([h, tr]);
}

/// MIDI одного органного пункта: noteOn в noteOn, noteOff в noteOff.
export function buildMidi({ program, key, noteOn = DEFAULT_NOTE_ON, noteOff = 4.4 }) {
  return eventBytes([
    { t: 0, m: [0xc0, program] },
    { t: noteOn, m: [0x90, key, 100] },
    { t: noteOff, m: [0x80, key, 0] },
  ]);
}

/// Мелодия: [{key, at, off}] — для проверок аккордов/секвенций.
export function buildMidiMelody({ program, notes, noteOn = 0 }) {
  const events = [{ t: 0, m: [0xc0, program] }];
  for (const n of notes) {
    events.push({ t: n.at, m: [0x90, n.key, n.vel ?? 100] });
    events.push({ t: n.off, m: [0x80, n.key, 0] });
  }
  events.sort((a, b) => a.t - b.t);
  return eventBytes(events);
}

/// Рендер нашим WASM (левый канал). wasmJs — путь к IntraSynth.js.
export async function renderOurs({ program, key, sampleRate = SR, wasmJs = DEFAULT_WASM_JS, noteOn, noteOff }) {
  const { default: Factory } = await import(path.resolve(wasmJs));
  const M = await Factory();
  const bytes = buildMidi({ program, key, noteOn, noteOff });
  const mp = M._malloc(bytes.length);
  M.HEAPU8.set(bytes, mp);
  const src = M._SourceCreateFromMidiFileData(mp, bytes.length, sampleRate, 2);
  M._free(mp);
  const BLK = 4096, out = [];
  for (;;) {
    const ptr = M._malloc(BLK * 2 * 4);
    const got = M._SourceGetUninterleavedSamples(src, ptr, BLK, BLK);
    if (got <= 0) { M._free(ptr); break; }
    const off = ptr >> 2;
    for (let i = 0; i < got; i++) out.push(M.HEAPF32[off + i]);
    M._free(ptr);
    if (got < BLK) break;
  }
  M._SourceFree(src);
  return Float64Array.from(out);
}

/// Чтение WAV (все каналы → первый).
export function readWavMono(file) {
  const wav = fs.readFileSync(file);
  const view = new DataView(wav.buffer, wav.byteOffset, wav.byteLength);
  let pos = 12, dataOff = 0, dataLen = 0, channels = 1, bits = 16;
  while (pos < wav.length - 8) {
    const id = wav.toString("ascii", pos, pos + 4);
    const len = view.getUint32(pos + 4, true);
    if (id === "fmt ") { channels = view.getUint16(pos + 10, true); bits = view.getUint16(pos + 22, true); }
    else if (id === "data") { dataOff = pos + 8; dataLen = len; break; }
    pos += 8 + len + (len & 1);
  }
  const bytesPer = bits / 8, stride = bytesPer * channels;
  const n = Math.floor(dataLen / stride);
  const x = new Float64Array(n);
  for (let i = 0; i < n; i++) {
    const o = dataOff + i * stride;
    x[i] = bits === 32 ? view.getFloat32(o, true) : view.getInt16(o, true) / 32768;
  }
  return x;
}

/// Рендер банка (fluidsynth, без реверба и хоруса). sf2 по умолчанию — Titanic.
/// `program` — программа СИНТЕЗАТОРА: к банку применяется bankProgramFor()
/// (см. BANK_PROGRAM). Отключить подмену: `rawProgram: true`.
export function renderBank({ program, key, sampleRate = SR, sf2 = DEFAULT_SF2, noteOn, noteOff, tag = "_an", rawProgram = false, gain = 0.2 }) {
  const bankProg = rawProgram ? program : bankProgramFor(program);
  const mid = path.join(".scratch", `${tag}_${bankProg}_${key}.mid`);
  const wav = path.join("/tmp", `${tag}_${bankProg}_${key}.wav`);
  fs.mkdirSync(".scratch", { recursive: true });
  fs.writeFileSync(mid, buildMidi({ program: bankProg, key, noteOn, noteOff }));
  // FluidSynth master gain is deliberately low by default to leave polyphony
  // headroom.  It is NOT an SF2 instrument-level property.  Relative program
  // calibration must use ratios (program/AGP), where this gain cancels.
  execFileSync("fluidsynth", ["-ni", "-g", String(gain), "-r", String(sampleRate), "-F", wav, "-R", "0", "-C", "0", sf2, mid], { stdio: "ignore" });
  return readWavMono(wav);
}

/// Удобный «A/B»: возвращает { ours, bank } (bank — null, если рендер упал).
export async function renderPair({ program, key, sampleRate = SR, wasmJs = DEFAULT_WASM_JS, sf2 = DEFAULT_SF2, noteOn, noteOff }) {
  const ours = await renderOurs({ program, key, sampleRate, wasmJs, noteOn, noteOff });
  let bank = null;
  let bankProgram = bankProgramFor(program);
  try { bank = renderBank({ program, key, sampleRate, sf2, noteOn, noteOff }); } catch { /* банка может не быть */ }
  return { ours, bank, bankProgram };
}

/// Выравнивание громкости по RMS СУСТЕЙНА (Update 73).
/// Зачем: в панели A/B банк был на 6-10 дБ тише наших рендеров, потому что
/// fluidsynth отдаёт семпл как есть, а наш выход громче. Владелец:
/// «у банка Титаник почему-то очень тихо». Для сравнения ТЕМБРА обе стороны
/// приводятся к одной громкости по сустейну (не по пику: у банка атака
/// импульсная, и выравнивание по пику сделало бы сустейн тихим, а именно
/// его и сравнивают). Пик после нормировки страхуется потолком.
export function normalizeSustainRms(x, {
  sampleRate = SR, fromSec = 0.6, toSec = 2.4, targetDb = -20, peakCeil = 0.97,
} = {}) {
  const a = Math.max(0, Math.round(fromSec * sampleRate));
  const b = Math.min(x.length, Math.round(toSec * sampleRate));
  if (b - a < sampleRate * 0.1) return x;
  let acc = 0, peak = 0;
  for (let i = a; i < b; i++) acc += x[i] * x[i];
  const rms = Math.sqrt(acc / (b - a));
  let gain = rms > 1e-12 ? Math.pow(10, targetDb / 20) / rms : 1;
  for (let i = 0; i < x.length; i++) { const v = Math.abs(x[i]) * gain; if (v > peak) peak = v; }
  if (peak > peakCeil) gain *= peakCeil / peak;
  const out = new Float64Array(x.length);
  for (let i = 0; i < x.length; i++) out[i] = x[i] * gain;
  return out;
}

/// Запись моно-WAV 16 бит (для A/B-файлов, которые слушает владелец).
export function writeWavMono(file, x, sampleRate = SR) {
  const n = x.length, buf = Buffer.alloc(44 + n * 2);
  buf.write("RIFF", 0); buf.writeUInt32LE(36 + n * 2, 4); buf.write("WAVE", 8);
  buf.write("fmt ", 12); buf.writeUInt32LE(16, 16); buf.writeUInt16LE(1, 20);
  buf.writeUInt16LE(1, 22); buf.writeUInt32LE(sampleRate, 24);
  buf.writeUInt32LE(sampleRate * 2, 28); buf.writeUInt16LE(2, 32); buf.writeUInt16LE(16, 34);
  buf.write("data", 36); buf.writeUInt32LE(n * 2, 40);
  for (let i = 0; i < n; i++) buf.writeInt16LE(Math.round(Math.max(-1, Math.min(1, x[i])) * 32767), 44 + i * 2);
  fs.writeFileSync(file, buf);
  return buf.length;
}
