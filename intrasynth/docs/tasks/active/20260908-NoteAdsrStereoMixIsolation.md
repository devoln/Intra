---
title: "Honest-stereo ADSR/mix isolation: a note's envelope must not multiply the accumulated mix of other voices"
status: "active"
created: 2026-09-08
started: 2026-09-07
updated: 2026-09-08
risk_level: high
related_files:
  - intrasynth/src/Intra/Synth/NoteSampler.cpp
  - intrasynth/src/Intra/Synth/NoteSampler.h
  - intrasynth/src/Intra/Synth/ADSR.h
  - intrasynth/src/Intra/Synth/Envelope.h
  - intrasynth/src/Intra/Synth/SamplerTask.h
  - intrasynth/src/Intra/Synth/MidiSynth.cpp
related_tasks:
  - 20260907-AudioCallbackWavetablePreload
  - 20260904-EpBeatsAndEgSaw
---

# Task

User report (Celine, then tous les garçons): after the "honest stereo" render
path (Update 18, uncommitted), the piano *breaks the sound of neighboring
instruments* — a note of one instrument ducks/fades the other instruments that
are already sounding. "Not voice killing, but an incorrect reuse/mutation of
the envelope", "a general problem for all instruments with ADSR/Envelope —
shared mutable state". Replacing EP2 with Vibraphone in Celine made the bug
go away; AGP (no envelope) never showed it.

## Root cause (from code)

The frame mixer (`SamplerTaskContext`, `SamplerTask.h`) keeps TWO shared
channel buffers (`allSamples[2*frameLength]`). Every voice task accumulates
its source signal into these buffers with `+=`; the final buffer IS the mix.

`NoteSampler::GenerateStereo` (Update 18) started rendering *all* voices
through `fillStereo(dstL, dstR)` followed by `applyModifiersStereo(dstL,
dstR)` — and `applyModifiersStereo` applies the voice's `GenericModifier`s
and its **ADSR envelope in place on those shared buffers**. The old path
rendered notes with modifiers/ADSR into a local mono temp buffer and only
then panned the result into the mix, so the envelope touched only the voice's
own signal.

Consequence, deterministic in single-threaded wasm (task order = voice
creation order): when voice R's task runs after voice B's task in a frame, R's
`ADSR(dstL)` multiplies B's already-accumulated samples by R's envelope. All
`MakeEnvelope({0,0,1,release,0,false,true})` instruments (EP2, all guitars,
Sitar, etc.) have a sustain of 1.0 but a **linear 1→0 release on note-off** —
so every released note multiplies every earlier-created voice by its decaying
release factor for the whole release length (EP2: 1.2 s; guitars 0.7–1.5 s).
That is exactly the reported "EP2 breaks neighbors" (EP2 has a long release +
dense chords), "tous les garçons" (guitars only), and why Vibraphone
(envelope applied *inside* `WaveTableSampler` per-voice before adding) and
AGP (no note-level envelope) are clean. It also made output depend on the
pull/frame chunk size and on parallel task order (native), which is what the
`.scratch/frame-dep.mjs` / `bisect-chunkdep` probes showed.

## Fix

`NoteSampler::GenerateStereo`: when the voice has modifiers/ADSR, render it
into a **zeroed chunk of the frame region** (bounded 1024-sample stack
scratch, like the old mono path), apply the modifiers/ADSR there, and add the
previously saved mix back:

1. save the accumulated mix chunk (`CopyTo`),
2. zero the region,
3. `fillStereo` the voice into the zeroed region,
4. `applyModifiersStereo` (now touches only this voice's signal),
5. `Add` the saved mix back.

Voices without modifiers/ADSR keep the direct `fillStereo` fast path (sources
only add; no in-place multiplication).

The two earlier ADSR fixes were re-checked and are NOT redundant — both stay:

- the L/R snapshot-restore in `applyModifiersStereo` (apply the same
  time-only envelope schedule to both channels);
- the eager segment transition at buffer ends in `ADSR.h` (a release that
  ends exactly on a chunk boundary must flip `Active=false`, otherwise the
  voice lingers forever as a silent zombie — `!ADSR` short-circuits before
  the kill check).

## Verification (wasm rebuilt, `dist/` reassembled)

`render-override.mjs` renders (44.1 kHz, 256-sample pulls, same invocation
verified by md5 against the stored references):

| comparison | divergent 0.5 s windows (>3 dB RMS) | direction |
|---|---|---|
| celine bug  vs pre-stereo ref | 18 | all NEGATIVE (dips to −15.8 dB at 253–258 s) |
| celine fixed vs pre-stereo ref | **0** | — |
| celine fixed vs bug | 16 | all negative (bug quieter — ducking removed) |
| tlg bug  vs pre-stereo ref | 201 | mostly +3..6 dB (intended KS stereo gain) + dips |
| tlg fixed vs pre-stereo ref | 367 | ALL positive +3..6 dB (intended KS stereo gain only) |
| tlg fixed vs bug | 22 | all negative (guitar-release ducking removed) |

- The pre-stereo reference (committed HEAD) already renders ADSR voices
  voice-only (mono temp path), so it is the correct reference for this bug.
- celine fixed matches the reference in every window; the only remaining
  tlg difference vs the reference is the intended +3–6 dB KS stereo gain
  (KarplusStrong now writes full level to both channels).
- No non-finite samples; peaks/RMS sane (ab_fixed rms 0.0464 vs pre 0.0475,
  bug 0.0446 — bug was ducked lower; tlg fixed rms 0.163 vs bug 0.140).
- `scripts/smoke-intrasynth.js` / `scripts/smoke-test-wasm.mjs` PASS.

## Performance follow-up (2026-09-08)

A fresh five-melody A/B was run at 48 kHz stereo with 4096-sample pulls,
full-file rendering, and the median of five runs per melody. The benchmark
includes source creation and rendering; the median reduces first-run/JIT/cache
noise while keeping the measurement comparable across variants.

Variants:

- **HEAD**: `fe9f14e`, freshly built with the same Release/Emscripten/SSE2
  settings (`164,849 B` WASM).
- **No isolation**: current working tree with only the new save/zero/render/
  add mix-isolation block replaced by the pre-fix direct stereo path
  (`198,031 B` WASM).
- **Current**: current working tree, including mix isolation (`198,269 B`
  WASM).

Higher `xRT` is faster; wall time is the measured render time.

| melody | HEAD | no isolation | current | current vs HEAD |
|---|---:|---:|---:|---:|
| Celine (282.0 s) | 1,832 ms / 154.0x | 715 ms / 394.6x | **816 ms / 345.6x** | **2.24x RT** |
| Merry Christmas (67.3 s) | 113 ms / 595.2x | 123 ms / 544.5x | **155 ms / 433.7x** | 0.73x RT |
| Tous Les Garcons (191.1 s) | 328 ms / 582.7x | 277 ms / 688.6x | **374 ms / 510.3x** | 0.88x RT |
| Chopin (290.5 s) | 2,963 ms / 98.1x | 2,892 ms / 100.5x | **2,900 ms / 100.2x** | 1.02x RT |
| Cherilady flute (215.3 s) | 96 ms / 2,254x | 126 ms / 1,708x | **125 ms / 1,721x** | 0.76x RT |

The attribution is therefore:

- The **honest-stereo and earlier instrument/flute work** account for the
  large Celine improvement; the no-isolation variant is already `2.56x RT`
  versus HEAD. The earlier stereo A/B also showed stereo itself neutral or
  faster than the old mono temporary-buffer path.
- The **latest mix-isolation fix costs real CPU** only where note-level ADSR or
  modifiers are active: relative to no isolation, wall time increases by
  about 14% (Celine), 26% (Merry Christmas), and 35% (Tous Les Garcons).
  It is effectively neutral on Chopin (+0.3%) and the flute file (-0.8%, within
  timing noise). This is the expected cost of the two saves, two clears, and
  two mix-add passes per affected frame chunk; it is required to prevent one
  voice's envelope from multiplying the shared mix.
- **Chopin is unchanged** within measurement noise, because its dominant piano
  path does not use the affected note-level ADSR isolation path.
- **The flute became slower versus the commit**, not faster: `2,254x` to
  `1,721x` realtime (`+31%` wall time). This is from the richer current flute
  implementation (multi-zone tables, vibrato, and breath/noise layers), not
  from the mix-isolation fix: current and no-isolation flute timings are the
  same within noise. The dedicated 17-second GM-73 probe likewise measured
  `7.0 ms` on HEAD versus `10.4 ms` current.

The speed result does not justify removing the isolation block: that block is
what eliminates the ADSR release ducking of neighboring voices. The measured
trade-off is documented here so future optimizations can target its temporary
copy/clear/add passes without reopening the audio corruption.

## Needs Human Verification

| Priority | Area | Why human | Steps | Expected |
|---|---|---|---|---|
| High | Celine / any file with EP2 (or any envelope instrument) playing over other instruments | The ducking is an audible artifact | Play Celine, tous les garçons in the preview; listen at ~195–280 s (Celine) and throughout (guitars) | Neighbors no longer dip/fade when a note ends; notes of one instrument no longer affect others |

## Session log

- 2026-09-07/08: user narrowed the bug to instruments with the note-level
  ADSR; probes (frame-dep, bisect-chunkdep, isolate-253) showed
  chunk-size-dependent output. Code review found the shared-frame-buffer
  in-place ADSR multiplication; implemented the save/zero/render/apply/
  restore isolation in `NoteSampler::GenerateStereo`; rebuilt wasm; verified
  the ducking windows (16–18) are eliminated on both files.

## Next-Step Handoff

- Listen in the preview (Needs Human Verification).
- If the intended KS stereo gain (+3–6 dB on guitars-only) is too loud, that
  is a separate level-calibration matter (KS `GenerateStereo` full-level per
  channel), not this bug.