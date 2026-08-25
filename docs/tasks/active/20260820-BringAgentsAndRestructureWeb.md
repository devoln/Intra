---
title: "Bring `.agents` from `dev-next` and clean up `web/` artifacts layout"
status: "in_progress"
created: 2026-08-20
started: 2026-08-20
updated: 2026-08-20
risk_level: medium
related_files:
  - .agents/
  - AGENTS.md
  - .gitignore
  - scripts/build-wasm.sh
  - scripts/build-wasm-size.sh
  - scripts/build-web.js
  - web/index.html
  - web/synth.js
related_decisions:
  - docs/decisions/active/20260820-WebBuildArtifactsLayout.md
related_skills:
  - shared-task-init-discipline
  - shared-worklog-discipline
  - shared-decision-discipline
  - shared-repo-automation-discipline
---

# Task

## Goal

Bring the `.agents` automation canon (and `AGENTS.md`) from `dev-next` into the
current `synth-wip` branch, so the agent maintains tasks/worklog/decisions going
forward. Restructure the `web/` directory so that hand-written source files
(`index.html`, `synth.js`) live in `web/`, generated artifacts (`IntraSynth.js`,
`IntraSynth.wasm`, `MusicSynthesizer.js`, `MusicSynthesizer.wasm`) live in
`web/generated/`, and the deploy bundle is assembled into `dist/` by
`scripts/build-web.js`. All three of these directories relate cleanly to
`.gitignore` rules so the repo only carries source.

## Out of Scope

- `CLAUDE.md` / `.windsurf/` content (not requested; can be added later if needed).
- Modifying `Demos/MusicSynthesizer/CMakeLists.txt` (pre-existing uncommitted
  change carried over from before this task).
- Deploy/preview wiring change beyond what the `web/` restructure demands.
- Any git operation (commit / push / force / reset). Per user, all git is
  opt-in from now on.

## Loaded Skills

- `shared-task-init-discipline` — frame the work, capture risk + invariants.
- `shared-worklog-discipline` — keep this lean.
- `shared-decision-discipline` — record the `web/` artifact layout as a decision
  trace, do not silently make the choice.
- `shared-repo-automation-discipline` — keep build entrypoints thin and
  declarative, do not bury the rule in the skill.

## Milestones

- [x] `M1:` Read relevant skills + current `build-wasm.sh` / `build-web.js`.
- [ ] `M2:` Create worklog (`docs/tasks/active/20260820-...md`).
- [ ] `M3:` Create decision trace (`docs/decisions/active/20260820-...md`).
- [ ] `M4:` Bring `.agents/` and `AGENTS.md` from `dev-next`.
- [ ] `M5:` Move generated artifacts out of `web/` into `web/generated/`.
- [ ] `M6:` Update `.gitignore` (drop `/web/`, add `/web/generated/` and
  `/dist/`).
- [ ] `M7:` Update `scripts/build-wasm.sh` and `scripts/build-wasm-size.sh`
  to stage into `web/generated/`.
- [ ] `M8:` Update `scripts/build-web.js` to assemble
  `web/` + `web/generated/` → `dist/`.
- [ ] `M9:` Rebuild wasm, run `build-web.js`, verify `dist/` has the four
  required files.
- [ ] `M10:` Report final state to the user. Await explicit git command.

## Invariants

- `INV-1:` The four required assets must exist in `dist/` after
  `node scripts/build-web.js`: `index.html`, `synth.js`, `IntraSynth.js`,
  `IntraSynth.wasm`.
- `INV-2:` `web/index.html` and `web/synth.js` must remain identical to HEAD
  before and after the task (only their tracking status changes; no content
  edits unless the task requires it — it does not here).
- `INV-3:` `web/generated/` and `dist/` must be `.gitignore`d (so they cannot
  accidentally be committed and they are reproducible from `scripts/...`).

## Deterministic Checks

- [ ] `git ls-files | grep '^ web/' | sort` lists exactly
  `web/index.html`, `web/synth.js`, NO `web/IntraSynth.*`,
  NO `web/MusicSynthesizer.*`, NO anything under `web/generated/`.
- [ ] `node scripts/build-web.js` exits 0.
- [ ] `ls dist/{index.html, synth.js, IntraSynth.js, IntraSynth.wasm}` succeeds
  and `file dist/IntraSynth.wasm` reports `WebAssembly (wasm)`.
- [ ] `wc -c dist/IntraSynth.wasm` matches `wc -c web/generated/IntraSynth.wasm`.

## Reality Verification

- Real wasm build runnable end-to-end (`build-wasm.sh` → `build-web.js`).
- The four required assets are byte-stable across the rebuild and are valid
  wasm/JS (no truncation, no syntax error from `node -c`).

## Persona Review

- `BLOCKER:` none
- `RISK:` Build scripts must remain single-source-of-truth. If anyone can
  drop a `.wasm` into `web/` directly, the layout invariant breaks.
  → enforced by `.gitignore` + comment in build scripts.
- `NOTE:` `web/MusicSynthesizer.{js,wasm}` have no in-repo script that I can
  see that produces them today. Pre-existing untracked-but-clearly-output
  artifact. If the user wants a script for those, it can be added later
  (out of scope here).

## Needs Human Verification

- `Priority:` medium
  `Area:` synthesized-acoustic-piano timbre regression check
  `Why human:` the user has been iterating on acoustic-piano sound all day
  with their own ears; only they can confirm the rebuild still sounds the
  same after the `web/` reshuffle.
  `Steps:` load the page, press the keys from earlier (C3, F3, C6, D5/E5).
  `Expected:` identical timbre/attack to the previously verified "morning"
  state.
  `Observed by agent:` not yet (this task does not change the wasm content,
  only its on-disk location).
  `Devices / environments:` user’s preferred browser/HTTP setup.

## Session Log

### Session 1 (2026-08-20)

- Done: read relevant skills; built current mental model of `build-wasm.sh`,
  `build-web.js`, current `web/` contents; created this worklog and the
  decision trace.
- Verified: dev-next branch fetched locally (`git fetch origin dev-next`),
  `.agents/` tree enumerated, AGENTS.md read.
- Next step: extract `.agents/` and `AGENTS.md`, restructure `web/`,
  rebuild wasm, smoke-test build-web.js.

## Surprises

- `web/MusicSynthesizer.{js,wasm}` exist but no in-tree script produces them
  today — they appear to be built manually and then staged. The same
  shakedown applies to any further `web/MusicSynthesizer*` change.
- `/scripts/` is gitignored wholesale, which collaterally excludes the
  legitimate build/serve/bench/smoke scripts that ARE hand-written
  (`build-wasm.sh`, `build-wasm-size.sh`, `build-wasm-simd.sh`,
  `build-web.js`, `serve.js`, `bench-native.sh`, `bench-native.cpp`,
  `smoke-intrasynth.js`, `smoke-live-midi.js`, `smoke-test-wasm.mjs`,
  `generate-piano-regions.js`). They work locally; the previous gitignore
  was a blanket. By the same logic the user gave for `web/{index.html,
  synth.js}` ("ручописные файлы нужны — сделай исключение"), each build/
  serve script should likely be re-included. Left for explicit user
  decision; not committed in this task. Two reasonable shapes:
  - add `!/scripts/<each build script>` re-include patterns;
  - move the build/serve scripts into a tracked `tools/` directory and
  keep `/scripts/` as a pure scratch area.

## Deferred

- Adding `AGENTS.md` "Loaded Skills" auto-derive (would need a script that
  walks `.agents/skills/`).
- Tracking `web/index.html`/`web/synth.js` in CI for syntactical sanity.
- Build-/serve-script tracking rule (see Surprises).

## Next Safe Step

Run `sh scripts/build-wasm.sh && node scripts/build-web.js` to verify the new
pipeline produces the expected `dist/`. Do not run any git command. Surface
state to the user and wait for their explicit commit/push instruction.
