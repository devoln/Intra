---
title: "Verify no audio clipping/overflow in WebAudio pipeline"
status: "draft"
created: 2026-08-20
started: 2026-08-20
updated: 2026-08-20
risk_level: low
related_files:
  - web/synth.js
  - intrasynth/src/Intra/Synth/MidiSynth.cpp
  - intrasynth/src/Intra/Synth/EmscriptenInterface.cpp
related_decisions: []
related_skills:
  - shared-task-init-discipline
---

# Task

## Goal

Confirm that the audio pipeline does not produce clipping or overflow artifacts.
The DynamicsCompressor is already disabled for AcousticPiano. Need to verify
that signal levels stay within safe bounds across all notes and velocities.

## Out of Scope

- Re-enabling DynamicsCompressor (user confirmed it should stay off)
- Adding a compressor checkbox UI (already discussed, not needed now)

## Current State

- Analysis shows peak levels well below 0 dB: max 0.44 (−7.1 dB) at key 84
  (C6) velocity 100
- No audible clipping reported by user
- WebAudio's built-in float32 pipeline handles ±1.0 naturally; overflow would
  only occur if the WASM output exceeds float range

## Milestones

- [ ] `M1:` Run peak analysis across all tested keys at velocity 100
  (already done: max −7.1 dB)
- [ ] `M2:` Test at maximum velocity (127) to check worst case
- [ ] `M3:` Test polyphonic playback (multiple notes simultaneously) for
  accumulated overflow
- [ ] `M4:` Confirm results with user

## Invariants

- `INV-1:` Peak output must stay below 1.0 (0 dB) for any single note at
  any velocity
- `INV-2:` Polyphonic sum must stay below 1.0 for typical playing (3–4 notes)

## Deterministic Checks

- [ ] Peak level analysis script at velocity 127
- [ ] Polyphonic peak test (if feasible)

## Session Log

### Session 1 (2026-08-20)

- Done: single-note peak check at velocity 100 — all safe (max −7.1 dB)
- Verified: no clipping artifacts heard
- Next step: test at velocity 127 and polyphonic if needed

## Next Safe Step

Run velocity-127 peak test; if safe, mark task done.

### Session 2 (2026-08-28): full-generation "detached ArrayBuffer" crash

User: "Периодически при полной генерации в начале или даже в середине процесса
вылезает ошибка: TypeError: Cannot perform Construct on a detached ArrayBuffer".

Root cause (web/synth.js, generateAll): the wasm heap view was captured once
before the render loop — `const heap = Module.HEAPF32`. When Emscripten grows
wasm memory mid-render (voice/partial/instrument allocations inside
_SourceGetUninterleavedSamples), the old ArrayBuffer is detached and
`heap.subarray(...)` throws exactly "Cannot perform Construct on a detached
ArrayBuffer". Intermittent because growth depends on session/file state.

Fix: re-read `Module.HEAPF32` on every chunk inside the loop (Emscripten's
updateMemoryViews keeps Module.HEAPF32 pointing at the current buffer after
each grow). Same stale-view pattern fixed in the __synthDebug.outputStats
test hook. applyRenderParams (view written before the only wasm call) and the
per-callback audio path were already safe.

Verified: .scratch/probe-detach-fix.js reproduces the exact error through the
real _emscripten_resize_heap path (stale view throws, fresh view works);
.smoke probe renders heavy polyphonic+96-program+reverb files with no error;
smoke-test-wasm.mjs and smoke-live-midi.js pass; dist rebuilt, dist/synth.js
== web/synth.js.
