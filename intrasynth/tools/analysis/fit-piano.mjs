"use strict";
// Единый офлайн-фиттер sustain-части акустического пианино из SF2.
//
// Цель: не собирать Amp и Decay разными несогласованными зондами. Для каждой
// гармоники выбирается ОДНА фиксированная спектральная линия, затем её L/R
// энергия отслеживается во времени. Из одной траектории одновременно получаем:
//   - Amp в PianoRegionData::DecayOnset;
//   - Decay1..4 на существующих границах сегментов;
//   - диагностику частоты partial и остаточной модуляции/биений.
//
// Атака до DecayOnset намеренно НЕ фитится. Phase/FreqRatio по умолчанию тоже
// не переписываются: phase тесно связан с будущей моделью удара, а частоты уже
// были хорошо измерены прежним fitter'ом. `--fit-frequency` включает обновление
// FreqRatio отдельно.
//
// Использование:
//   node intrasynth/tools/analysis/fit-piano.mjs \
//     --sf2 "/tmp/sf2/Titanic 200 GM-GS v1.2.sf2" \
//     --header intrasynth/src/Intra/Synth/PianoRegions.h \
//     --out .scratch/piano-joint-fit.json
//
//   # Записать полученную packed-таблицу в отдельный header для diff/review:
//   node .../fit-piano.mjs ... --write-header .scratch/PianoRegions.joint.h
//
// Зависимости: только Node.js и общий проверенный БПФ из ./lib/fft.mjs.
import fs from "node:fs";
import path from "node:path";
import { fftInPlace } from "./lib/fft.mjs";

const DB_PER_NEPER = 20 / Math.log(10); // 8.685889638...
const LOG_Q_DB = 20 * Math.log10(2) / 128;
const FIELD_SHIFT = { amp: 14n, d1: 25n, d2: 36n, d3: 47n, d4: 58n };
const FIELD_MASK = 0x7ffn;
const DECAY_SCALE = [2621.4, 2621.4, 5461.25, 5461.25];

function argsOf(argv) {
  const o = {};
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (!a.startsWith("--")) continue;
    const k = a.slice(2);
    if (i + 1 < argv.length && !argv[i + 1].startsWith("--")) o[k] = argv[++i];
    else o[k] = true;
  }
  return o;
}
const median = (a) => {
  if (!a.length) return NaN;
  const b = Array.from(a).sort((x, y) => x - y), n = b.length;
  return n & 1 ? b[n >> 1] : 0.5 * (b[n / 2 - 1] + b[n / 2]);
};
const clamp = (x, a, b) => Math.max(a, Math.min(b, x));

function riffSubchunks(buf, start, end) {
  const out = [];
  for (let p = start; p + 8 <= end;) {
    const id = buf.toString("ascii", p, p + 4), size = buf.readUInt32LE(p + 4);
    const d0 = p + 8, d1 = d0 + size;
    if (d1 > end) throw new Error(`Обрезанный RIFF chunk ${id}`);
    out.push({ id, d0, d1, listType: id === "LIST" ? buf.toString("ascii", d0, d0 + 4) : null });
    p = d1 + (size & 1);
  }
  return out;
}

function readSf2(file) {
  const buf = fs.readFileSync(file);
  if (buf.toString("ascii", 0, 4) !== "RIFF" || buf.toString("ascii", 8, 12) !== "sfbk")
    throw new Error(`${file}: не SF2`);
  const riffEnd = Math.min(buf.length, 8 + buf.readUInt32LE(4));
  const top = riffSubchunks(buf, 12, riffEnd);
  const sdta = top.find((c) => c.id === "LIST" && c.listType === "sdta");
  const pdta = top.find((c) => c.id === "LIST" && c.listType === "pdta");
  if (!sdta || !pdta) throw new Error("SF2 без sdta/pdta");
  const ssub = riffSubchunks(buf, sdta.d0 + 4, sdta.d1);
  const psub = riffSubchunks(buf, pdta.d0 + 4, pdta.d1);
  const smpl = ssub.find((c) => c.id === "smpl"), shdr = psub.find((c) => c.id === "shdr");
  if (!smpl || !shdr) throw new Error("SF2 без smpl/shdr");
  const samples = new Int16Array(buf.buffer, buf.byteOffset + smpl.d0, Math.floor((smpl.d1 - smpl.d0) / 2));
  const headers = [];
  for (let o = shdr.d0; o + 46 <= shdr.d1 - 46; o += 46) {
    const name = buf.toString("latin1", o, o + 20).replace(/\0.*$/, "");
    headers.push({
      name, start: buf.readUInt32LE(o + 20), end: buf.readUInt32LE(o + 24),
      startLoop: buf.readUInt32LE(o + 28), endLoop: buf.readUInt32LE(o + 32),
      sampleRate: buf.readUInt32LE(o + 36), originalPitch: buf[o + 40],
      pitchCorrection: buf.readInt8(o + 41), sampleLink: buf.readUInt16LE(o + 42),
      sampleType: buf.readUInt16LE(o + 44),
    });
  }
  return { buf, samples, headers };
}

function parseHeader(file) {
  const text = fs.readFileSync(file, "utf8");
  // Current compact PianoRegionData:
  // {RootKey, PartCount, PartOffset, F0, AttackT, DecayOnset, SampleLen, StereoRatioRtoL}.
  // Decay segment durations are shared by all regions in the runtime.
  const rm = [...text.matchAll(/^\s*\{(\d+),\s*(\d+),\s*(\d+),\s*([0-9.]+)f,\s*([0-9.]+)f,\s*([0-9.]+)f,\s*([0-9.]+)f,\s*([0-9.]+)f\},?$/gm)];
  const regions = rm.slice(0, 25).map((m) => ({
    root: +m[1], count: +m[2], offset: +m[3], f0: +m[4], attackT: +m[5], decayOnset: +m[6],
    segT: 0.235, segT2: 0.550, segT3: 0.900, sampleLen: +m[7], stereo: +m[8],
  }));
  if (regions.length !== 25) throw new Error(`Ожидалось 25 acoustic regions, найдено ${regions.length}`);
  const tm = text.match(/static const uint8 PianoAllPartialsPacked\[\] = \{([\s\S]*?)\n\};/);
  if (!tm) throw new Error("Не найдена PianoAllPartialsPacked");
  const packed = Uint8Array.from([...tm[1].matchAll(/0x([0-9a-fA-F]{2})/g)], (m) => parseInt(m[1], 16));
  if (packed.length !== 544 * 11) throw new Error(`Packed table: ${packed.length}, ожидалось ${544 * 11}`);
  return { text, regions, packed };
}

function raw88(packed, row) {
  const o = row * 11;
  let v = 0n;
  for (let i = 0; i < 11; i++) v |= BigInt(packed[o + i]) << BigInt(8 * i);
  return v;
}
function qField(packed, row, name) { return Number((raw88(packed, row) >> FIELD_SHIFT[name]) & FIELD_MASK); }
function unpackLog(q) { return q === 0 ? 0 : Math.round(Math.pow(2, (q - 1) / 128)); }
function packLog(v) { return v <= 0 ? 0 : clamp(1 + Math.round(128 * Math.log2(v)), 1, 2047); }
function decodeRow(packed, row) {
  const v = raw88(packed, row);
  return {
    k: Number(v & 0x3fn), phase: Number((v >> 6n) & 0xffn),
    ampQ: Number((v >> 14n) & 0x7ffn), dQ: [25n, 36n, 47n, 58n].map((s) => Number((v >> s) & 0x7ffn)),
    freqQ: Number((v >> 69n) & 0xffffn),
  };
}
function setBits(packed, row, shift, bits, value) {
  const o = row * 11, mask = ((1n << BigInt(bits)) - 1n) << BigInt(shift);
  let v = raw88(packed, row);
  v = (v & ~mask) | ((BigInt(value) << BigInt(shift)) & mask);
  for (let i = 0; i < 11; i++) packed[o + i] = Number((v >> BigInt(8 * i)) & 0xffn);
}

function detectOnset(x, sr) {
  if (!x.length) return 0;
  const win = Math.max(32, Math.round(0.005 * sr)), hop = Math.max(1, win >> 1);
  const n = Math.min(1 + Math.max(0, Math.floor((x.length - win) / hop)), Math.max(1, Math.floor(0.25 * sr / hop)));
  const r = new Float64Array(n); let peak = 0;
  for (let i = 0; i < n; i++) {
    let s = 0; const a = i * hop;
    for (let j = 0; j < win; j++) { const v = x[a + j] / 32768; s += v * v; }
    r[i] = Math.sqrt(s / win + 1e-24); peak = Math.max(peak, r[i]);
  }
  if (peak <= 1e-12) return 0;
  const th = peak * Math.pow(10, -35 / 20);
  for (let i = 0; i < n; i++) if (r[i] >= th) return i * hop;
  return 0;
}

function solveLinear(A, b) {
  const n = b.length, m = Array.from({ length: n }, (_, i) => [...A[i], b[i]]);
  for (let c = 0; c < n; c++) {
    let p = c; for (let r = c + 1; r < n; r++) if (Math.abs(m[r][c]) > Math.abs(m[p][c])) p = r;
    [m[c], m[p]] = [m[p], m[c]];
    if (Math.abs(m[c][c]) < 1e-12) continue;
    const q = m[c][c]; for (let j = c; j <= n; j++) m[c][j] /= q;
    for (let r = 0; r < n; r++) if (r !== c) {
      const f = m[r][c]; if (!f) continue;
      for (let j = c; j <= n; j++) m[r][j] -= f * m[c][j];
    }
  }
  return m.map((r, i) => Number.isFinite(r[n]) ? r[n] : 0);
}
function pieceDurations(t, r) {
  const b1 = r.decayOnset + r.segT, b2 = b1 + r.segT2, b3 = b2 + r.segT3;
  return [clamp(t - r.decayOnset, 0, r.segT), clamp(t - b1, 0, r.segT2), clamp(t - b2, 0, r.segT3), Math.max(0, t - b3)];
}
function segId(t, r) {
  const b1 = r.decayOnset + r.segT, b2 = b1 + r.segT2, b3 = b2 + r.segT3;
  return t < b1 ? 0 : t < b2 ? 1 : t < b3 ? 2 : 3;
}
function baselineRates(packed, row) {
  const q = decodeRow(packed, row).dQ;
  return q.map((x, i) => unpackLog(x) / DECAY_SCALE[i] * DB_PER_NEPER);
}

function robustPieceFit(times, ys, r, baseRates) {
  let pts = [];
  for (let i = 0; i < times.length; i++) if (Number.isFinite(ys[i]) && times[i] >= r.decayOnset && times[i] <= r.sampleLen - 0.08)
    pts.push({ t: times[i], y: ys[i] });
  if (pts.length < 10) return null;
  let early = pts.filter((p) => p.t <= Math.min(r.decayOnset + 0.5, r.sampleLen - 0.1)).map((p) => p.y).sort((a, b) => a - b);
  if (early.length < 3) early = pts.slice(0, 10).map((p) => p.y).sort((a, b) => a - b);
  const top = median(early.slice(-Math.min(5, early.length))), floor = top - 58;
  pts = pts.filter((p) => p.y > floor); if (pts.length < 10) return null;
  const counts = [0, 0, 0, 0], bySeg = [[], [], [], []];
  for (const p of pts) { const s = segId(p.t, r); counts[s]++; bySeg[s].push(p.y); }
  const active = counts.map((n, j) => n >= 4 && (!bySeg[j].length || median(bySeg[j]) >= top - 52));
  const vars = ["L0", ...active.map((x, i) => x ? i : -1).filter((i) => i >= 0)];
  const fixed = baseRates.slice(); let beta = [median(pts.slice(0, 4).map((p) => p.y)), ...vars.slice(1).map((j) => baseRates[j])];
  let robust = new Float64Array(pts.length).fill(1);
  for (let iter = 0; iter < 8; iter++) {
    const n = vars.length, ATA = Array.from({ length: n }, () => Array(n).fill(0)), ATb = Array(n).fill(0);
    for (let pi = 0; pi < pts.length; pi++) {
      const p = pts[pi], d = pieceDurations(p.t, r), s = segId(p.t, r);
      let yy = p.y;
      for (let j = 0; j < 4; j++) if (!active[j]) yy += d[j] * fixed[j];
      const x = [1, ...vars.slice(1).map((j) => -d[j])];
      const balance = Math.sqrt(pts.length / Math.max(1, 4 * counts[s]));
      const w = balance * robust[pi];
      for (let a = 0; a < n; a++) { ATb[a] += w * x[a] * yy; for (let b = 0; b < n; b++) ATA[a][b] += w * x[a] * x[b]; }
    }
    beta = solveLinear(ATA, ATb);
    const rates = fixed.slice(); vars.slice(1).forEach((j, i) => rates[j] = clamp(beta[i + 1], 0, 180));
    beta[0] = pts.reduce((s, p) => s + p.y + pieceDurations(p.t, r).reduce((q, d, j) => q + d * rates[j], 0), 0) / pts.length;
    const residual = pts.map((p) => beta[0] - pieceDurations(p.t, r).reduce((q, d, j) => q + d * rates[j], 0) - p.y);
    for (let i = 0; i < residual.length; i++) { const a = Math.abs(residual[i]); robust[i] = a <= 1.7 ? 1 : 1.7 / a; }
  }
  const rates = fixed.slice(); vars.slice(1).forEach((j, i) => rates[j] = clamp(beta[i + 1], 0, 180));
  const abs = pts.map((p) => Math.abs(beta[0] - pieceDurations(p.t, r).reduce((q, d, j) => q + d * rates[j], 0) - p.y));
  const sorted = abs.slice().sort((a, b) => a - b);
  // Quality-gate the replacement against the currently packed model on the
  // exact same trajectory. Convergence alone is not enough for sparse/noisy
  // upper partials.
  const baseL0 = median(pts.map((p) => p.y + pieceDurations(p.t, r).reduce((q, d, j) => q + d * baseRates[j], 0)));
  const baseAbs = pts.map((p) => Math.abs(baseL0 - pieceDurations(p.t, r).reduce((q, d, j) => q + d * baseRates[j], 0) - p.y));
  const baseSorted = baseAbs.slice().sort((a, b) => a - b);
  const maeDb = median(abs), p90Db = sorted[Math.floor(0.9 * (sorted.length - 1))];
  const baseMaeDb = median(baseAbs), baseP90Db = baseSorted[Math.floor(0.9 * (baseSorted.length - 1))];
  const accepted = maeDb <= 4.0 && p90Db <= 10.0 && (maeDb + 0.10 < baseMaeDb || p90Db + 0.25 < baseP90Db);
  return { L0: beta[0], rates, active, n: pts.length, maeDb, p90Db, baseMaeDb, baseP90Db, accepted, topDb: top, floorDb: floor };
}

function spectraForPair(samples, hl, hr, onset, r) {
  const N = 4096, hop = 1024, bins = N / 2 + 1, sr = hl.sampleRate;
  const n = Math.min(hl.end - hl.start, hr.end - hr.start);
  const starts = [];
  for (let s = 0; s + N <= n; s += hop) starts.push(s);
  const P = new Float32Array(starts.length * bins), times = new Float64Array(starts.length);
  const win = Float64Array.from({ length: N }, (_, i) => 0.5 - 0.5 * Math.cos(2 * Math.PI * i / (N - 1)));
  const rl = new Float64Array(N), il = new Float64Array(N), rr = new Float64Array(N), ir = new Float64Array(N);
  for (let fi = 0; fi < starts.length; fi++) {
    const s = starts[fi]; rl.fill(0); il.fill(0); rr.fill(0); ir.fill(0);
    for (let i = 0; i < N; i++) { const w = win[i] / 32768; rl[i] = samples[hl.start + s + i] * w; rr[i] = samples[hr.start + s + i] * w; }
    fftInPlace(rl, il); fftInPlace(rr, ir);
    for (let k = 0; k < bins; k++) P[fi * bins + k] = rl[k] ** 2 + il[k] ** 2 + rr[k] ** 2 + ir[k] ** 2;
    times[fi] = (s + N / 2 - onset) / sr;
  }
  return { N, hop, bins, sr, P, times };
}

function measureRegion(sf2, region, packed, nameToId) {
  const li = nameToId.get(`${region.root}(L)`), ri = nameToId.get(`${region.root}(R)`);
  if (li === undefined || ri === undefined) throw new Error(`Нет stereo pair root ${region.root}`);
  const hl = sf2.headers[li], hr = sf2.headers[ri]; if (hl.sampleRate !== hr.sampleRate) throw new Error("L/R sample rate mismatch");
  const xl = sf2.samples.subarray(hl.start, hl.end), xr = sf2.samples.subarray(hr.start, hr.end), sr = hl.sampleRate;
  const onset = Math.min(detectOnset(xl, sr), detectOnset(xr, sr));
  const S = spectraForPair(sf2.samples, hl, hr, onset, region), binHz = sr / S.N;
  const earlyFrames = [];
  for (let i = 0; i < S.times.length; i++) if (S.times[i] >= Math.max(0.035, region.decayOnset * 0.65) && S.times[i] <= Math.min(1.0, region.sampleLen - 0.1)) earlyFrames.push(i);
  const recs = [];
  for (let j = 0; j < region.count; j++) {
    const row = region.offset + j, pp = decodeRow(packed, row), k = pp.k;
    const fr0 = 0.95 + pp.freqQ / 327675, expected = region.f0 * k * fr0;
    const halfHz = Math.min(Math.max(2 * binHz, 0.008 * expected), 0.30 * region.f0);
    const lo = Math.max(1, Math.floor((expected - halfHz) / binHz)), hi = Math.min(S.bins - 2, Math.ceil((expected + halfHz) / binHz));
    let best = lo, bestP = -1;
    for (let b = lo; b <= hi; b++) {
      let a = 0; for (const fi of earlyFrames) a += S.P[fi * S.bins + b];
      a /= Math.max(1, earlyFrames.length); if (a > bestP) { bestP = a; best = b; }
    }
    // Parabolic interpolation of log-power around the fixed peak for frequency diagnostics.
    const meanAt = (b) => earlyFrames.reduce((a, fi) => a + S.P[fi * S.bins + b], 0) / Math.max(1, earlyFrames.length);
    const ym = Math.log(Math.max(meanAt(best - 1), 1e-30)), y0 = Math.log(Math.max(meanAt(best), 1e-30)), yp = Math.log(Math.max(meanAt(best + 1), 1e-30));
    const den = ym - 2 * y0 + yp, delta = Math.abs(den) > 1e-12 ? clamp(0.5 * (ym - yp) / den, -0.5, 0.5) : 0;
    const peakHz = (best + delta) * binHz;
    const y = new Float64Array(S.times.length);
    for (let fi = 0; fi < S.times.length; fi++) {
      let p = 0; for (let b = Math.max(1, best - 1); b <= Math.min(S.bins - 1, best + 1); b++) p += S.P[fi * S.bins + b];
      y[fi] = 10 * Math.log10(Math.max(p, 1e-30));
    }
    const fit = robustPieceFit(S.times, y, region, baselineRates(packed, row));
    if (!fit) { recs.push({ row, k, valid: false, reason: "fit_failed", peakHz, expectedHz: expected }); continue; }
    const cents = 1200 * Math.log2(peakHz / expected), valid = Math.abs(cents) < 25 && fit.n >= 10;
    recs.push({ row, k, valid, peakHz, expectedHz: expected, peakCents: cents, ...fit });
  }
  return { root: region.root, sampleL: hl.name, sampleR: hr.name, sampleRate: sr, onsetSeconds: onset / sr, partials: recs };
}

function renderPackedArray(bytes) {
  const lines = [];
  for (let i = 0; i < bytes.length; i += 16) lines.push("  " + Array.from(bytes.slice(i, i + 16), (x) => `0x${x.toString(16).padStart(2, "0")}`).join(", ") + ",");
  return lines.join("\n");
}

const opts = argsOf(process.argv.slice(2));
if (!opts.sf2) { console.error("Нужен --sf2 путь"); process.exit(2); }
const headerPath = opts.header || "intrasynth/src/Intra/Synth/PianoRegions.h";
const outPath = opts.out || ".scratch/piano-joint-fit.json";
const H = parseHeader(headerPath), sf2 = readSf2(opts.sf2), nameToId = new Map(sf2.headers.map((h, i) => [h.name, i]));
const selectedRoots = opts.roots ? new Set(String(opts.roots).split(",").map((x) => +x.trim()).filter(Number.isFinite)) : null;
const checkpointDir = opts["checkpoint-dir"] ? path.resolve(String(opts["checkpoint-dir"])) : null;
if (checkpointDir) fs.mkdirSync(checkpointDir, { recursive: true });
const measured = [];
for (let i = 0; i < H.regions.length; i++) {
  const r = H.regions[i];
  if (selectedRoots && !selectedRoots.has(r.root)) continue;
  const cp = checkpointDir ? path.join(checkpointDir, `root_${r.root}.json`) : null;
  let data = null;
  if (cp && opts.resume && fs.existsSync(cp)) {
    process.stderr.write(`root ${r.root}: resume ${cp}\n`);
    data = JSON.parse(fs.readFileSync(cp, "utf8"));
  } else {
    process.stderr.write(`root ${r.root} (${i + 1}/${H.regions.length})\n`);
    data = measureRegion(sf2, r, H.packed, nameToId);
    if (cp) {
      const tmp = `${cp}.tmp`;
      fs.writeFileSync(tmp, JSON.stringify(data, null, 2));
      fs.renameSync(tmp, cp);
    }
  }
  measured.push({ ri: i, data });
}

const fitted = Uint8Array.from(H.packed), perRegion = [];
for (const item of measured) {
  const ri = item.ri, r = H.regions[ri], validPs = item.data.partials.filter((p) => p.valid);
  const ps = validPs.filter((p) => p.accepted !== false);
  const low = ps.filter((p) => p.k <= 16 && decodeRow(H.packed, p.row).ampQ > 0);
  // Абсолютный уровень FFT произволен. Сохраняем Loudness зоны, выравнивая
  // median low-harmonic Amp к текущей таблице, а меняем только форму спектра.
  const offsets = low.map((p) => {
    const amp = unpackLog(decodeRow(H.packed, p.row).ampQ) / 65535;
    return 20 * Math.log10(Math.max(amp, 1e-12)) - p.L0;
  });
  const offsetDb = median(offsets);
  for (const p of ps) {
    const amp = clamp(Math.pow(10, (p.L0 + offsetDb) / 20), 0, 1);
    setBits(fitted, p.row, 14, 11, packLog(Math.round(amp * 65535)));
    for (let j = 0; j < 4; j++) if (p.active[j]) {
      const raw = Math.max(0, p.rates[j]) / DB_PER_NEPER * DECAY_SCALE[j];
      const q = p.rates[j] < 0.10 ? 0 : packLog(Math.max(1, raw));
      setBits(fitted, p.row, [25, 36, 47, 58][j], 11, q);
    }
    if (opts["fit-frequency"]) {
      const q = clamp(Math.round((p.peakHz / (r.f0 * p.k) - 0.95) * 327675), 0, 65535);
      setBits(fitted, p.row, 69, 16, q);
    }
  }
  perRegion.push({ root: r.root, ampOffsetDb: offsetDb, validPartials: validPs.length, acceptedPartials: ps.length });
}

const report = {
  method: {
    stereo: "L/R FFT power sum",
    fft: 4096, hop: 1024,
    peak: "one fixed early-spectrum peak per partial; parabolic log-power interpolation for frequency diagnostics",
    envelopeFit: "robust IRLS continuous four-stage piecewise exponential in dB; boundaries from PianoRegionData",
    ampTimeZero: "PianoRegionData::DecayOnset; attack before it is deliberately excluded",
    applies: opts["fit-frequency"] ? "Amp + Decay1..4 + FreqRatio" : "Amp + Decay1..4; FreqRatio/Phase preserved",
  },
  sf2: path.resolve(opts.sf2), header: path.resolve(headerPath), selectedRoots: selectedRoots ? Array.from(selectedRoots) : null,
  perRegion, regions: measured.map((x) => x.data),
};
fs.mkdirSync(path.dirname(outPath), { recursive: true }); fs.writeFileSync(outPath, JSON.stringify(report, null, 2));
console.log(`Замеры: ${outPath}`);

if (opts["write-header"]) {
  let text = H.text;
  const re = /(static const uint8 PianoAllPartialsPacked\[\] = \{)\n[\s\S]*?\n(\};)/;
  if (!re.test(text)) throw new Error("Не удалось заменить PianoAllPartialsPacked");
  text = text.replace(re, `$1\n${renderPackedArray(fitted)}\n$2`);
  fs.mkdirSync(path.dirname(opts["write-header"]), { recursive: true }); fs.writeFileSync(opts["write-header"], text);
  console.log(`Header-кандидат: ${opts["write-header"]}`);
}