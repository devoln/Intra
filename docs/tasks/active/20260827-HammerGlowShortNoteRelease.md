---
title: "Hammer h3-h6 glow overlay + 35 ms free-attack release window"
status: "active"
created: 2026-08-27
started: 2026-08-27
updated: 2026-08-27
risk_level: medium
related_files:
  - intrasynth/src/Intra/Synth/AdditiveSampler.cpp
  - intrasynth/src/Intra/Synth/AdditiveSampler.h
related_tasks:
  - 20260820-PianoAttackTimbreFix
  - 20260823-HammerDebugProvenance
---

# Task

Two items from the hammer analysis review (external ChatGPT pass over the
synth-wip worklogs), plus the requested binary-size analysis of the h3-h6
addition:

1. **h3-h6 attack glow** — the measured 10-30/30-100 ms deficit of our h3-h6
   vs the SF2 samples was addressed with a short-lived absolute-amplitude
   overlay of h3..h6 at the attack.
2. **Short-note release** — very early NoteOff immediately damped the string
   AND cut the already-excited cabinet/transient energy (h1/h2 push, bloom),
   so short notes became "bitten-off" synthetic sounds. A minimum 35 ms
   free-attack window before applying the damper fixes this without stretching
   longer notes.
3. **Binary size analysis** — quantify what the h3-h6 glow adds to the
   assembled WASM.

## Outcome (2026-08-27, user A/B)

- **The glow overlay is REMOVED.** User listening pass: "starting from B4 up a
  strange sound appeared; below B4 nothing changed at all."
- **The 35 ms free-attack release window is KEPT** (independent fix, verified).

## Why the glow failed (measured)

Region selection is nearest-root, so MIDI 71 (B4) maps to region 72 (C5)
which carries the strongest glow table — exactly the boundary the user heard
("strange from B4 up"; A4 and below are in regions 69/60 where the table is
−38…−45 dB, i.e. effectively off — "nothing below").

Render diff (current vs no-glow, same note, 10-30 ms window):

| key | glow added | note |
|---|---|---|
| 60/66/69 (no glow / ~off) | ~0 (−45 dB) | inaudible |
| 71 (B4) | +5.5 dB | audible buzz |
| 72 (C5) | +5.5 dB | cur overshoots FL ref by +3 dB |
| 74-76 (D5-F5) | +2.5 dB | overshoot ~3-5 dB |
| 84 (C6) | +3.8 dB | overshoot |
| 96 (C7) | +1.2 dB | h3-only |

Structural root cause: the glow adds a FIXED h3-h6 tone at an absolute level
relative to the FINAL H1 amplitude, but our H1 is still ramping through its
attack (per-partial atk) in the first ~10-30 ms. The added tone therefore
lands ~20-30 dB above the momentary H1 and dominates the attack — a foreign
buzzy layer, not a hammer. The reference's attack brightness comes from its
fast-rising partials, so the correct fix for any remaining h3-h6 deficit is
attack-envelope shaping of the partials themselves, not an additive layer
(see Deferred).

## What remains in the code

`AdditiveSampler::NoteRelease()` defers the damper for notes younger than
35 ms (`mReleasePending`/`mReleaseAt`, checked in RenderInto); the former
release body is `ApplyRelease()`. Only notes released before 35 ms are
affected — longer notes behave exactly as before (verified bit-identical
below).

## Binary size analysis (glow, before removal)

| variant | bytes |
|---|---|
| 2bf8770 baseline | 168 178 |
| no-glow (release window only) | 167 682 |
| glow + release window (removed) | 170 978 |
| **current (release window only)** | **168 288** |

- h3-h6 glow cost was +3 296 B (+2.0%) vs no-glow — not significant.
- Release window alone is +110 B vs baseline.
- Builds are deterministic (same source → same size/hash across build dirs).

## Verification (current build)

- Held notes (60/66/72/75/84/96, no NoteOff in window): **bit-identical to the
  2bf8770 baseline** — the glow removal restored the accepted attack exactly.
- Short notes (NoteOff at ~10 ms): differ from baseline (release window
  defers the damper); 10 ms note peaks identically to a 52 ms note
  (0.12930) — the attack plays out instead of being cut mid-strike.
- `scripts/smoke-intrasynth.js` and `scripts/smoke-test-wasm.mjs` PASSED
  (0 NaN/Inf).
- `web/generated/` == `dist/` byte-identical
  (sha256 6f5751d2…, 168 288 B).

## Session 2026-08-27 (afternoon): short notes 100-300 ms vs FluidSynth

User: the 35 ms window changed nothing by ear; the offending notes are
100-300 ms. Before presenting any result, compare against FluidSynth (SF2
render, reverb/chorus off) and make it match better than before. The two
commits were also removed from git+GitHub per request (branch back at
2bf8770, working tree kept).

### Test harness fixes (why earlier numbers were garbage)

- The fluidsynth short-note MIDI used `TICKS_PER_S = 2*4800` but the file
  division is 480 tpq (500000 us/qn), so the "80-400 ms" notes were actually
  0.8-4.0 s. Fixed `scripts/_tmp-flshort.js`, re-rendered all 30 refs.
- Live-event renders sent NoteOff only at 4096-sample pull boundaries
  (~93 ms) — up to 66 ms late for a 120 ms note. Switched to MIDI-file
  renders (sample-accurate events, `_SourceCreateFromMidiFileData`).

### Measured vs FluidSynth (Titanic SF2, program 0, velocity 100)

Grid: keys 60/72/75/76/84/96 × durations 80/120/180/250/400 ms, onset-aligned,
per-20 ms normalized envelopes + per-harmonic Goertzel windows.

1. **Bloom was NOT gated at NoteOff.** `mBloomBuf` is mixed into body/push
   overlays without `*mOverlayGain`, while body and push fade with the 8 ms
   release tau. After release the region-75 h2/h3 bloom kept playing its own
   envelope → D#5/E5 post-release RMS +5…+12 dB vs FL. Fix: multiply bloom by
   `mOverlayGain` in both overlay loops (attack + sustain). Held notes
   unchanged (`mOverlayGain` stays 1.0 until release). A/B on key 75/120 ms:
   post-release now decays 4-5 dB faster, pre-release identical.
2. **Release (damper) decay was 1.3-2.3× slower than FL** on every key.
   FL RMS τ after NoteOff: 100 ms (C4), 58 ms (C5), 54 ms (D#5), 40 ms (C6);
   ours: 156/92/104/51 ms. Root cause: `tauR = 0.28s*sqrt(261.6/f)`.
   Retuned to `tauR = 0.13s*(261.6/f)^0.65 / sqrt(k)` (√k spread kept).
   After: release τ = 91/56/57/55/32/43 ms vs FL 100/58/54/59/40/61
   (ratios 0.8-1.1). Natural (held) decay untouched (0.8-1.2× FL already).
3. **Region 75 early sustain** (D#5/E5, 0-300 ms): FL render decays fast
   (h1 −10.9→−23.2 dB re ref by 0.4-0.8 s), ours holds ~+4…+10 dB hotter;
   our h2 with bloom reads +10…+12 dB above FL in 0.1-0.4 s. Note: region 75
   tables were calibrated against the RAW SF2 sample (h2/h1 +0.7 dB at
   0.15-0.45 s per 20260825 worklog), while FL's render (its SF2 envelope on
   top) shows h2/h1 −12.6 dB in the same window — so this residual is
   reference ambiguity (sample vs render), NOT a confirmed bug. Left as-is;
   needs an explicit user decision (see Deferred).

### Verification (current build, sha256 423dfa4e, 168 288 B)

- Held notes (60/72/75/76/84/96): bit-identical to 2bf8770 baseline until
  release; diffs appear only in the post-release tail (first at 4.10 s for
  C4), i.e. only the intended damper change. Key 96 fully identical.
- Post-release envelopes now track FL within ±3-5 dB on all keys;
  pre-release C5 (72) matches FL within ±4 dB through the release.
- `scripts/smoke-intrasynth.js` and `scripts/smoke-test-wasm.mjs` PASSED
  (0 NaN/Inf). `dist/` == `web/generated/` (423dfa4e…).

## Session 2026-08-27 (evening): short low notes (D3) attack, FL render vs sample

User: short LOW notes (e.g. D3) sound less rich than FluidSynth; release is
fine, the attack is not — for a short note only the attack is heard, and our
low notes don't "sound like a string". Also asked whether FL adds an envelope
beyond release (should a held render equal the sample?).

### FL render vs raw sample (held, h2/h1 and h3/h1 at 0.1-0.3 s)

FL's render is NOT exactly the sample: differences in h3 balance are ±2…13 dB
and flip direction per key — e.g. D#5 (75) ≈ sample, C5 (72) FL h3 −12 dB vs
sample, C7 (96) FL h3 −14 dB vs sample, D3 (38) FL h3 +3 dB vs sample.
Most likely FL's velocity curve/filter per zone, not an amp envelope (the
early RMS tracks the sample within ±2-3 dB on most keys). So the earlier
"sample vs render ambiguity" is real and key-specific.

### D3 (key 38) diagnosis

Absolute per-harmonic attack (ref = FL h1 at 0.1-0.3 s):

| window | FL h1/h2/h3 | ours (before) | sample h1/h2/h3 |
|---|---|---|---|
| 10-30 ms | 5.6/13.0/10.3 | 1.4/10.8/7.1 | 12.4/28.7/19.1 |
| 30-100 ms | 2.4/12.0/12.3 | 0.8/9.1/7.2 | 17.4/29.1/26.2 |
| 100-300 ms | 0.0/6.5/8.8 | −0.6/6.9/4.4 | 19.6/26.0/25.5 |

- Our h3 was ~4-5 dB below FL in the body (30-300 ms); h2 fine. Note: our
  table matched the SAMPLE (h3/h1 6.0) within 1 dB — the gap was FL's render
  being +3 dB above the sample, i.e. the user's reference.
- Onset: ours is instant (−11 dB re peak at t=0), FL −25 (slow roll-in),
  sample −14.5. Ours sits between; left as-is (the sample says we are closer
  than FL; the "richness" comes from the h2/h3 head, not the quieter onset).

### Fix (region 38, D3)

- PianoRegions.h region 38, k=3: Amp 4429 → 6200 (+2.9 dB), comment added.
- After: 100-300 ms h3 = 6.9 vs FL 8.8 (gap 1.9 dB, was 4.4); 10-30 ms gap
  1.0 dB (was 3.2). h2/h1 untouched. Only region 38 changed.
- Release/damper/gate untouched by this edit; both smoke tests PASSED;
  dist == web/generated (a9b16c20, 168 374 B).

### D3 attack-body rebalance (same session, second pass)

User: the missing thing is not in the 3-10 kHz band — a LOW strike is
missing, "probably not describable by harmonics at all".

Measured (FFT band RMS, 0-50/50-150 ms, normalized to own total):

| band (D3, f0=73.4 Hz) | FL | ours (before) | sample |
|---|---|---|---|
| 0-0.6f0 (0-44 Hz, below f0) | −35.0 | −32.1 | −31.7 |
| 0.6-1.4f0 (h1) | −12.5 | −17.2 | −11.9 |
| 1.4-3f0 (h2-h3) | −3.3 | −7.2 | −2.5 |
| 8-20f0 (h8-h20) | −8.8 | **−1.9** | −7.9 |

- **No sub-fundamental thump exists in ANY render** (FL −35 dB too) — the
  non-harmonic low-strike hypothesis is rejected; the reference's "low body"
  IS the h1-h3 head.
- Our D3 attack+body was top-heavy: 587-1468 Hz band ~7 dB hot, h1-h3 bands
  4-5 dB quiet. Present in both attack and sustain → a partial-table issue,
  not the attack buffer.
- Per-harmonic check (100-300 ms, Goertzel with **stretch scan** −2%..+3% —
  the exact-k scan had misread h14/h15, which are stretched +0.4%): the hot
  partials are h9-h13, 3.7-6.3 dB above the sample (h10 +6.3, h13 +5.0,
  h9 +4.9, h11 +4.3, h12 +3.7; h14/h15 fine once stretch is accounted).
- Fix: region 38 Amp h9 1898→1080, h10 8502→4100, h11 3078→1880,
  h12 1706→1110, h13 11591→6500 (scaled by measured delta).
- After: bands within ~1-4 dB of FL/sample everywhere (8-20f0 −4.1 vs −8.8;
  h1 band −14.8 vs −12.5). Because the constructor normalizes the per-period
  peak sum, cutting h9-h13 also raised h1/h2/h3 in the mix (same overall
  level, stronger fundamental) — exactly the desired "more body".
- Side effects: key 43 unchanged (probe ±2 dB); D3 short note tracks FL ±2 dB
  from 10 ms through release; smokes PASSED; dist == web/generated
  (e5752970). Note: FL render h6 is +6 dB above the sample at D3 (FL
  artifact) — we keep matching the sample there.

## Hammer noise layer — attack broadband noise for low keys (2026-08-27)

User: "Меня смущают короткие низкие ноты вроде D3… видимо атака не
соответствует… У fluidsynth эта нота звучит богаче за счёт оформленного
удара". Instruction: compare against the **raw sample**, not the FL render.

### Diagnosis (measured, D3 key 38)

- D3 partials end at k=40 → 2.94 kHz, so 3–10 kHz of our render is empty:
  deficit vs sample 3–6 kHz **−14 dB**, 6–10 kHz **−78 dB** (noise floor).
  C4: −10 dB in 3–6 kHz (partials reach 8.4 kHz there).
- The sample's 1.5–10 kHz attack spectrum is **noise** (peak/mean ≈ 2.3, not
  harmonic comb) — hammer contact + string friction, not overtones. So the
  fix is a noise layer, not extending the partial table.
- Sample band profile (0–100 ms, rel body 0–600 Hz): D3 1.5–3k −9.4,
  3–6k −24.0, 6–10k −34.6; C4 3–6k −17.9, 6–10k −23.2. 6–10k ≈ −12 rel
  3–6k at D3.

### Implementation (AdditiveSampler, fourth overlay mHammerBuf)

- **White noise: splitmix32** bit-mixing. First attempt used LCG hash bits
  `(i*2654435761u + 0x9e3779b9u)>>8 & 0xffff` — NOT white: the bits advance
  by 14096 mod 65536 per step → a linear modulo-ramp with period 4096
  samples → a harmonic comb, not noise (measured +12 dB tilt at 6–10k vs
  0–600 Hz). This made the first build's spectrum land in 6–10 kHz only.
- Shape: HP 1.5 kHz 2-pole + LP 2.2 kHz 4-pole (measured in situ): body
  <0.6 kHz −21 dB (invisible), 1.5–3 kHz minimal (partials cover it),
  6–10 kHz ≈ −10 dB rel 3–6 kHz (sample −12).
- Envelope: 2 ms rise, τ=0.15 s decay (sample 3–6k drops −3.6 dB in
  0–46 ms, −1.3 in 46–92 ms).
- Level: normalized to atkRms (attack body RMS, 0–46 ms) × hGain;
  hGain = 20.8 for midi ≤ 60, linear to 0 at C6, × impactV. Calibration:
  3–6 kHz = −24 dB rel body@150 ms at D3 — **±0.6 dB vs sample**.
- Release: gated by mOverlayGain like body/push (dies with the 8 ms gate).

### User A/B #1: "оно пшикает! У FL такого и близко нет"

Calibrating to the **raw sample** (−24 dB in 3–6k) made the attack hiss.
Recalibrated to the **FL render** (what the user actually hears):

- Correct measurements (see FFT bug below) — FL D3 render, rel body@150 ms:
  3–6k **−31.4** dB, 6–10k −48.0 (the raw sample has 3–6k at −24.0 — FL's
  render is 7 dB quieter in HF, effectively lowpassed).
- Fix: hGain 20.8 → **9.5** (−6.8 dB), τ 0.15 → **0.12 s** (FL's 3–6k decay
  −3.8/−4.0 dB per 46 ms window; τ=0.12 gives −3.3).
- After: D3 3–6k −30.4/−34.0/−36.2 vs FL −31.4/−35.2/−39.2 (within 3 dB
  in every window), 6–10k −49.3 vs −48.0. D#5/C6/C4 still +0.0..+0.1 dB.

### FFT bug in scratch probes (invalidates earlier numbers)

Two scratch FFTs (`probe-fl-vs-sample-fft.js`, `/tmp/dbg-attack.js` and
others written by hand) were **missing the butterfly line**
`re[i+k+len/2] = ur - vr; im[i+k+len/2] = ui - vi;` — their spectra were
lowpassed garbage (Parseval ratio 228 instead of n/2). This briefly made the
sample look like it had 3–6k at −4.6 dB rel body and FL at −6.2, implying
FL≠sample. The correct FFT (in probe-attack-noise.js) shows FL≈sample in
h1-h3 and the sample at −24.0 in 3–6k. Rule: verify Parseval on any new FFT
probe (probe's ratio 1024.011 ✓).

### Verification (final, hGain 9.5, τ 0.12)

- D3 attack band levels (rel body@150ms, vs FL render): 3–6k −30.4 vs −31.4
  (+1.0), 6–10k −49.3 vs −48.0 (−1.3), 1.5–3k unchanged (partials dominate,
  −11.1 vs FL −16.2 — pre-existing partial-table issue, not the hammer).
- High/mid keys untouched: D#5 +0.0 dB, C6 +0.0 dB, C4 +0.1 dB (A/B vs
  baseline 2bf8770, absolute band powers).
- Release still correct on D3: note-off at 180 ms → smooth decay, τ ≈ 0.29 s
  (damper formula 0.13·(261.6/f)^0.65). Note: probe-env.js has the same
  480-tpq bug that _tmp-flshort.js had (off = on + dur·960 ticks → 2×
  duration) — its post-release columns are unreliable; probe-d3-release.js
  uses correct math.
- Smoke tests PASSED; dist == web/generated (sha256 1ebc5533, 169 550 B).
  Baseline 2bf8770: 168 178 B → +1 372 B for the hammer layer.

### Remaining gaps (next sessions)

- C4/C5 mid-band partial deficits (h12–h32 too quiet vs sample/FL, 3–6k −9 dB
  at C4) — partial-table issue, not noise; the hammer only closes a couple dB
  there.
- D3 1.5–3k is +5 dB hot vs FL (partials h21–40) — pre-existing, separate
  from the hammer; candidate for a partial-table pass.
- FL's D3 sustain has ongoing tonal 3–6k (harmonics h41+) at −40..−44 dB
  through 400 ms — our modal bank stops at h40, so after the hammer ends
  (~0.55 s) the sustain is empty above 2.9 kHz. Real fix = extend low-region
  partial tables (perf cost), not noise.

## 2026-08-27 — attack roll-in («раскат»): низкая струна больше не «бьёт» с 0 мс

### Why
User: «у FL звук ноты D3 двойной — струна + низкий, чёткий молоточек, не шум!»
The hammer *noise* layer (prev. section) was not the missing component. The real
signature is the ATTACK SHAPE: FL/sample start near-silent and roll in over
~15-30 ms, ours hit −11 dB at t=0. Measured (band probe, 5 ms windows, D3):

| window | FL 0-300 Hz | SM 0-300 Hz | ours (before) | ours (after) |
|---|---|---|---|---|
| 0-5 ms | −32.6 | −41.9 | −10.4 | −21.6 (delay+ramp) |
| 5-15 ms | −15.4 | −32.2 | −12.9 | −18.2 |
| 15-46 ms | +0.3 | +1.5 | +0.7 | −0.6 |

After: 5-15/15-46 ms within ~3 dB of FL, attack rolls in from a 4 ms quiet head
instead of an instant clack.

### Implementation (`mSwellGain`/`mSwellStep`/`mSwellDelay`)
Per-voice output envelope at the sink: gain = 0 for the first 4 ms (the
sample's quiet head), then exponential approach `gain += (1-gain)·step` with
step set so −6 dB lands at T = 12 ms·clamp((500−f)/250, 0, 1): D3 ~12 ms,
C4 ~11.4 ms, C5+ → step=1 (delay skipped — instant, unchanged). Applied ONLY
to the string (per-partial amp), the thump overlays (body + hammer) fire at
full level through their own 4 ms head — swell over the whole voice was
«вата» (fluffy): string rolls in, hammer stays a sharp clack.

- Head: D3 starts −57 dB (4 ms) → thump lands ~5 ms at full level → string
  body by ~12 ms (T=12 calibration: FL reaches −6 dB at 8 ms, we 6 ms;
  C4: FL 7 ms, we 8 ms — both within 1-2 ms of FL).
- C5/C6/D#5: bit-for-bit unchanged behavior (step=1 path, no delay + body/
  hammer heads skipped when swell inactive).
- Hammer/body levels preserved; 0-46 ms A/B vs baseline: D#5/C6 +0.0 dB,
  C4 −2.3 dB (intended string ramp).
- Smoke PASSED; dist == web/generated (wasm 2659b75d, 169 646 B).

## 2026-08-27 (late): attack redesign «тук → шум» (user: «пшикает», потом «вата»,
«низкий чёткий молоточек, не шум»)

User ear: FL D3 is «струна + низкий чёткий молоточек, не шум»; the noise, if
any, appears AFTER the knock («глухой удар распадается на шум»), not as
filtered noise over time. Coarse band probe (rel body 0-600@150ms) confirms
FL's structure: 0-5 ms quiet, 5-15 ms LOW knock (0-300 −15.4, 3-6k still
−40.9), 15-46 ms noise arrives (−31.8), 46-92 ms decays (−35.4). Our old
build fired the noise WITH the knock (2 ms rise after 4 ms head) and its
LP 2.2k tail hung on — the «пшик», and the noise masked the knock.

### Final design «тишина → БАМ → шум» (v3, after «вата так и осталась»)

Full-band envelope measurement settled it: FL = ~8 ms quiet, then the sound
arrives in 2-4 ms (−12.3@8ms → −1.7@10ms); the commit (2bf8770) hits at full
within 2 ms (no knock contrast); my v2 (head + 12 ms swell + 15 ms noise
rise) faded everything in over 15-20 ms — that IS the «вата». FL's attack is
a JUMP, not a ramp.

- String: 5 ms silence, then LINEAR ramp full in 2 ms (mSwellGain += step,
  step = 1/(0.002 s·sr); replaced the 12 ms exponential).
- Body + hammer: 5 ms head, then at full — the whole sound arrives as one
  «тук» at ~5-8 ms.
- Hammer noise: single dark layer, HP 3k 2p + LP 4k 6-pole (6-10k −15 rel
  3-6k; FL −17.7; the bright two-layer idea was dropped — its 6-10k is
  inaudible but reads as hiss), rise 12 ms (peak ~17 ms — AFTER the БАМ),
  decay τ=0.08, hGain 5.0 (3-6k −31.6 at 15-46 ms vs FL −31.8).
- Result: 0-46 ms attack window on C4 is now −0.1 dB vs baseline (commit-identical —
  no вата), D3 full-band BAM lands ~6 ms after note-on, noise tail after.

### Measured (D3, band probe; our windows shifted ~5 ms by the head)
| window | FL 3-6k | our | FL 6-10k | our |
|---|---|---|---|---|
| 0-5 ms | −44.8 | −35.5 | −69.8 | −60.5 |
| 5-15 ms | −40.9 | −32.7 | −66.6 | −56.9 |
| 15-46 ms | −31.8 | −31.6 | −49.5 | −46.7 |
| 46-92 ms | −35.4 | −33.1 | −57.6 | −50.5 |

3-6k matched within ~3 dB everywhere; 6-10k hot but < −55 dB rel body
(inaudible). 1.5-3k noise is masked by the string (noise −26 vs string
−11 rel body) — the 1.5-3k hotness (−10.9 vs FL −16.1) is the D3 partial
table (h11-h20), pre-existing, not the noise.

- D#5/C6 unchanged (+0.0 dB, head/swell/noise gated by freq < 500 Hz);
  C4 attack window −0.1 dB vs baseline (commit-identical — no вата).
- Found+fixed: my JS filter replica was fine; the earlier «flat spectrum»
  result was an FFT window-overrun bug (out[w0+i] undefined → NaN).
- Smoke PASSED; dist == web/generated (wasm f42edea5, 169 646 B).

### Tooling note
str_replace/write_file on AdditiveSampler.cpp got stuck on a stale content
cache mid-session (every oldString "not found" while disk was fine); worked
around by write_file to a new path + `mv` over the original. Parseval-check
the FFT probes (see earlier entry).

## Deferred

- Real h3-h6 attack deficit fix: per-partial attack-envelope shaping
  (partials should reach their sustain level faster at the attack, matching
  the sample's fast-rising partials), not an additive absolute-level tone.
  Requires a proper per-harmonic tracker session.
- Region 75 (D#5/E5) early-sustain decay: whether to chase the FL *render*
  trajectory (faster early decay + reduced bloom h2) or keep matching the raw
  sample. User decision needed — region 75 has a history of comb-filter and
  "double timbre" traps; a wrong move re-opens those.
- Other low keys with smaller h2/h3 gaps vs FL render (43/47 ~+2 dB, 54 h2
  +7 dB) — sweep after the user confirms the D3 fix by ear.

## 2026-08-27 (final): hammer-noise experiment REVERTED — user verdict

User after the «БАМ» build: «Сейчас звук (например, D3) звучит как последний
коммит, не лучше, только после F4 пшикание добавилось». Verdict after 6
iterations: the noise-based hammer never provided an audible improvement at
low keys (D3 −31.6 dB rel body is masked/inert) and only added hiss where
the 3-6k band already has real partials (F4-C6, hGain fade 5.0→0). The
swells/heads were either inaudible or «вата».

Reverted to 2bf8770 (git checkout on AdditiveSampler.cpp/.h + PianoRegions.h,
including the D3 h3/h9-13 partial rebalance — user hears no difference, so
nothing was kept), and re-applied ONLY the free-attack-window fix
(mReleasePending/mReleaseAt, 35 ms, previously user-approved «можно
оставить»): ApplyRelease() extraction + NoteRelease pending logic + render
check. A/B vs baseline: bit-identical attack on D#5/C6/C4 (+0.0 dB).
Smoke PASSED; dist == web/generated (wasm f9a933ef).

Conclusion: the missing «hammer» on short low notes is NOT an additive
noise/transient layer (all attempts hissed or were inaudible). Remaining
candidate: per-partial attack-envelope shaping (partials reaching sustain
level faster at attack) or partial-table work — see Deferred.

---

## Velocity layers + per-key loudness check (user question)

SF2 «Titanic 200 GM-GS» Clavinova Grand (preset bank 0/prog 0 → inst 182
«Clavinova P6»): one sample per key group (L/R), NO velocity layering — the
only velocity split in the whole preset is at the extremes (vel 0-1 and
101-127 switch to adjacent-sample groups; vel 2-100 always the same sample).
So velocity richness in FL comes from fluidsynth's vel→amplitude + vel→filter
curves applied to the same sample, not from sample switching.

Our synth: `volume = exp(vel/127 − 1) × CC7/127` (exponential, web-midisynth
style). Same shape as FL's default vel curve, so no mismatch there.

Per-key body loudness (RMS @ 100-150 ms after onset, vel 100, re key 60):
FL is systematically louder in the low-mid (keys 25-52, avg **−2.7 dB** vs
ours) and upper-mid (72-84, **−2.2 dB**); 54-69 matches (~±1 dB). Peak-level
check (10 ms window, first 200 ms) confirms it's a real volume-curve
difference, not decay-rate artifact: FL has a mid-high boost (72: +3.5 dB,
84: +3 dB over us at peak) that our Loudness table lacks. Top end (87-96)
messy because of fast decay (peak: FL C6 ≈ us, FL C7 −3 dB under).

So «высокие заглушаются низкими» is a real balance difference: our per-region
Loudness curve is flatter than FL's (which boosts the melody range). Fix
candidate: reshape per-region loudness toward FL's curve — see Deferred.

Velocity→filter correction: checked the SF2 modulators — the ONLY cutoff mod
is note→cutoff (0x102, key tracking), no velocity→cutoff anywhere (imod/pmod
scan). So FL's velocity character is vel→amplitude only (same sample, no
filter sweep). Nothing to port for filter; our exp(vel/127−1) already matches.

---

## Restored: release retune that was accidentally reverted (user report)

The «release should decay faster» fix was in the working tree during the
hammer experiments, then lost in the `git checkout 2bf8770` revert (only the
35 ms window was re-applied). Restored exactly per the verification section:

1. **Bloom now gated by `mOverlayGain`** at BOTH overlay mix sites (attack +
   sustain) — after NoteOff the region-75 h2/h3 bloom dies with the 8 ms gate
   instead of ringing its own envelope (was D#5/E5 post-release +5…+12 dB vs
   FL).
2. **Damper retune**: `tauR = 0.28·√(f_note/f_C4)` → `0.13·(f_note/f_C4)^0.65`
   — release τ now 91/56/57/55/32/43 ms vs FL 100/58/54/59/40/61
   (was 156/92/104/51 ms, 1.3-2.3× slower).

Verification: wasm hash **423dfa4e — byte-identical to the previously
verified build**; held notes bit-identical to 2bf8770 baseline until note-off
(first diff exactly at release: 550 ms for a 1000 ms note, 139.6 ms for
180 ms), post-release decays 4-5 dB faster on key 75. Smoke PASSED,
dist == web/generated.

---

## Per-key loudness curve matched to FL (user: «исправляй громкость»)

Measured body loudness (RMS @ 100-150 ms after onset, vel 100, re key 60)
vs FL renders: low-mid 25-52 was −2.7 dB, mid 54-69 ≈0, upper-mid 72-84
−2.2 dB. Top 87-96 unreliable (fast decay → window noise), left unchanged.

Fixes in `PianoRegions.h` `Loudness` (linear whole-voice gain):

- Regions 25..51 (keys 25-52): ×1.365 (+2.7 dB)
- Regions 72/78/81/84 (C5, F5, A5, C6): ×1.66 (+4.4 dB), second pass on
  the 72-84 group after measuring residual
- Region 75 (D#5-E5, keys 74-76): **reverted to original** — FL dips there
  like the raw sample (−9.6…−10.8 dB re C4); boosting overshot by +5-6 dB
- All regions: ×0.776 (−2.2 dB) global pull-back to restore headroom
  (worst-case 12-note chord vel 127: +0.6 dBFS clipping → −0.8 dBFS clean;
  baseline was −1.6 dBFS)

Result vs FL (body, re C4): 25-52 avg **+0.0 dB**, 54-69 **−0.6 dB**,
72-84 **+0.8 dB** (previously −2.2…−2.7 dB); C5/B5/C6 within ±1 dB of FL,
D#5-E5 dip preserved. Release τ unchanged (C4 ~111 ms vs FL 100, C6 ~39 vs
40 — the restored release fix is intact). Smoke PASSED; dist == web/generated
(wasm 47c8556a, 168 309 B). Not committed.

## 27 авг — атака низких нот (< C4): «тишина → удар» вогнутой рампой

Проблема (жалоба пользователя): короткие низкие ноты (D3) звучат «не струной»,
вата в атаке; шумовой молоток (6 итераций) был тупиком и закрыт. Перепроверка
по данным показала: «тук» у FL/семпла — это НЕ шум, а сама форма нарастания.

Померено сырьём SF2 (семпл `51(L)` для D3/ключ 50, origPitch 60) и FL-рендером
per-period полной полосы (реф = тело 100-300 мс):
- семпл D3: P0 **−18.9**, P1 **−6.6**, P2 **−2.8** дБ (набирает к 2-му периоду)
- наш базис: P0 **−1.8** (бьёт сразу, без контраста) — вот чего не хватало

Реализация (`AdditiveSampler`):
1. **Глобальная огибающая атаки** `mAttackEnv` для нот < C4: 3 мс тишины →
   линейный подъём к 1 за ~12 мс (вогнутая «тишина → удар»). Применяется на
   выходе блока струны (main convolution sink), P0P1 газатся. NOT экспонента
   (та звучала как «вата»): линейная рампа после паузы даёт перкуссионный тук.
2. **Per-partial приглушение** h1-h3 для нот < C4: `atkSeed` (0.02/0.05/0.10)
   + mAtk задержка на 5 мс (`mAtkStartSamples`), затем дорастание τ=7 мс/k.
3. Корпусная воронка **откачена к базису** (в пределах контакта) — общая
   тишина→удар теперь от mAttackEnv, растяжение корпуса на 20 мс душило тук.
4. `mAtkFull` — новый массив «настоящего» шага; mAtk может держать 0 в окне
   задержки, переключение на границе за O(count) (как сегменты затухания).

Результат D3 (P0..P3 per-period, дБ к телу):
- семпл: −18.9 −6.6 −2.8 −2.1
- мы:    **−21.4 −7.2 −0.9** −0.3  (P0 чуть тише, P1-P2 совпадают)

Первое отличие A/B от базиса — ровно на онсете 52 мс (это и есть атака);
удержанная часть бит-в-бит, D#5/C6 (>= C4) нетронуты. Smoke PASSED,
dist == web/generated. Not committed.

## 27 авг (позже) — «тишина → удар» не сработала, откат

Попытка 27 авг (см. выше «Глобальная огибающая атаки») ДАТАМИ сказала, что
семпл/FL набирают низ по периодам (P0 −19/−7/−3 дБ), и я реализовал вогнутую
рампу «3 мс тишины → 12 мс подъёма» + per-partial приглушение h1-h3.

**Прослушивание (пользователь): «Никакого замаха и тука, атака БОЛЕЕ ВАТНАЯ,
чем в принятом варианте».**

Причина найдена объективным замером (peak/1 мс, D3):
- базис: 0 мс −41, 1 мс −24, 2 мс −21 — резкий удар без контраста;
- вариант: 0 мс −40, 1 мс −180 (тишина), 2 мс −54, 3 мс −47... — первый 1 мс
  блинк, потом плавное всплытие.

Рампа ЗАГЛУШАЛА настоящий физический транзиент (буфер атаки ~2 мс целиком
попадал в нулевое окно), оставался только медленный подъём — ровно «вата».
FL/семпл набирают АБСОЛЮТНЫЙ уровень плавно, но НАШ синтезатор построен как
резкий контакт — резать его рождает вату.

**Решение: откат.** Вернул AdditiveSampler .cpp/.h атаку к принятому базису:
- убрал глобальную огибающую mAttackEnv и mAtkEnvLen;
- убрал mAtkStartSamples/mAtkStarted/mAtkFull и per-partial приглушение atkSeed
  (0.02/0.05/0.10, mAtk=0 в окне) — mAtk снова единый `τ=min(AttackT/k,0.6 мс)`;
- вернул корпусную воронку (0.5·contactN) и seed струны `gSeam`.

Проверка (A/B vs baseline, D3, ±0.5 дБ во всех 2 мс-окнах атаки):
- атака бит-в-бит как принятый вариант (единственное отличие ~0.94× — это
  принятая ранее калибровка громкости, а не атака);
- body RMS 0.053/0.050, всё в пределах. Smoke PASSED, dist == web/generated.

**Вывод:** направление «искусственная тишина → рамп» для низких нот —
тупик, оно читается как вата. Исходная жалоба на «не струну» у коротких
низких нот остаётся, но теперь есть точный инструмент: атака принятого
варианта объективно бьёт резко, и менять её тайминг — регрессия. Точечная
работа — вкус (громкость/спектр УДАРА, корпус), не тайминг.
