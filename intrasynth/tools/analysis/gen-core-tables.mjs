"use strict";
// ГЕНЕРАТОР ТАБЛИЦ ЯДЕР (kVoiceCore*) из замера fit-harmam.mjs.
//
//   node intrasynth/tools/analysis/fit-harmam.mjs 54:51 54:60 54:72 54:81 \
//        --bank --out .scratch/harmam54.json
//   node intrasynth/tools/analysis/gen-core-tables.mjs .scratch/harmam54.json \
//        --regions 51,60,72,81 --name kVoiceCore
//
// ПРАВИЛА ПЕРЕВОДА ЗАМЕРА В БАЙТЫ (все три числа — из ОДНОГО кода, что и
// измеряет `fit-harmam`: огибающая гармоники, полосовой фильтр ±1.5 % + ФНЧ 30 Гц):
//
//  * ГЛУБИНА. Ядро движка даёт уровень `1 + m·sin`, то есть в дБ это
//    `20·log10(1 + m·cos) ≈ 8.686·m·cos` — синус с амплитудой `8.686·m` дБ и
//    rms `8.686·m/√2`. Поэтому глубина берётся по измеренному RMS огибающей:
//    `m = R/6.142`, байт = `round(255·m)`. Почему не по размаху: у сэмплового
//    банка движение ПИКОВОЕ (у 53:60 h1 размах 21 дБ, а rms всего 2.7), одной
//    синусоидой размах не повторить, а энергия модуляции (rms) — та величина,
//    которая переносится в sin-модель осмысленно.
//  * СКОРОСТЬ. Берётся СИЛЬНЕЙШАЯ линия модуляции (`modPeaks[0]`): только у неё
//    осмысленна и фаза. Центроид (`--rate centroid`) печатается как диагноз
//    «эффективной скорости», но у банка движение ШИРОКОПОЛОСНОЕ (у 54:72 h1
//    линии 1.72/1.61/1.83 Гц по 2.4 дБ И семейство 3-5 Гц по 1.6 дБ, центроид
//    4.4), и одной синусоидой его не повторить: энергия берётся по rms (глубина),
//    а скорость и фаза — по линии, которая эту энергию в основном несёт.
//    Байт = `round(частота/0.05)`, 255 = 12.75 Гц.
//  * ФАЗА. Ядро стартует с `AmPhase` в момент note-on, и по фазе получается
//    вход ноты («у банка двухфазная атака»). Замер даёт уровень как
//    `base + mag·cos(2πft − φ)`, движок делает `1 + m·sin(2πft + ψ)`, то есть
//    требуется `ψ = π/2 − φ`: байт = `round(256·(0.25 − φ)) mod 256`.
//
//  * ПОПРАВКА НА ВЫХОД (`--ours`). Глубины выше взяты из замера БАНКА, то есть
//    модель ядра считается идеальной; на деле в полосе гармоники всегда есть
//    ещё что-то (воздух, движение таблицы), и измеренная глубина ВЫХОДА выходит
//    меньше. Поэтому второй проход: `fit-harmam … --ours --out ours.json`, и
//    генератор с `--ours ours.json` умножает глубину на `rms_банк/rms_наш` по
//    КАЖДОЙ гармонике области (кламп сверху `--cap`, по умолчанию 240 = m 0.94:
//    глубже начинается провал ниже −30 дБ, которого у ядра быть не может).
//    Это честный цикл «фит → проверка выходом → повтор»: он и записан в журнал.
import fs from "node:fs";

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
const file = opts._[0];
if (!file) { console.log("укажите JSON от fit-harmam (--out) и --regions 51,60,72,81"); process.exit(2); }
const data = JSON.parse(fs.readFileSync(file, "utf8"));
const regions = String(opts.regions || "").split(",").filter(Boolean).map(Number);
if (!regions.length) { console.log("нужен --regions (клавиши-центры областей по порядку)"); process.exit(2); }
const name = opts.name || "kVoiceCore";
const KMAX = Number(opts.harm || 8);

const bank = new Map();
for (const row of data.rows) bank.set(row.key, row);

const arr = (label, fn) => {
  const lines = regions.map((key) => {
    const row = bank.get(key);
    if (!row) return `\t\t{${"?".repeat(1)}},  // нет замера по клавише ${key}`;
    const vals = [];
    for (let k = 0; k < KMAX; k++) {
      const h = row.harmonics[k];
      if (!h) { vals.push(0); continue; }
      vals.push(fn(h, key, k));
    }
    return `\t\t{${vals.map((v) => String(v).padStart(3)).join(", ")}},   // ${key}`;
  });
  return `\tstatic const uint8 ${label}[${regions.length}][kVoiceCoreMax] =\n\t{\n${lines.join("\n")}\n\t};`;
};

const clampByte = (v) => Math.max(0, Math.min(255, Math.round(v)));
// Фаза — величина круговая: байт оборачивается по модулю 256.
const phaseByte = (phi) => ((Math.round(256 * (0.25 - phi)) % 256) + 256) % 256;
const mOf = (h) => Math.max(0, Math.min(1, h.rms / 6.142));

// Поправка на выход: та же нота нашего синтеза.
const oursRows = new Map();
if (opts.ours) {
  for (const row of JSON.parse(fs.readFileSync(opts.ours, "utf8")).rows) oursRows.set(row.key, row);
}
const cap = Number(opts.cap ?? 240);
const ratioFor = (key, k, h) => {
  const row = oursRows.get(key);
  const o = row?.harmonics?.[k];
  if (!o || !o.rms) return 1;
  return Math.max(0.2, Math.min(4, h.rms / Math.max(o.rms, 0.01)));
};
const depthByte = (key) => (h, k) => Math.min(cap, clampByte(255 * mOf(h) * ratioFor(key, k, h)));

const useCentroid = opts.rate === "centroid";
console.log(`// сгенерировано из ${file} (области ${regions.join("/")}), правило: m = rms/6.142,`
  + ` скорость = ${useCentroid ? "центроид" : "сильнейшая линия"}/0.05, фаза = 0.25 − φ`
  + (opts.ours ? `, поправка на выход из ${opts.ours} (потолок ${cap})` : ""));
console.log(arr(name + "Depth", (h, key, k) => depthByte(key)(h, k)));
console.log(arr(name + "Rate", (h) => clampByte((useCentroid ? (h.rateC ?? h.rate) : h.rate) / 0.05)));
console.log(arr(name + "Phase", (h) => phaseByte(useCentroid ? (h.phaseC ?? 0.25) : (h.phase ?? 0.25))));
