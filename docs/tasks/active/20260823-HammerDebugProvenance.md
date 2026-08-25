---
title: "Verify current piano hammer debug audio provenance"
status: "completed"
created: 2026-08-23
started: 2026-08-23
updated: 2026-08-23
risk_level: low
related_files:
  - web/index.html
  - scripts/make-hammer-debug.js
  - web/generated/IntraSynth.js
  - web/generated/IntraSynth.wasm
  - dist/IntraSynth.js
  - dist/IntraSynth.wasm
related_tasks:
  - 20260820-PianoAttackTimbreFix
---

# Task

Ensure the browser debug spoiler plays the current physical hammer-only render,
not WAV files left by an earlier sine or sample-backed experiment.

## Completed Work

- `scripts/make-hammer-debug.js` removes the legacy `hammer_sample_*` and
  `hammer_our_*` files before generating debug output.
- The SF2 comparison files use the explicit prefix
  `hammer_reference_residual_*`.
- The current WASM hammer-only render uses the explicit prefix
  `hammer_physical_current_*`.
- `web/index.html` references only the explicit current names for keys 84, 96,
  99, and 105.
- The WAV files were regenerated from the current
  `web/generated/IntraSynth.js` and `web/generated/IntraSynth.wasm`.
- The web output was rebuilt into `dist`.
- `web/generated/IntraSynth.js` and `dist/IntraSynth.js` are byte-identical.
- `web/generated/IntraSynth.wasm` and `dist/IntraSynth.wasm` are byte-identical.
- No legacy hammer WAV names or embedded `PianoHammers` table are present in
  the current generated output.

## Human Verification

Open the debug spoiler after restarting the preview and listen to the files
whose names include `physical_current`. This is a diagnostic comparison only;
it does not change the compact synthesizer or embed audio samples into it.
