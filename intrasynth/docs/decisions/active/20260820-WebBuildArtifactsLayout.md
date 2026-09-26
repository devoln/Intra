---
title: "Web build artifacts layout: hand-written in web/, generated in web/generated/, deploy bundle in dist/"
status: "active"
created: 2026-08-20
updated: 2026-08-20
task: "docs/tasks/active/20260820-BringAgentsAndRestructureWeb.md"
tags: [repo-layout, build-output, gitignore, web-player]
supersedes: []
superseded_by: []
---

# Decision

## What Was Decided

The `web/` directory is split into three distinct layers, each with a single
gitignore rule so the tracked tree only ever carries hand-written source:

- `web/<hand-written>.{html,js}` — UI source, tracked.
- `web/generated/...` — every artifact produced by the build pipeline
  (currently `IntraSynth.js`, `IntraSynth.wasm`, `MusicSynthesizer.js`,
  `MusicSynthesizer.wasm`), untracked.
- `dist/...` — final assembled deploy bundle, untracked.

The build pipeline is:

1. `scripts/build-wasm.sh` (and `scripts/build-wasm-size.sh`) →
   `web/generated/IntraSynth.{js,wasm}`.
2. `scripts/build-web.js` → copies `web/` + `web/generated/` → `dist/`.
3. `.gitignore` excludes `/scripts/` (analysis/setup scratch), `/web/generated/`
   and `/dist/` (build output).

## Context

Before this decision, `web/` carried both hand-written source
(`index.html`, `synth.js`) and binary build output (`IntraSynth.wasm`,
`MusicSynthesizer.wasm`, `IntraSynth.js`, `MusicSynthesizer.js`). The previous
commit shoved the whole `web/` directory into `.gitignore`, which is correct
in spirit for the binaries but collateral damage for `index.html` and
`synth.js` (the only copies of the UI source). The user explicitly asked for
a tighter rule:

> "Если index.html и т.п. рукописные файлы нужны, сделай исключение. Лучше
> все генерируемые файлы положить в отдельную подпапку или всё собирать в
> некий dist, который можно держать в .gitignore."

## Options Considered

- `A:` Keep `web/` fully gitignored as it is, lose tracking of
  `index.html`/`synth.js`.
  → Simplest, but removes the only copy of the UI source from version
  control; deploys from a clean checkout still need the wasm (which the
  deploy image cannot build because it is Node-only per the comment in
  `scripts/build-wasm.sh`), so the situation was already fragile, and this
  makes it worse.
- `B:` Distinguish tracked vs untracked with explicit `web/*.wasm`,
  `web/IntraSynth.js`, `web/MusicSynthesizer.*` ignore patterns; leave
  `web/{index.html,synth.js}` tracked.
  → Works. Zero source-layout churn. But: the rule is implicit
  ("anything wasm-shaped") and takes attention to keep in sync if the build
  ever adds `web/FooSynth.*`.
- `C:` Split: hand-written source in `web/`, generated in `web/generated/`,
  final bundle in `dist/`. Therefore `.gitignore` only mentions directories,
  not globs.
  → Slightly more layout churn (move four files; update two build scripts).
  Pays for itself with a single rule per directory and a one-liner
  comment in build scripts.

## Why This Path Was Chosen

- Option `C` keeps the rule name and the directory name identical —
  impossible to forget for future contributors.
- Aligns with the existing deploy script: `scripts/build-web.js` already
  produces `dist/`, and just needs to do `web/` + `web/generated/` → `dist/`
  instead of `web/` → `dist/`.
- Avoids file-shape glob (`*.wasm`) that future additions would silently
  match.

## Consequences

- `web/` is now small (just two tracked files). Trivial to see the UI
  source in a tree view, trivial to audit, trivial to diff.
- The build scripts change from "stage into `web/`" to "stage into
  `web/generated/`".
- `dist/` stays gitignored (it always was); only the assembly rule changes.
- The hand-written `index.html/synth.js` is again version-controlled —
  restores the invariant the prior `web/`-wide gitignore broke.

## What Should Change Downstream

- `.gitignore` should mention `web/generated/` and `dist/` and stop listing
  `web/`.
- Any future build target that emits a wasm/loader file should default to
  `web/generated/`, not `web/`.
- A lint or comment near `web/index.html` could remind contributors that
  anything directly added to `web/` becomes version-controlled; generated
  artifacts must go in `web/generated/`.

## Review Notes

- `Privacy / secrets:` none — wasm is generated from open-source C++.
- `Reliability:` the four-file invariant for `dist/` keeps the deploy
  contract small and checkable.
- `Cognitive cost:` low — three named directories is easier than three
  implicit rules.
- `RISK consciously accepted:` if a contributor drops a wasm into `web/`
  by mistake and not into `web/generated/`, the next `.gitignore`-aware
  diff will catch it. The trivial mistake would be visible in
  `git status` as an unexpected `web/*.wasm` file.
