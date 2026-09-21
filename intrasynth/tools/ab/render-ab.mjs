"use strict";
// A/B-файлы для спойлера «A/B-рендеры» в веб-плеере (Update 72).
//
// Зачем: владелец слушает рендеры ушами, но не может запускать зонды. Раньше
// wav-ы складывались в dist/ab/ вручную и исчезали при следующей сборке
// (scripts/build-web.js стирает dist/). Теперь они генерируются в
// web/generated/ab/ — в штатное место генерируемых артефактов, откуда
// build-web копирует их в dist/ab/ (та же схема, что у samples/).
//
// Имена файлов СОДЕРЖАТ хеш содержимого (<slug>.<hash8>.wav). Поэтому URL
// меняется только у изменившегося файла, и браузер не перекачивает остальные:
// превью отдаёт ab/*.wav с Cache-Control: immutable (scripts/serve.js), а
// любой другой статик-хост — минимум 304 по ETag.
//
// Готовый файл используется повторно, если он НОВЕЕ своей сборки (mtime wasm),
// поэтому обычный запуск после правки кода перерисовывает только изменившееся.
//
// Запуск из КОРНЯ репозитория:
//   node intrasynth/tools/ab/render-ab.mjs            # обновить по mtime
//   node intrasynth/tools/ab/render-ab.mjs --force    # перерендерить всё
//
// «prev» — сборка ПРЕДЫДУЩЕГО апдейта (в ней владелец слышал прошлый дефект).
// Перед новой правкой положите текущую пару из web/generated/ в
// .scratch/ab-builds/prev/ — иначе сравнение «до/после» пропадёт из панели.
import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";
import { renderOurs, renderBank, writeWavMono, normalizeSustainRms, bankProgramFor, SR, DEFAULT_SF2 } from "../analysis/lib/render.mjs";

const OUT_DIR = path.join("web", "generated", "ab");
const LENGTH_SEC = 2.6; // 0.2 с тишины до note-on + 2.4 с ноты
const FORCE = process.argv.includes("--force");

// Сборки, которые попадают в A/B. «bank» — не сборка, а fluidsynth+SF2.
const BUILDS = [
  {
    id: "nash",
    label: "наш · текущая",
    desc: "Текущая сборка web/generated/IntraSynth.js "
      + "(канон: plain -Os, INTRA_PIANO_ALL_TABLES=ON).",
    wasmJs: path.join("web", "generated", "IntraSynth.js"),
  },
  {
    id: "prev",
    label: "наш · предыдущая",
    desc: "Сборка предыдущего апдейта (снимок .scratch/ab-builds/prev) — "
      + "с ней и сравнивается правка «до/после».",
    wasmJs: path.join(".scratch", "ab-builds", "prev", "IntraSynth.js"),
  },
  {
    id: "bank",
    // Update 74: подпись с ПРЕСЕТОМ обязательна. Владелец: «Я не понял, что за
    // бред в A/B Titanic? Там вообще звук посторонний» — это была наша ошибка
    // сопоставления программ: для флейты 43/115 рендерился пресет 43
    // (Contra Bass), а не 73 (Flute). Теперь пресет пишется прямо в подписи.
    label: "банк Titanic",
    desc: "FluidSynth + Titanic 200 GM-GS v1.2.sf2, СУХО (реверб/хорус выкл.) — "
      + "эталон, по которому тюним (см. BANK_PROGRAM в tools/analysis/lib/render.mjs).",
    bank: true,
  },
];

const GROUPS = [
  { title: "Флейта 43 · C4", program: 43, key: 60, variants: ["nash", "prev", "bank"] },
  { title: "Флейта 43 · C5", program: 43, key: 72, variants: ["nash", "prev", "bank"] },
  { title: "Флейта 43 · C6", program: 43, key: 84, variants: ["nash", "prev", "bank"] },
  { title: "Флейта 115 (гибрид) · C4", program: 115, key: 60, variants: ["nash", "prev", "bank"] },
  { title: "Пан-флейта · C4", program: 75, key: 60, variants: ["nash", "prev", "bank"] },
  { title: "Пан-флейта · C5", program: 75, key: 72, variants: ["nash", "prev", "bank"] },
  { title: "Пан-флейта · C6", program: 75, key: 84, variants: ["nash", "prev", "bank"] },
  // Update 93: РЕЛИЗ флейт. У этих групп note-off стоит ВНУТРИ файла (1.2 с),
  // иначе щелчок окончания ноты просто не попадает в 2.6-секундный рендер
  // (у остальных групп нота держится до конца файла). Владелец слышал
  // «каждая нота флейты заканчивается щелчком» ИМЕННО здесь: в «предыдущей»
  // сборке (снимок prev — та, что он слушал) флейта глохнет за один семпл.
  { title: "Флейта 73 (FluteDLS) · релиз C4", program: 73, key: 60, noteOff: 1.2, variants: ["nash", "prev", "bank"] },
  { title: "Флейта 43 · релиз C4", program: 43, key: 60, noteOff: 1.2, variants: ["nash", "prev", "bank"] },
];

// note-off — часть имени файла: у релизных групп он свой, и без этого slug
// совпал бы с обычной группой той же программы и клавиши.
const slugFor = (program, key, id, noteOff) =>
  `${program === 75 ? "pf" : `f${program}`}-${key}-${id}` + (noteOff === undefined ? "" : `-rel${Math.round(noteOff * 1000)}`);

// Имена пресетов Titanic (bank 0) — чтобы в подписи было видно, ЧТО играет.
const PRESET_NAME = { 73: "Flute", 74: "Recorder", 75: "Panflute", 76: "Bottle Blow",
  77: "Shakuhachi", 78: "Whistle", 79: "Ocarina" };

/// Время сборки, к которой привязан файл: у банка — SF2, иначе сам wasm.
function inputMtime(build) {
  const p = build.bank ? DEFAULT_SF2 : build.wasmJs;
  try { return fs.statSync(p).mtimeMs; } catch { return null; }
}

async function main() {
  fs.mkdirSync(OUT_DIR, { recursive: true });
  const builds = new Map(BUILDS.map((b) => [b.id, b]));
  const wanted = new Set();
  const groups = [];
  let totalBytes = 0, rendered = 0, reused = 0, missing = 0;

  for (const g of GROUPS) {
    const items = [];
    for (const id of g.variants) {
      const build = builds.get(id);
      // Update 74: у банка в slug входит НОМЕР ПРЕСЕТА. Иначе правка
      // сопоставления (BANK_PROGRAM) не меняла имя файла, а готовый файл
      // считался валидным по mtime SF2 — и в панели оставался старый, неверный
      // рендер (именно это владелец и услышал: «в A/B Titanic звук посторонний»).
      const slug = slugFor(g.program, g.key, id, g.noteOff) + (build.bank ? `-p${bankProgramFor(g.program)}` : "");
      const mtime = inputMtime(build);
      const existing = fs.readdirSync(OUT_DIR).filter((f) => f.startsWith(slug + ".") && f.endsWith(".wav"));
      let file = null;
      if (!FORCE && mtime !== null && existing.length === 1) {
        const st = fs.statSync(path.join(OUT_DIR, existing[0]));
        if (st.mtimeMs >= mtime) file = existing[0];
      }
      if (!file) {
        let x = null;
        try {
          x = build.bank
            ? renderBank({ program: g.program, key: g.key, noteOff: g.noteOff })
            : await renderOurs({ program: g.program, key: g.key, wasmJs: build.wasmJs, noteOff: g.noteOff });
        } catch (err) { x = null; }
        if (!x || !x.length) {
          console.log(`  ${slug}: пропуск (${build.bank ? "банк не отрендерился" : `нет сборки ${build.wasmJs}`})`);
          missing++;
          continue;
        }
        // Update 73: выравнивание громкости по сустейну (см. normalizeSustainRms):
        // без него банк был на 6-10 дБ тише нашего синтезатора.
        const tmp = path.join(OUT_DIR, slug + ".tmp.wav");
        writeWavMono(tmp, normalizeSustainRms(x, { sampleRate: SR }).slice(0, Math.round(LENGTH_SEC * SR)));
        const buf = fs.readFileSync(tmp);
        fs.unlinkSync(tmp);
        const hash = crypto.createHash("sha256").update(buf).digest("hex").slice(0, 8);
        file = `${slug}.${hash}.wav`;
        fs.writeFileSync(path.join(OUT_DIR, file), buf);
        for (const stale of existing) if (stale !== file) fs.unlinkSync(path.join(OUT_DIR, stale));
        rendered++;
      } else {
        reused++;
      }
      const bytes = fs.statSync(path.join(OUT_DIR, file)).size;
      wanted.add(file);
      totalBytes += bytes;
      // Для банка в подпись идёт номер пресета, который реально отрендерился.
      const bp = bankProgramFor(g.program);
      const label = build.bank
        ? `${build.label} · пресет ${bp}${PRESET_NAME[bp] ? ` «${PRESET_NAME[bp]}»` : ""}`
        : build.label;
      items.push({ label, desc: build.desc, src: `ab/${file}`, bytes });
    }
    if (items.length) groups.push({ title: g.title, program: g.program, key: g.key, noteOff: g.noteOff, items });
  }

  // Устаревшие файлы (нет в матрице) удаляем, чтобы каталог не рос вечно.
  for (const f of fs.readdirSync(OUT_DIR)) {
    if (f.endsWith(".wav") && !wanted.has(f)) { fs.unlinkSync(path.join(OUT_DIR, f)); console.log(`  удалён устаревший ${f}`); }
  }

  const manifest = {
    generated: new Date().toISOString(),
    note: `A/B-рендеры. Громкость выровнена по RMS сустейна (иначе банк `
      + `звучит на 6-10 дБ тише синтезатора), поэтому сравнивайте тембр и `
      + `атаку, а не громкость. Имена содержат хеш содержимого: изменившийся `
      + `файл получает новый URL, остальные браузер не перекачивает. `
      + `Длина ${LENGTH_SEC} с.`,
    groups,
  };
  fs.writeFileSync(path.join(OUT_DIR, "manifest.json"), JSON.stringify(manifest, null, 2) + "\n");
  console.log(`ab: групп ${groups.length}, файлов ${wanted.size} `
    + `(${rendered} отрендерено, ${reused} взято готовыми, ${missing} пропущено), `
    + `${(totalBytes / 1024 / 1024).toFixed(2)} МБ в ${OUT_DIR}`);
}

await main();
