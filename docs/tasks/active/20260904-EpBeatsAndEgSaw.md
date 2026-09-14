---
title: "EP2 sample beating (2 voices + flat BeatCents) + EGP C4 saw melts; sample-pitch audit completed"
status: "active"
created: 2026-09-04
started: 2026-09-04
updated: 2026-09-04
risk_level: medium
related_files:
  - intrasynth/src/Intra/Synth/AdditiveSampler.h
  - intrasynth/src/Intra/Synth/AdditiveSampler.cpp
  - intrasynth/src/Intra/Synth/InstrumentLibrary.cpp
  - scripts/generate-sf2-samples.js
  - web/generated/samples/manifest.json
related_tasks:
  - 20260828-LoudnessBalanceFix
  - 20260903-MidiTracksAndSampleTabs
---

# 2026-09-04 (night) — EP2 got its sample beating + EGP C4 saw melts

Continuation of `20260828-LoudnessBalanceFix.md` (whose file is too large for
the edit tool's snapshot this session — this entry lives here instead).

User A/B report (sample tabs, C4-C5): "EP1-2 почти 1 в 1, только биения
отличаются — в семплах они заметнее (в EP2 C5 особенно)"; "EGP C4 у нас
слишком пилообразный, а в семпле пила только немного в начале, потом чистый
звук".

## Part 1 — Sample-tab pitch audit (see also 20260903 worklog)

All 40 manifest tab WAVs were re-pitched to their labels (TRUE_PITCH table in
`scripts/generate-sf2-samples.js`, measured from audio; the SF2 shdr
originalPitch is unreliable in this bank). Verified with
`.scratch/probe-sample-pitch.js` (harmonic-sum F0): every label within ±0.35 st.
Stale leftover WAVs (E4 × 6, Flute/G5) deleted from web/generated + dist.

## Part 2 — Measurements (`.scratch/probe-am-vs-sample.js`, early-body extrema depth)

Per-partial AM depth (max→min dB over 0.15-1.6 s, decay-robust extrema) at the
partial's own peak frequency, render vs tab WAV:

| case | h1 | h2 | h3 |
|---|---|---|---|
| EP2 C5 render | 0.0 | — | — |
| EP2 C5 sample | **6.9 @~2-3 Hz** | 3.0 | 5.3 |
| EP2 C4 render | 0.0 | — | — |
| EP2 C4 sample | **6.7** | 3.2 | 4.1 |
| EP1 C4/C5 render | 0-1 | 0-4 | 0-5 |
| EP1 C4/C5 sample | 0-1.5 | 0.4-2.7 | 0.5-6.7 |
| EG C4 render | 0.0 | 8.7 | 0.4 |
| EG C4 sample | 12.6 | 9.7 | 3.0 |

- **EP2 root cause**: `UnisonVoices=1` — a single lane cannot interfere, so the
  fundamental sat perfectly flat while the DX7 Soft sample swells ~7 dB at
  ~2-3 Hz (tine chorus). EP1's samples barely beat on h1-h3 (0-0.5 dB) — our
  EP1 (0.8c/2 voices) already sits in that band, left unchanged.
- **EG C4**: brightness curve (h2..h10 / h1, 250 ms windows) shows the sample
  oscillating around −6..−8 dB with beat spikes while ours held a steady
  −2.5..−4 dB through ~1.5 s — the "постоянная пила". The XP50 upper partials
  decay faster in the early body; ours did not.

## Part 3 — Changes

1. **`AdditiveSampler.{h,cpp}` — new `BeatCents` ctor param (0 = off, default)**.
   When > 0 it REPLACES the region beat ladder (calibrated to Clavinova strings;
   it would scale mid-key beats to ~0.2×) with a FLAT spread, and switches the
   per-partial beat-weight profile to the EP shape (h1 full, h2/h3 half, h4+
   quarter — all partials beat together like the sample chorus). Acoustic and
   every existing instrument are untouched (param defaults 0).
2. **EP2** (`InstrumentLibrary.cpp`): `UnisonVoices 1→2`, `DetuneCents 6.0` +
   `BeatCents 6.0` (13th initializer) → C5 h1 beats ~2.2 Hz, C4 ~1.1 Hz,
   depth ~8 dB on h1 / ~4.6 dB on h2-h3. `VolumeDb −8.6→−7.2`: two voices add
   +3.2 dB peak (g0+g1) and the beat lowers mean RMS03 by ~2.4 dB
   (avg E² = (1+r²)/2, r=0.379) — recalibrated to the SF2 target −5.8 dB
   (`scripts/_tmp-instlevel.js`: EP2 = −5.8 dB exactly; EG +0.5, EP1 +5.9,
   Harpsi −3.9, Clav +0.7 all unchanged at target).
3. **EG** (`InstrumentLibrary.cpp`): `Brightness 0.45→0.40` (upper partials
   −1..−2 dB via k^0.08) + `DecayStiffness 0→0.02` (λ·(1+0.02·k²) — h8 melts
   ~2.3× faster, the saw tapers like the XP50 sample).

## Part 4 — Verification (dist rebuilt 2026-09-04 21:11, IntraSynth.wasm 176,945 B)

- EP2 C5 h1 AM depth **8.2 dB** (sample 6.9), rate ~2.2 Hz; probe-held2 RMS
  wobbles ±3-4 dB at ~2 Hz over the hold (was flat). EP2 C4 h1 10.7 dB
  (sample 6.7 — slightly deeper; acceptable, user will judge).
- EG C4 brightness gap vs sample roughly halved (steady −2.5..−4 → −3.6..−4.7
  vs sample median ~−6..−8); attack unchanged by design (sample has saw there).
- `scripts/smoke-test-wasm.mjs` + `scripts/smoke-intrasynth.js` pass;
  acoustic and all other instruments' levels unchanged; dist == web/generated
  (md5). EP2 bass (C2-C3) now gets a slow ~0.3-0.5 Hz swell from the flat
  spread — note for future listening, not a target of this pass.

Next: user listening pass at C4-C5; BeatCents (rate) and the 0.45 g1 depth cap
are the knobs if EP2 needs to be calmer or livelier.