---
title: "Test and evaluate strike/hammer component for acoustic piano"
status: "draft"
created: 2026-08-20
started: 2026-08-20
updated: 2026-08-20
risk_level: low
related_files:
  - intrasynth/src/Intra/Synth/AdditiveSampler.cpp
  - intrasynth/src/Intra/Synth/InstrumentLibrary.cpp
related_decisions: []
related_skills:
  - shared-task-init-discipline
  - shared-worklog-discipline
  - shared-human-verification-queue
---

# Task

## Goal

Enable the strike/hammer component for acoustic piano, evaluate whether it
improves the attack, and decide whether to keep it enabled or disable it.

The strike is a short tonal pulse (f0/2 + f0) added in the first ~10 ms of each
note. Previous analysis showed the SF2 samples have low-frequency energy
(30–300 Hz) in the attack that our sine model lacks. The current hammer
implementation uses f0/2 + f0 tones, which may not match the actual strike
spectrum.

## Out of Scope

- Redesigning the strike model (that would be a separate task if the current
  f0/2+f0 approach doesn't work)
- Changes to the envelope correction or region data (separate task)

## Loaded Skills

- `shared-task-init-discipline` — frame the work
- `shared-human-verification-queue` — user must evaluate by ear

## Current State

- `HammerLevel` in `InstrumentLibrary.cpp` was changed from 0.0 to 0.3 in the
  current session (may or may not be the desired state)
- The hammer generates: `0.55 * eLo * sin(phLo) + 1.0 * eHi * sin(phHi)` where
  phLo = f0/2, phHi = f0, with exponential decay (τ_lo ≈ 12 ms, τ_hi ≈ 4 ms)
- At `HammerLevel=0.3`, analysis shows the early attack (2–6 ms) rises by
  5–7 dB, bringing it closer to sample values

## Milestones

- [ ] `M1:` Confirm HammerLevel=0.3 (or another value) is the desired test state
- [ ] `M2:` User listens and evaluates: does the strike improve the attack?
- [ ] `M3:` If yes: fine-tune HammerLevel and decay parameters to match sample
- [ ] `M4:` If no: disable (set HammerLevel=0.0) and document why
- [ ] `M5:` If the f0/2+f0 model doesn't work: extract actual strike spectrum
  from samples (scripts `_tmp-strike-extract.js`, `_tmp-strikewave.js` exist)
  and redesign the component

## Invariants

- `INV-1:` The strike component must not cause clipping (peak < 0 dB)
- `INV-2:` The strike must not change the steady-state sound (only affects
  first ~10 ms)
- `INV-3:` User must explicitly approve keeping or disabling the strike

## Deterministic Checks

- [ ] `node scripts/_tmp-f3check.js` — attack comparison with strike enabled
- [ ] Peak clipping check

## Needs Human Verification

- `Priority:` medium
  `Area:` acoustic piano attack quality
  `Why human:` only the user can judge whether the strike sounds like a
  "dull thump" (глухой удар) or "hiss" (шипение)
- `Steps:` Play C3, F3, C6 with strike enabled; compare with fluidsynth
- `Expected:` the attack should have more body/punch without sounding like noise
- `Observed by agent:` analysis shows 5–7 dB boost in 2–6 ms window
- `Devices / environments:` user's browser

## Session Log

### Session 1 (2026-08-20)

- Done: enabled HammerLevel=0.3, verified no clipping, verified 5–7 dB boost
  in early attack
- Verified: peaks still safe (max −7.1 dB)
- Next step: user listens and decides

## Deferred

- Strike model redesign (if f0/2+f0 doesn't match the sample's actual strike)
- Per-key strike scaling (currently uniform across all notes)

## Next Safe Step

User evaluates the current sound with strike enabled and decides keep/disable.
