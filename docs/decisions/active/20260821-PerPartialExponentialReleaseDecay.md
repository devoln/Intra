---
title: "Per-partial exponential release decay (replaces linear ADSR fade)"
status: "active"
created: 2026-08-21
updated: 2026-08-22
task: "docs/tasks/active/20260820-PianoAttackTimbreFix.md"
tags: [audio-engine, note-off, physical-model, release-decay, do-not-revert]
supersedes: []
superseded_by: []
---

# Decision

## What Was Decided

When a piano key is released, the damper **adds** its decay to the
string's natural decay: `dec_release(p) = dec_natural(p) ×
dec_damper(p)` (multiply, not replace). Physically the damper adds
its own energy-loss rate to the string's inherent losses, so a
released note always dies at least as fast as a held one.

The damper's per-harmonic rate is

    λ_damper(k) = (1/0.28 s) · √(f_note/f_C4) · √k

so at C4 the fundamental gets τ=280 ms and k=8 gets τ≈99 ms — a
√k spread (≈2.8×), NOT 1/k (8×). The softer spread keeps the timbre
from collapsing to a pure fundamental on short notes; the earlier
`replace` model (dec[p] = damper step alone) sounded like a strange
artifact on staccato — that's why it's a multiplier now.

The release decay is **baked into the existing hot-loop coefficients**
(`dec[p]`) — zero additional per-sample computation. `NoteRelease()` does
one O(count) pass to multiply `dec[p] *= mDecayRelease[p]` and clear
`atk[p]`, then the normal oscillator loop handles everything.

**Do NOT revert to a linear fade or a uniform (non-per-harmonic) ADSR
release.** When calibrating the synth against SF2 samples, the release
behavior is an intentional design choice, not a bug to "fix" by going
back to ADSR.

## Context

### What the old ADSR did

`AcousticPiano` had `g.Envelope = MakeWebEnvelope({0, 0, 1, 0.6f, 0,
false, true})` — an ADSR with:
- Attack=0, Decay=0, Sustain=1 (no-op during sustain)
- Release=0.6 s (linear fade to silence; ~-6 dB at 300 ms)
- `exponential=false` → **linear** fade
- Applied to the **entire summed signal** — one gain for all harmonics

This was removed to enable the stereo path (`fillStereo` requires
`Modifiers.Empty() && !ADSR`). The ADSR was the only thing handling
note-off, so removing it broke note-off entirely until
`AdditiveSampler::NoteRelease()` was added.

### Why the old approach was wrong

1. **Linear fade** sounds unnatural — real dampers cause exponential
   amplitude decay (constant energy loss rate, not constant amplitude
   change rate).
2. **Uniform gain** — all harmonics fade at the same rate. In a real
   piano, the damper contacts the string and damps higher-frequency
   modes more efficiently (shorter wavelength = more friction per
   unit length).
3. **Extra pass** — ADSR applied a gain multiplier in a separate loop
   (`applyModifiers`) after the oscillator render. The new approach
   replaces `dec[p]` in-place, so the existing `av = av*mv + ak` line
   in the hot loop does all the work.

### What fluidsynth does

fluidsynth uses the SF2 sample's natural decay (no separate release
envelope). The sample plays once and stops. For note-off,
fluidsynth applies a quick fade on the sample playback, which is
also uniform (not per-harmonic). Our approach is **physically more
correct** than both the old ADSR and fluidsynth.

## How It Works

1. **Constructor** computes `mDecayRelease[o] = exp(-1/(τ·SR))` where
   `τ = 0.28·sqrt(261.63/freq) / sqrt(k)` (k = harmonic number, freq =
   note frequency) — the damper's OWN step.
   - C4 k=1: τ=280 ms; C7 k=1: τ≈106 ms; C3 k=1: τ≈396 ms
   - k=8 at C4: τ≈99 ms (√k spread, not 1/k)
   - Stored per-oscillator alongside the normal decay steps.

2. **`NoteRelease()`** (called from `NoteSampler::NoteRelease()` on
   MIDI Note Off):
   - Sets `mReleased = true`
   - Multiplies `dec[p] *= mDecayRelease[p]` (one O(count) pass) — the
     damper ADDS to whatever natural decay segment was active, so
     release is never slower than the natural decay
   - Clears `atk[p] = 0` (attack ramp no longer needed)
   - Does NOT touch `mEndSamples` (note ends naturally when amplitudes
     are near-zero)

3. **Hot loop** (`RenderInto`):
   - Segment switching (`mDecay1→2→3→4`) is **skipped** when
     `mReleased` (the release decay stays active)
   - Boundary clamping is **skipped** (no segment boundaries to cross)
   - The normal `av = av*mv + ak` applies the release decay per-sample
   - After each block, `maxAmp` is checked; if < 1e-3 (≈ -60 dB),
     `mDone = true` → `GenerateStereo` returns 0 → `NoteSampler`
     removes the voice

4. **Voice cleanup**: `mDone` → `GenerateStereo` returns 0 →
   `fillStereo` calls `GenericSamplers.RemoveUnordered` →
   `NoteSampler::Empty()` → `Generate` returns false →
   `MidiSynth` deletes the `NoteSampler`.

## Design Constraints (Do Not Break)

- **τ must be per-harmonic**: `τ ∝ 1/√k`, not a uniform value.
  Higher harmonics must decay faster, but the spread is √k (≈2.8× at
  k=8) — a 1/k spread made short notes collapse to the fundamental
  (audible artifact).
- **Exponential, not linear**: the decay step is
  `exp(-1/(τ·SR))`, not a linear ramp.
- **No extra per-sample computation**: the decay must go through
  `dec[p]` in the existing hot loop. No separate release-modifier
  pass.
- **ADSR stays removed** for `AcousticPiano`: adding it back would
  re-break stereo (forces mono fallback path) and re-introduce the
  uniform linear release.
- **`mDecayRelease.SetCount(count)` must be called BEFORE the
  constructor loop that fills values** (not after). If SetCount is
  called after, it reallocates and zeros the array. This was the cause
  of the initial bug where release cut instantly.

## Parameters

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| damper τ for k=1 (C4) | 280 ms | Clearly audible fade; applied as multiplier on natural decay |
| τ pitch scaling | √(f_C4/f_note) | Higher notes must release faster than their natural decay (flat 350 ms made C7 ring longer when released than when held) |
| τ harmonic scaling | 1/√k | Higher harmonics decay faster, but softly — 1/k made the timbre collapse to fundamental on short notes (artifact) |
| Release application | dec[p] *= decR[p] | Damper ADDS to natural decay; replacing (dec = decR) sounded wrong on staccato |
| mDone threshold | 1e-3 | ~-60 dB from peak — inaudible in any mix; 1e-5 kept released voices alive ~2.6 s and made rendering 2-3× slower |
| atk[p] on release | 0 | Attack ramp not needed during release |

## What Could Change Later

- History: the initial 50 ms release sounded like an instant cut to
  the user (who compared against the old ADSR's 0.6 s linear fade);
  a flat 350 ms fixed that but made high notes (C7) ring LONGER when
  released than when held, because their natural decay is faster than
  350 ms. The pitch-scaled τ (√(f_C4/f)) guarantees the release is
  always faster than natural decay while keeping a clear audible fade
  on mid/bass notes.
- The initial implementation REPLACED the natural decay step with the
  damper step; the user reported short notes cut off with a strange
  artifact. Changed to a multiplier (damper adds to natural decay) and
  softened the harmonic spread to √k.
- The √k scaling could be replaced with a measured damper profile
  from real piano samples — but the physical model is a good
  approximation.
- The mDone threshold was 1e-5 (≈ -100 dB) until 2026-08-22: with
  τ=280 ms the fundamental needs ~9 τ to cross it (≈2.6 s), so every
  released note kept rendering its inaudible tail — measured 2-3×
  render slowdown. Raised to 1e-3 (-60 dB): still far below audibility,
  cleanup now happens in ~1.3-1.8 s (C4/C3) instead of 2.6-4 s.
- If stereo needs ADSR back for some reason, the solution is to make
  ADSR apply per-channel (not revert to mono fallback), NOT to remove
  the per-partial release.
