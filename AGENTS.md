Use relevant skills from `.agents/skills/`.

For non-trivial work, the default runtime path should come from thin skills, especially shared task-init / worklog / closeout discipline, not from large maintainer docs.

Whatever you do, from designing a system to tiny bugfix, you MUST always keep in mind and follow `docs/Rules.md`.

Task/history documents live in `docs/tasks/`: `active/`, `planned/`, `archived/`.

Architecture and trade-off records live in `docs/decisions/`: `active/`, `accepted/`, `archived/`.

## Язык (правило владельца)

- Всё, что не является размышлениями и комментариями в коде, должно быть **только на русском**: ответы агента, тексты worklog/task/decision-документов, описания изменений, отчёты о размере WASM и т.п.
- Исключения: внутренние рассуждения (reasoning), комментарии в коде и технические идентификаторы (имена файлов, функций, переменных).

Task and decision file names should use the format `YYYYMMDD-UpperCamelCase.md`.

If work continues for a while in one direction without an assigned task, and decisions or trade-offs are being made, create or attach a task/worklog before continuing further.

High-level process artifacts and shared skills in `.agents/` must not be changed without explicit owner agreement.


Temporary experiment output (ad-hoc build trees, probes, scratch renders) must go under `.scratch/` and stay gitignored; never leave untracked `build-*/` or `probe-*` dirs in the repo root (`/.scratch/`, `/build-*/`, `/probe-tmp/` are in `.gitignore`).

## IntraSynth WASM build mandate (Intra project)

- The canonical, owner-tracked build is **plain `-Os` everywhere with
  `INTRA_PIANO_ALL_TABLES=ON`** (`sh scripts/build-wasm.sh`, ~199 KB on the
  current instrument set). It is the ONLY configuration whose size the owner
  tracks and wants reported.
- **Every modification to the synth code must rebuild the canonical config
  (`sh scripts/build-wasm.sh`), stage it (`web/generated/` + `dist/` via
  `node scripts/build-web.js`), and report its exact WASM size in the response
  and in the worklog entry.** Do not finish a turn without reporting the size.
- The canonical build also enables `INTRA_UI_METERS` (per-note envelope levels
  for the web-UI track indicators; see `scripts/build-wasm.sh`). Minimal builds
  leave it OFF (default) and carry zero bytes of it — every part of that feature
  is behind `#ifdef INTRA_UI_METERS`.
- The `-Oz` size hybrid is **forbidden** unless the owner explicitly requests
  it: it produces slower codegen and its size is NOT the number the owner
  tracks. `build-wasm.sh size|oz` refuses to run. Never switch configurations
  silently, and always state the configuration alongside any size figure.