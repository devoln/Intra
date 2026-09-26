"use strict";
// Ориентировка агента: одна команда вместо 15 минут обхода репозитория.
//
//   node intrasynth/tools/agent/orient.mjs            полная сводка
//   node intrasynth/tools/agent/orient.mjs --brief    только шапка, порядок чтения и команды
//   node intrasynth/tools/agent/orient.mjs --json     та же сводка машинно-читаемо
//   node intrasynth/tools/agent/orient.mjs --check    проверить, что карта ещё жива
//
// Зачем: каждая сессия Freebuff/Codebuff может начинаться с чистого контекста, и
// агент заново «изучает» проект — читает правила, ищет файлы синтеза, упирается в
// то, что `scripts/`, `intrasynth/web/generated/`, `dist/` и `.scratch/` не видны инструментам
// поиска (они в .gitignore). Отсюда и «первые 15 минут». Этот скрипт отдаёт то же
// знание за один вызов, а `--check` не даёт карте устареть: он проверяет, что все
// пути, упомянутые в intrasynth/docs/Architecture.md, AGENTS.md и скиллах, всё ещё существуют.
//
// Запускать можно откуда угодно: корень вычисляется от расположения скрипта.
import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";
import { fileURLToPath } from "node:url";
import { execFileSync } from "node:child_process";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "..", "..", "..");
process.chdir(ROOT);

const WASM = "intrasynth/web/generated/IntraSynth.wasm";
const CANON_CONFIG = "plain -Os + INTRA_PIANO_ALL_TABLES=ON + INTRA_UI_METERS=ON";

// ---------------------------------------------------------------------------
// Карта проекта (данные, а не проза: --check сверяет её с диском)
// ---------------------------------------------------------------------------

const READ_ORDER = [
  { p: "intrasynth/docs/Architecture.md", why: "карта проекта: подсистемы, инварианты, чего не делать" },
  { p: "intrasynth/docs/Rules.md", why: "жёсткие правила C++/ядра (stdlib, constexpr, unsafe)" },
  { p: "intrasynth/tools/analysis/README.md", why: "как мерить звук и какие грабли уже стоили выводов" },
  { p: "AGENTS.md", why: "язык ответов, мандат сборки и размер WASM" },
];

const SUBSYSTEMS = [
  { name: "InstrumentLibrary.cpp", files: ["intrasynth/src/Intra/Synth/InstrumentLibrary.cpp"], why: "числа всех инструментов: гармоники, ширина, шум, атака, регионы" },
  { name: "генераторы и голос", files: ["intrasynth/src/Intra/Synth/Synth.h", "intrasynth/src/Intra/Synth/NoteSampler.cpp", "intrasynth/src/Intra/Synth/NoteSampler.h"], why: "NoiseSampler, VibratoLfo, микс слоёв голоса, огибающие" },
  { name: "аддитив/PADsynth", files: ["intrasynth/src/Intra/Synth/AdditiveSampler.cpp", "intrasynth/src/Intra/Synth/AdditiveSampler.h"], why: "из профиля в таблицу, ширина b(k), нормировка" },
  { name: "вейвтейбл", files: ["intrasynth/src/Intra/Synth/WaveTableSampler.cpp", "intrasynth/src/Intra/Synth/WaveTableGeneration.cpp", "intrasynth/src/Intra/Synth/WaveTable.cpp"], why: "чтение таблицы, петли и их периодика" },
  { name: "волны и струны", files: ["intrasynth/src/Intra/Synth/WaveFormSampler.cpp", "intrasynth/src/Intra/Synth/SpectralStringSampler.cpp", "intrasynth/src/Intra/Synth/KarplusStrongSampler.cpp"], why: "формулы волн (согласные), модальные/спектральные струны" },
  { name: "физические модели", files: ["intrasynth/src/Intra/Synth/DrumPhysicalModel.cpp", "intrasynth/src/Intra/Synth/ViolinPhysicalModel.h", "intrasynth/src/Intra/Synth/SnarePhysicalModel.h"], why: "барабаны, скрипка, малый барабан" },
  { name: "фортепиано", files: ["intrasynth/src/Intra/Synth/PianoRegions.h", "intrasynth/src/Intra/Synth/PianoTablesExtra.h"], why: "сгенерированные таблицы регионов и партиалов" },
  { name: "MIDI и микшер", files: ["intrasynth/src/Intra/Synth/MidiSynth.cpp", "intrasynth/src/Intra/Synth/MusicSynthesizer.cpp", "intrasynth/src/Intra/Synth/MidiInstrumentMapping.cpp"], why: "программа → инструмент, полифония, эффекты" },
  { name: "мост в WASM", files: ["intrasynth/src/Intra/Synth/EmscriptenInterface.cpp"], why: "C-API: _SourceCreateLive, _SourceSendMidiEvent, отладочные SynthSet…" },
  { name: "ядра/SIMD", files: ["intrasynth/src/Intra/Synth/ComputeKernels.h", "Intra/Simd"], why: "векторизованные ядра, блочные LFO" },
  { name: "веб-плеер", files: ["intrasynth/web/synth.js", "intrasynth/web/index.html", "intrasynth/web/debug.mjs"], why: "AudioWorklet, MIDI; отладочные спойлеры (сэмплы, A/B, ручки) — в опциональном debug.mjs" },
  { name: "инструментовка", files: ["intrasynth/tools/analysis/cli.mjs", "intrasynth/tools/ab/render-ab.mjs", "intrasynth/tools/instruments/guitar-apply.mjs"], why: "замеры, A/B-панель, генераторы блоков инструментов" },
];

const COMMANDS = [
  { what: "ориентировка (эта команда)", cmd: "node intrasynth/tools/agent/orient.mjs" },
  { what: "сборка канона", cmd: "sh scripts/build-wasm.sh" },
  { what: "стейджинг плеера", cmd: "node scripts/build-web.js" },
  { what: "проверка живости сборки", cmd: "node scripts/smoke-test-wasm.mjs" },
  { what: "самопроверка анализа (обязательна)", cmd: "node intrasynth/tools/analysis/cli.mjs selftest" },
  { what: "замер наш vs банк", cmd: "node intrasynth/tools/analysis/cli.mjs spec 52:60" },
  { what: "A/B-панель владельцу", cmd: "node intrasynth/tools/ab/render-ab.mjs" },
];

const SYMPTOMS = [
  { s: "«пшик»/вспышка шума в атаке", c: "node intrasynth/tools/analysis/cli.mjs timbre <prog:key> --to 0.5" },
  { s: "«левый период», робот", c: "node intrasynth/tools/analysis/cli.mjs period <prog:key>" },
  { s: "тремоло/вибрато не той глубины", c: "node intrasynth/tools/analysis/cli.mjs trem <prog:key> --wide 200,8000" },
  { s: "скорость и структура модуляции", c: "node intrasynth/tools/analysis/cli.mjs mod <prog:key> --from 0.9" },
  { s: "тембр не тот", c: "node intrasynth/tools/analysis/cli.mjs spec <prog:key> --kmax 24" },
  { s: "воздух/шумовая полка не той формы", c: "node intrasynth/tools/analysis/cli.mjs bands <prog:key>" },
  { s: "«инструмент тяжёлый / сколько стоит нота»", c: "node intrasynth/tools/analysis/cli.mjs cost <prog:key> --chord 5" },
  { s: "атака длиннее/короче", c: "node intrasynth/tools/analysis/fit-attack.mjs <prog:key>" },
  { s: "ширина гармоник в центах", c: "node intrasynth/tools/analysis/cli.mjs vib <prog:key> --harm 12" },
];

const PITFALLS = [
  "время в замерах — от NOTE-ON, а не от нуля файла (иначе мерите тишину)",
  "программа синтезатора ≠ пресет банка: сопоставление в lib/render.mjs (BANK_PROGRAM)",
  "узкая полоса вокруг гармоники превращает ЧМ в АМ — тремоло мерить широкой полосой",
  "лупнутая таблица даёт корреляцию на лаге ровно N семплов — это и есть «робот»",
  "ширина таблицы ≠ энсамбль: в центах ширина падает с номером гармоники",
  "шум — форма гласной (наклон в Гц), а не ровная полка до верха",
  "BuildWaveTable нормирует на сумму амплитуд, громкость даёт RMS — компенсируйте VolumeScale",
  "дБ→амплитуда: 20·log10(2)=6.0206, иначе верх ослабляется в 6 раз сильнее задуманного",
  "SineRange вырождается ниже ~4 Гц: для медленных огибающих фазовый аккумулятор",
  "один экземпляр WASM на свип: Factory() внутри renderOurs() не видит ручки зонда",
];

const DOC_DIRS = ["intrasynth/docs/tasks/active", "intrasynth/docs/decisions/active", "intrasynth/docs/tasks/planned"];

// ---------------------------------------------------------------------------
// Утилиты
// ---------------------------------------------------------------------------

const pad = (s, n) => String(s).padEnd(n);
const padL = (s, n) => String(s).padStart(n);
const exists = (p) => fs.existsSync(path.join(ROOT, p));
const kb = (p) => { try { return Math.round(fs.statSync(path.join(ROOT, p)).size / 1024) + " КБ"; } catch { return "—"; } };
const lines = (p) => { try { return fs.readFileSync(path.join(ROOT, p), "utf8").split("\n").length; } catch { return 0; } };
const mtime = (p) => { try { return fs.statSync(path.join(ROOT, p)).mtimeMs; } catch { return 0; } };

function git(args) {
  try { return execFileSync("git", args, { cwd: ROOT, encoding: "utf8", stdio: ["ignore", "pipe", "ignore"] }).trim(); }
  catch { return null; }
}

function md5(p) {
  try { return crypto.createHash("md5").update(fs.readFileSync(path.join(ROOT, p))).digest("hex"); }
  catch { return null; }
}

function newest(dir, n) {
  let names = [];
  try { names = fs.readdirSync(path.join(ROOT, dir)); } catch { return []; }
  return names
    .filter((f) => f.endsWith(".md"))
    .map((f) => ({ f, t: mtime(path.join(dir, f)), l: lines(path.join(dir, f)) }))
    .sort((a, b) => b.t - a.t)
    .slice(0, n);
}

function skills() {
  const dir = "intrasynth/.agents/skills";
  let names = [];
  try { names = fs.readdirSync(path.join(ROOT, dir)); } catch { return []; }
  return names.map((n) => {
    const p = `${dir}/${n}/SKILL.md`;
    let desc = "";
    try {
      const m = fs.readFileSync(path.join(ROOT, p), "utf8").match(/^description:\s*(.+)$/m);
      desc = m ? m[1].trim() : "";
    } catch { /* нет файла — покажем пустое описание */ }
    return { n, p, desc };
  });
}

function repoState() {
  const head = git(["log", "-1", "--oneline"]);
  const branch = git(["rev-parse", "--abbrev-ref", "HEAD"]);
  const status = git(["status", "--porcelain"]) || "";
  // « M path» / «?? path»: режем первые два символа ПОСЛЕ снятия ведущих пробелов
  // (trim() всего вывода съедает пробел первой строки и портит первый путь).
  const rows = status.split("\n").filter(Boolean).map((r) => r.replace(/^ +/, ""));
  return {
    branch,
    head,
    modified: rows.filter((r) => !r.startsWith("??")).length,
    untracked: rows.filter((r) => r.startsWith("??")).length,
    dirtySample: rows.slice(0, 8).map((r) => r.slice(2).trim()).concat(rows.length > 8 ? ["…"] : []),
  };
}

function wasmState() {
  const w = md5(WASM);
  const size = exists(WASM) ? fs.statSync(path.join(ROOT, WASM)).size : 0;
  let distFiles = 0, distNewest = 0;
  try {
    for (const f of fs.readdirSync(path.join(ROOT, "dist"))) {
      distFiles++;
      distNewest = Math.max(distNewest, mtime(path.join("dist", f)));
    }
  } catch { /* dist не собран */ }
  return {
    size,
    sizeKb: Math.round(size / 1024),
    md5: w,
    built: exists(WASM) ? new Date(mtime(WASM)).toISOString().slice(0, 16).replace("T", " ") : "нет",
    distFiles,
    distFresh: distFiles > 0 && distNewest >= mtime(WASM),
    config: CANON_CONFIG,
  };
}

// ---------------------------------------------------------------------------
// Вывод
// ---------------------------------------------------------------------------

function printFull() {
  const st = repoState();
  const w = wasmState();
  const sk = skills();
  const docs = READ_ORDER.map((r) => ({ ...r, ok: exists(r.p), size: kb(r.p), lines: lines(r.p) }));

  console.log("==============================================================");
  console.log(" Intra / IntraSynth — ориентировка");
  console.log("==============================================================");
  console.log(`корень:   ${ROOT}`);
  console.log(`git:      ветка ${st.branch ?? "?"}, HEAD ${st.head ?? "?"}`);
  console.log(`рабочее:  изменено ${st.modified}, неотслеживаемых ${st.untracked}` + (st.dirtySample.length ? ` (${st.dirtySample.join(", ")})` : ""));
  console.log(`канон:    ${w.size ? `${w.size} байт` : "не собран"} (${w.sizeKb} КБ), md5 ${w.md5 ?? "—"}`);
  console.log(`          ${w.config}`);
  console.log(`          сборка от ${w.built}; dist: ${w.distFiles} файлов, ${w.distFresh ? "свежее сборки" : "СТАРШЕ сборки — нужен node scripts/build-web.js"}`);

  console.log("\n-- порядок чтения (вместо обхода репозитория) ---");
  for (const d of docs) console.log(`  ${d.ok ? " " : "!"} ${pad(d.p, 40)} ${padL(d.size, 7)} ${padL(d.lines, 5)} строк  ${d.why}`);

  console.log("\n-- скиллы (.agents/skills) ---");
  for (const s of sk) console.log(`  ${pad(s.n, 38)} ${s.desc ? s.desc.slice(0, 90) : "(нет description)"}`);

  console.log("\n-- подсистемы (что где) ---");
  for (const s of SUBSYSTEMS) {
    const sizes = s.files.map((f) => (fs.statSync(path.join(ROOT, f), { throwIfNoEntry: false })?.isDirectory() ? `${f}/` : `${f} (${lines(f)} стр.)`));
    console.log(`  ${pad(s.name, 22)} ${s.why}`);
    console.log(`  ${" ".repeat(22)} ${sizes.join(", ")}`);
  }

  console.log("\n-- НЕ ВИДНО инструментам поиска (в .gitignore, но существует) ---");
  console.log("  scripts/ (сборка, стейджинг, смоук, генераторы)  intrasynth/web/generated/ (wasm, js, samples/, ab/)");
  console.log("  dist/ (собранный плеер)                          .scratch/ (зонды, ab-builds/<id>/ — не мусор)");

  console.log("\n-- канонические команды (из корня репозитория) ---");
  for (const c of COMMANDS) console.log(`  ${pad(c.what, 34)} ${c.cmd}`);

  console.log("\n-- симптом → замер ---");
  for (const s of SYMPTOMS) console.log(`  ${pad(s.s, 34)} ${s.c}`);

  console.log("\n-- грабли, которые уже стоили дорого ---");
  for (const p of PITFALLS) console.log(`  - ${p}`);

  console.log("\n-- свежие апдейты (intrasynth/docs/tasks/active) ---");
  for (const t of newest("intrasynth/docs/tasks/active", 5)) console.log(`  ${pad(t.f, 52)} ${padL(t.l, 4)} строк`);
  console.log("-- свежие решения (intrasynth/docs/decisions/active) ---");
  for (const t of newest("intrasynth/docs/decisions/active", 5)) console.log(`  ${pad(t.f, 52)} ${padL(t.l, 4)} строк`);

  console.log("\n-- дальше ---");
  console.log("  1) intrasynth/docs/Architecture.md — карта целиком (инварианты, чего не делать)");
  console.log("  2) node intrasynth/tools/analysis/cli.mjs selftest — пока не PASSED, числам замеров верить нельзя");
  console.log("  3) intrasynth/.agents/skills/project-intra-fitting-and-measurement — подходы фиттинга и ловушки");
  console.log("  4) intrasynth/.agents/skills/stack-freebuff-cloud-session — правила работы в этой среде");
}

function printBrief() {
  const st = repoState();
  const w = wasmState();
  console.log(`Intra/IntraSynth | ветка ${st.branch}, HEAD ${st.head}`);
  console.log(`канон WASM: ${w.size} байт, md5 ${w.md5}, ${w.config}; dist ${w.distFresh ? "ок" : "устарел"}`);
  console.log("карта: intrasynth/docs/Architecture.md | правила: intrasynth/docs/Rules.md | замеры: intrasynth/tools/analysis/README.md");
  console.log("сборка: sh scripts/build-wasm.sh && node scripts/build-web.js; A/B: node intrasynth/tools/ab/render-ab.mjs");
  console.log("замер: node intrasynth/tools/analysis/cli.mjs selftest (обязательно), затем <spec|bands|vib|trem|mod|timbre|pitch|period|floor|env|attack|cost> prog:key");
  console.log("язык ответов и документов — русский; временное — в .scratch/; серверы и .env не трогать.");
}

function printJson() {
  console.log(JSON.stringify({
    repo: repoState(),
    wasm: wasmState(),
    readOrder: READ_ORDER,
    skills: skills(),
    subsystems: SUBSYSTEMS,
    commands: COMMANDS,
    symptoms: SYMPTOMS,
    pitfalls: PITFALLS,
    tasks: newest("intrasynth/docs/tasks/active", 5).map((t) => ({ name: t.f, lines: t.l })),
    decisions: newest("intrasynth/docs/decisions/active", 5).map((t) => ({ name: t.f, lines: t.l })),
  }, null, 2));
}

// ---------------------------------------------------------------------------
// --check: карта не должна устареть
// ---------------------------------------------------------------------------

// Путь в обратных кавычках ищем не только от корня: в скиллах и README пути
// пишутся и относительно своих каталогов (например `lib/render.mjs`).
const BASE_HINTS = ["", "intrasynth/tools/analysis", "intrasynth/tools/ab", "intrasynth/tools/agent",
  "intrasynth/tools/instruments", "intrasynth/src/Intra/Synth", "scripts", "intrasynth/web", "intrasynth/web/generated", "intrasynth/docs", "intrasynth/.agents/skills"];

// Верхний уровень репозитория: путём считается только то, что начинается с него
// (`active/`, `tools/`, `Medium/High` — это фрагменты текста, а не пути).
const TOP_LEVEL = new Set(fs.readdirSync(ROOT));
const PLACEHOLDER = /YYYY|XXX|<|>|\{\}|\*|Name\.md$/;

function resolveHint(tok) {
  const clean = tok.replace(/^\.\//, "").replace(/^\/+/, "").replace(/\/+$/, "");
  for (const base of BASE_HINTS) {
    const p = base ? `${base}/${clean}` : clean;
    if (fs.existsSync(path.join(ROOT, p))) return p;
  }
  return null;
}

function isPathToken(tok) {
  if (!/^[\w./-]+$/.test(tok) || !tok.includes("/")) return false;
  if (PLACEHOLDER.test(tok)) return false;
  const top = tok.replace(/^\.\//, "").replace(/^\/+/, "").split("/")[0];
  return TOP_LEVEL.has(top);
}

function scanDoc(rel) {
  const full = path.join(ROOT, rel);
  if (!fs.existsSync(full)) return [{ file: rel, item: "(файл отсутствует)", kind: "file" }];
  const text = fs.readFileSync(full, "utf8");
  const bad = [];
  for (const m of text.matchAll(/`([^`\n]+)`/g)) {
    const tok = m[1].trim();
    if (isPathToken(tok) && !resolveHint(tok)) bad.push({ file: rel, item: tok, kind: "path" });
  }
  // Команды ищем и в обратных кавычках, и в блоках кода (там их пишут без кавычек).
  for (const m of text.matchAll(/(?:^|[`\s])(?:node|sh)\s+(\S+)/gm)) {
    const arg = m[1].replace(/[`).,;]+$/, "");
    if (!arg.includes("/") || PLACEHOLDER.test(arg)) continue;
    if (!resolveHint(arg)) bad.push({ file: rel, item: arg, kind: "command" });
  }
  return bad;
}

function printCheck() {
  let bad = [];
  const docs = ["intrasynth/docs/Architecture.md", "AGENTS.md", "intrasynth/AGENTS.md", ...skills().map((s) => s.p),
    "intrasynth/tools/analysis/README.md"];
  for (const d of docs) bad = bad.concat(scanDoc(d));
  for (const s of SUBSYSTEMS) for (const f of s.files) if (!exists(f)) bad.push({ file: "SUBSYSTEMS", item: f, kind: "path" });
  for (const r of READ_ORDER) if (!exists(r.p)) bad.push({ file: "READ_ORDER", item: r.p, kind: "path" });
  for (const c of COMMANDS) for (const m of c.cmd.matchAll(/(?:node|sh)\s+(\S+)/g)) if (!resolveHint(m[1])) bad.push({ file: "COMMANDS", item: m[1], kind: "command" });

  if (!bad.length) {
    console.log(`КАРТА ЖИВА: ${docs.length} документов, ${SUBSYSTEMS.length} подсистем, ${COMMANDS.length} команд — все пути и команды на месте.`);
    return 0;
  }
  console.log(`КАРТА УСТАРЕЛА (${bad.length}):`);
  for (const b of bad) console.log(`  ${b.file}: ${b.kind === "command" ? "команда" : "путь"} ${b.item}`);
  console.log("Правьте карту (intrasynth/docs/Architecture.md, AGENTS.md, скиллы) — не подгоняйте её под реальность молча.");
  return 1;
}

// ---------------------------------------------------------------------------

const arg = process.argv[2] || "";
if (arg === "--check") process.exit(printCheck());
if (arg === "--brief") printBrief();
else if (arg === "--json") printJson();
else if (arg === "" || arg === "--full") printFull();
else {
  console.log("Ориентировка агента по репозиторию Intra.");
  console.log("  node intrasynth/tools/agent/orient.mjs [--brief | --json | --check]");
  process.exit(2);
}
