"use strict";
// Подбор параметров атаки (огибающей) инструмента под замер банка.
//
// Зачем: до этого константы огибающей подбирались «на глаз» по одному-двум
// окнам, и Update 64 промахнулся на 2 дБ по пику и вдвое по времени раздува
// («бугорок у оригинала длиннее, у нас быстро спадает»). Здесь форма атаки
// банка снимается равномерно (окна 2 мс, 4-96 мс), нормируется на полку и
// параметры огибающей подбираются численно.
//
//   node intrasynth/tools/analysis/fit-attack.mjs 74:62 74:67 74:70 74:72 74:74 74:79 74:84
//
// Печатает подобранные (startV, peak, dip, t1, t2, t3) и остаточную ошибку.
import { SR, renderPair } from "./lib/render.mjs";
import { levelWindows, referenceLevel } from "./lib/dsp.mjs";

/// Кубический спуск по сетке для одной зоны.
function fitShape(target, { t0 }) {
  // Модель огибающей: StartVolume → Pk (экспон.) → Dip (экспон.) → 1 (экспон.)
  // t1 = t0*t1m, t2 = t0*t2m, t3 = t3abs.
  const model = (t, p) => {
    if (t <= 0) return p.v0;
    if (t < p.t1) { const u = t / p.t1; return p.v0 * Math.pow(p.pk / p.v0, u); }
    if (t < p.t1 + p.t2) { const u = (t - p.t1) / p.t2; return p.pk * Math.pow(p.dip / p.pk, u); }
    const u = (t - p.t1 - p.t2) / p.t3;
    if (u >= 1) return 1;
    return p.dip * Math.pow(1 / p.dip, u);
  };
  // Вес: «бугорок» (4-30 мс) — то, что слышно, — втрое важнее остального.
  const err = (p) => {
    let s = 0, n = 0;
    for (const [t, r] of target) {
      const m = 20 * Math.log10(Math.max(model(t, p), 1e-6));
      const w = t >= 0.004 && t <= 0.030 ? 3 : 1;
      s += w * (m - r) ** 2; n += w;
    }
    return Math.sqrt(s / n);
  };
  // dipRel = провал/пик в [0.72, 0.97): реальный «бугорок» банка, а не просто
  // двухфазный подъём. t2 (спад с пика) — 4..12 мс.
  const mk = (v0, pk, dipRel, t1, t2, t3) => ({ v0, pk, dip: pk * dipRel, dipRel, t1, t2, t3 });
  let best = mk(0.004, 0.030, 0.90, 0.011, 0.007, 0.035);
  let bestE = err(best);
  const steps = [
    { k: "v0", s: [0.0005, 0.001, 0.002] }, { k: "pk", s: [0.002, 0.005, 0.01] },
    { k: "dipRel", s: [0.02, 0.05] }, { k: "t1", s: [0.001, 0.002, 0.004] },
    { k: "t2", s: [0.001, 0.002] }, { k: "t3", s: [0.005, 0.02, 0.05] },
  ];
  for (let pass = 0; pass < 80; pass++) {
    let improved = false;
    for (const { k, s } of steps) for (const d of s) for (const sign of [1, -1]) {
      const p = { ...best, [k]: best[k] + sign * d };
      p.dip = p.pk * p.dipRel;
      if (p.v0 <= 0 || p.pk <= p.v0) continue;
      if (p.dipRel < 0.80 || p.dipRel > 0.96) continue;
      if (p.t1 < 0.009 || p.t1 > 0.019 || p.t2 < 0.007 || p.t2 > 0.011 || p.t3 <= 0) continue;
      const e = err(p);
      if (e < bestE - 1e-9) { best = p; bestE = e; improved = true; }
    }
    if (!improved) break;
  }
  return { best, bestE };
}

const notes = process.argv.slice(2).filter((a) => a.includes(":"));
if (!notes.length) { console.log("Укажите ноты: 74:62 74:72 ..."); process.exit(2); }

for (const note of notes) {
  const [program, key] = note.split(":").map(Number);
  const { bank } = await renderPair({ program, key });
  if (!bank) { console.log(`${note}: банк не отрендерился`); continue; }
  const ref = referenceLevel(bank, SR, 2.2, 0.5);
  const prof = levelWindows(bank, SR, { lenMs: 2, hopMs: 2, fromSeconds: 0.2, toSeconds: 0.3 });
  const plateau = prof.find((p) => Math.round(p.t) === 88)?.db - ref ?? 0;
  const target = prof.filter((p) => p.t >= 4 && p.t <= 96)
    .map((p) => [p.t / 1000, (p.db - ref) - plateau]);
  // Прямая экстракция (надёжнее подгонки): t1 — время локального максимума в
  // 4-20 мс, pk/dip — значения огибающей банка в пике и в следующем минимуме,
  // t2 — расстояние между ними, t3 — время от минимума до 0.9 полки.
  // Если «бугорка» нет (кривая монотонна) — пик на 6 мс, провал = пик, t2 = 7 мс.
  const E = (t) => Math.pow(10, (target.find(([tt]) => tt >= t)?.[1] ?? 0) / 20);
  const inWin = target.filter(([t]) => t >= 0.004 && t <= 0.022);
  let iMax = 0, iMin = -1;
  for (let i = 1; i < inWin.length; i++) if (inWin[i][1] > inWin[iMax][1]) iMax = i;
  for (let i = iMax + 1; i < inWin.length; i++) {
    if (iMin < 0 || inWin[i][1] < inWin[iMin][1]) iMin = i;
    if (inWin[i][1] > inWin[iMax][1]) { iMin = -1; break; } // «пик» не пик
  }
  const bump = iMin > iMax;
  let t1, t2, pk, dip;
  if (bump) {
    t1 = inWin[iMax][0]; pk = E(t1);
    t2 = inWin[iMin][0] - t1; dip = E(inWin[iMin][0]);
  } else {
    t1 = 0.006; pk = dip = E(0.006); t2 = 0.007;
  }
  let t3 = 0.06;
  for (const [t, r] of target) {
    if (t > t1 + t2 && r - target[target.length - 1][1] > -1.0) { t3 = t - t1 - t2; break; }
  }
  const best = { v0: Math.min(pk * 0.12, 0.004), pk, dip, t1, t2, t3 };
  const evalErr = (p) => {
    let s = 0, n = 0;
    for (const [t, r] of target) {
      let e;
      if (t < p.t1) e = p.v0 * Math.pow(p.pk / p.v0, t / p.t1);
      else if (t < p.t1 + p.t2) e = p.pk * Math.pow(p.dip / p.pk, (t - p.t1) / p.t2);
      else { const u = (t - p.t1 - p.t2) / p.t3; e = u >= 1 ? 1 : p.dip * Math.pow(1 / p.dip, u); }
      const w = t >= 0.004 && t <= 0.030 ? 3 : 1;
      s += w * (20 * Math.log10(Math.max(e, 1e-6)) - r) ** 2; n += w;
    }
    return Math.sqrt(s / n);
  };
  const bestE = evalErr(best);
  console.log(`${note}: старт ${best.v0.toFixed(4)} пик ${best.pk.toFixed(4)} провал ${best.dip.toFixed(4)}`
    + ` | t1 ${(best.t1 * 1000).toFixed(1)} t2 ${(best.t2 * 1000).toFixed(1)} t3 ${(best.t3 * 1000).toFixed(1)} мс`
    + ` | rms ошибки ${bestE.toFixed(2)} дБ`);
}
