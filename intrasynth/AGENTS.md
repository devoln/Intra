Use relevant skills from `.agents/skills/`.

For non-trivial work, the default runtime path should come from thin skills, especially shared task-init / worklog / closeout discipline, not from large maintainer docs.

## Session start

- Inside a Freebuff/Codebuff Cloud workspace, load
  `.agents/skills/stack-freebuff-cloud-session/SKILL.md` first: it carries the
  environment rules (sandbox, editing, preview, git, always-on duties) for every turn
  there.
- Project orientation is one command, not a repository sweep:
  `node intrasynth/tools/agent/orient.mjs` (`--brief` for the short form,
  `--check` to verify the map is still accurate).
- The project map is `intrasynth/docs/Architecture.md`; for any sound work load
  `intrasynth/.agents/skills/project-intra-fitting-and-measurement/SKILL.md`.
- Do not rediscover the layout: `scripts/`, `intrasynth/web/generated/`, `dist/` and
  `.scratch/` are gitignored and therefore invisible to file search, yet they exist
  and are load bearing. `intrasynth/docs/Architecture.md` lists them.

Whatever you do, from designing a system to tiny bugfix, you MUST always keep in mind and follow `intrasynth/docs/Rules.md`.

Task/history documents live in `intrasynth/docs/tasks/`: `active/`, `planned/`, `archived/`.

Architecture and trade-off records live in `intrasynth/docs/decisions/`: `active/`, `accepted/`, `archived/`.

## Language (owner's rule)

- Anything that is not reasoning or code comments must be **Russian only**: agent replies, worklog/task/decision documents, change descriptions, WASM size reports, and so on.
- Exceptions: internal reasoning, code comments, and technical identifiers (file, function, and variable names).
- **Commit messages are English only** (subject and body). This is the single exception to the Russian-language rule for text.

Task and decision file names should use the format `YYYYMMDD-UpperCamelCase.md`.

If work continues for a while in one direction without an assigned task, and decisions or trade-offs are being made, create or attach a task/worklog before continuing further.

High-level process artifacts and shared skills in `.agents/` must not be changed without explicit owner agreement.

Temporary experiment output (ad-hoc build trees, probes, scratch renders) must go under `.scratch/` and stay gitignored; never leave untracked `build-*/` or `probe-*` dirs in the repo root (`/.scratch/`, `/build-*/`, `/probe-tmp/` are in `.gitignore`).

## IntraSynth WASM build mandate (Intra project)

- The canonical, owner-tracked build is **plain `-Os` everywhere with
  `INTRA_PIANO_ALL_TABLES=ON`** (`sh scripts/build-wasm.sh`, ~216 KB on the
  current instrument set). It is the ONLY configuration whose size the owner
  tracks and wants reported.
- **Every modification to the synth code must rebuild the canonical config
  (`sh scripts/build-wasm.sh`), stage it (`intrasynth/web/generated/` + `dist/` via
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

## IntraSynth subtree (interim step towards a separate repository)

This tree is the synth on its way out of the Intra repository: `intrasynth/` will
become the root of its own repository. It already carries its own docs
(`intrasynth/docs/`), web player (`intrasynth/web/`), agents setup
(`AGENTS.md`, `.agents/` — this file and this directory). The fitting/measurement
skill lives here; repo-agnostic shared skills stay at the Intra root. Gitignored
workspace dirs that still sit outside (`scripts/`, `dist/`, `.scratch/` at the
Intra root) will move here at the actual split.
