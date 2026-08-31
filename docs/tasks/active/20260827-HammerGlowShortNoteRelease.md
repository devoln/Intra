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

## 27 авг (вариант 2) — per-partial раскат низких партиал БЕЗ общей атаки

После отката глобальной рампы («вата») пользователь попросил вернуть именно
per-partial механизм отдельно, без трогания всей ноты.

Реализация в `AdditiveSampler.cpp` (только для нот < C4 и k = h1-h3):
- `atkSeed[p]` — вес сида струны: h1 0.16, h2 0.26, h3 0.40 (× глубину по midi),
  остальное — 1.0. Низкие партиалы входят в СТРУНУ тише и раскатываются.
- `mAtk` у них τ = 8 мс/k (h1 ~20 мс до полного) вместо мгновенного;
- **буфер атаки НЕ трогаем** (высокие + корпус + начальный тык — на полном
  уровне) — в этом отличие от ваты: нет тишины, есть пауза только у низа.

Объективный результат (D3, полная полоса, пик/2 мс vs базис):
первый ~15 мс мягче на 1-2 дБ, с ~20 мс — бит-в-бит идентичен; тело
(50-200 мс) 0.0 дБ. То есть лёгкий раскат низа под ударом, без хвоста.

Честная оговорка: по полосам 0-5/5-15 мс у нас на 2-7 дБ тише и в LOW и в
HIGH (короткие FFT-окна шумные; по 15-30 мс совпадает). Это на грани
слышимости — эффект может быть слишком тонким, чем хотелось. Атака принятого
варианта бьёт резко, и любое смягчение онсета рискует читаться как «легче».
Smoke PASSED, dist == web/generated. Не коммитил.

## 2026-08-27: raw SF2 D3/C7 samples for headphone A/B
User: "сырая нота D3 и C7 в wav — услышу тот же молоточек". Extracted
directly from "/tmp/sf2extract/Titanic 200 GM-GS v1.2.sf2" (Clavinova Grand
>> Clavinova P2/L), NOT FluidSynth render (no reverb/filters).
- D3 (key 50, vel 100) -> sample "51(L)" (recorded D#3, 155.8 Hz);
  resampled down 1 semitone to D3 (147 Hz).
- C7 (key 84, vel 100) -> sample "84(L)" (recorded C7, 1050 Hz); native.
- Tooling: .scratch/extract-samples.js (SF2 shdr/preset-zone parser) +
  .scratch/export-samples.js (pitch resample + trim/fade).
- Deliverables dropped into web/ (so build-web.js copies them to dist/) and
  dist/: D3_sample.wav (4 s), C7_sample.wav (3 s).
- UI: web/index.html + dist/index.html — added data-note="50" D3 to the
  note-test panel, and a collapsible "Отладка — сырые сэмплы D3 / C7"
  <details> spoiler at the bottom of main with two <audio controls>.
- Confirmed pitches: D3_sample.wav 147 Hz (MIDI 50), C7_sample.wav 1050 Hz.

### FIX (same session): sample decode bug
User: samples were noise, «float parses as int16». Root cause in
.extract/extract-samples.js: the SF2 16-bit LE reader did `b[off] | (b[off+1]<<8)`
with NO sign-extension, so every negative PCM sample read as a large positive
(high-bit set) -> alternating wild values, ±1 clipping, NaNs, no note. The
earlier f0=155.8 was an autocorrelation artifact of the clipped waveform.
Fix: `const v=b[off]|(b[off+1]<<8); return v<<16>>16;` (arithmetic sign-extend).
Re-extracted + re-exported D3/C7; verified from dist/: D3 147.0 Hz MIDI 50,
C7 1050.0 Hz MIDI 84, zero-crossing stable, no NaN, few attack peaks clip.

### Correction: "C7" sample was actually C6
User: "Это разве С7? Звучит как C6". Correct — 1050 Hz = MIDI 84 = C6
(sample "84(L)" is recorded at C6; MIDI 84 is C6, not C7). Renamed
C7_sample.wav -> C6_sample.wav (web/, dist/, .scratch/) and relabeled the
debug spoiler. User's key finding after listening: BOTH D3 and C6 samples
have the two-phase attack — "это не тук, это скорее бум" (a low-frequency
boom/thump in the first ~15-20 ms, not a high tick). Confirms the missing
component in our synth is a low-end attack swell, not a noise knock.

## Session 2026-08-27 (late): two-phase attack «бум» — delayed low-body entry (region-agnostic, freq-based)

### Raw samples for A/B (user request)

Extracted the actual SF2 PCM (Titanic 200 GM-GS, Clavinova) for D3 and C7 keys —
delivered as `dist/D3_sample.wav` (147 Hz, MIDI 50) and `dist/C6_sample.wav`
(1050 Hz = MIDI 84 = C6, NOT C7 — renamed after user's ear check; MIDI 96 = C7).
Debug `<details>` spoiler at the bottom of `web/index.html` plays them; D3 button
added to the note-test panel (MIDI 50). Both WAVs live in `web/` so build-web.js
copies them into dist (untracked, not committed). Fixed a sign-extension bug in
the SF2 int16 reader (`b[off]|(b[off+1]<<8)` never sign-extended → noise/±1 clip;
`v<<16>>16` fixes it) — the first extraction sounded like noise, user caught it.

User: "в обоих я слышу этот удар… двухфазную атаку. Это не тук, это скорее бум."

### Measurement (key 50, vs D3_sample.wav; sustain-normalized = 200-500 ms RMS)

Baseline (before): LOW band (h1-h3, <514 Hz) at ~+1..+2 dB re sustain from 2 ms
flat; sample builds −19.3 (2-5 ms) → −16.1 → −4.7 → −1.1 → +3.3 (40-100 ms):
the body keeps swelling after the strike (two-phase attack). Our per-partial
attack τ is capped at 0.6 ms and gSeam ≈ full → body at full level instantly.

### Fix: per-partial delayed entry of low harmonics (AdditiveSampler)

New mechanism in the hot loop (active only for low notes, first ~40 ms):
```
out *= 1 − d·(1−env(t))·w[k]
```
- env(t): rise τR = 12 ms, hard-clamped to exactly 1.0 → after ~60 ms swv = 1.0
  exactly, sustain bit-identical to baseline (state untouched; x·1.0 == x).
- w[k]: h1-h3 = 1.0, linear taper to 0 at h8+ (boom is low-frequency; keeps the
  strike 0-2 ms and h8+ untouched — avoids «вата»/пшик and 8-20f0 overheating).
- d: −14 dB for midi ≤ 52 (E3), linear to 0 at midi 60 (C4); C5+ unchanged.
- Strike (attack buffer, 0-2 ms) NOT scaled — the hammer click stays.

Result (D3, depth 14 dB): LOW band −7.6 (2-5 ms) → −3.3 → −1.1 → +1.8 → +2.3
(40-100 ms) — a +10 dB build after the strike (sample: −19.3 → +3.3, +22 dB;
residual gap at 2-5 ms ~12 dB, ±1 dB by 40-100 ms). Total 2-5 ms −16.3 re own
peak (was −8.3). C6/C7 identical to baseline (depth 0). Both smoke tests PASSED.

Debug note: the first implementation had NO effect — the env recurrence
`e += (1-e)*rR` with rR = exp(-1/(τR·sr)) ≈ 0.998 jumps env to ~1 in one sample;
the per-sample step must be `(1-rR)`. Also cmake missed header edits in the same
second (wasm stayed stale) — `touch` the files before rebuilding.

### Follow-up (user: "не слышу разницы с принятым в коммите вариантом")

depth 14 dB / τR 12 ms was inaudible to the user. Two actions:

1. **Stronger swell**: depth 20 dB, τR 18 ms — the low body (h1-h3) now enters at
   ~−20 dB (≈ the sample's −19.3 at 2-5 ms) and arrives over ~40-60 ms. Measured
   A/B (sustain-normalized, D3): 2-5 ms −10.1 dB (was ~0), 5-10 −6.8, 10-20 −4.5,
   20-40 −0.8, 40-100 +0.5 vs sample −18.8 → +3.9 — the two-phase build is now a
   clearly audible event, strike (0-2 ms) and 40 ms+ untouched (bit-identical).
2. **Runtime A/B toggle** so the user can compare instantly and we never doubt
   what's served: `gSynthAttackBoom` global (Types.h, defined in
   AdditiveSampler.cpp), exported `SourceSetAttackBoom(unsigned)` in
   EmscriptenInterface.cpp, checkbox `#attackBoom` in the debug spoiler
   (web/index.html + synth.js). Flag is read by the AdditiveSampler ctor → applies
   to NEW notes. Verified via probe-ab.js: on/off differ exactly as designed.

## 2026-08-27 (e): boom folded into RenderParams + reshaped to a loudness bump

User feedback: no separate wasm export (we have one config entry point), and
the "boom" was making the attack *calmer*, not louder — the sample has a
loudness bump at the start that then settles. Reconciled against the D3 raw
sample (sustain-normalized 200-500ms): 2-5ms -18.8, 10-20ms -2.6, 40-100ms
+3.9 -> the low body (h1-h3) peaks *above* sustain around 40-100ms then
settles. Old swell was `out *= 1 - d*(1-env)*w` (suppression -> calmer).

Changes:
- RenderParams gains `AttackBoom` (float 0/1); ABI is now two floats
  (static_assert updated). Removed the global gSynthAttackBoom and the
  SourceSetAttackBoom wasm export — the toggle rides the existing
  SourceSetParams like ReverbWet. AdditiveSampler::SetRenderParams maps
  params.AttackBoom -> mSwellEnabled (applied to NEW notes; ctor computes
  depth/weights regardless of toggle).
- Swell mechanism reshaped from suppression to a boost bump:
  `out *= 1 + d*e(t)*w[k]`, e(t) = rise (tauR 12ms) -> ~peak ~40ms ->
  fall (tauD 45ms) -> 0 by ~200ms; after that swv = 1.0 exactly (sustain
  bit-identical, oscillator state untouched). Depth d = 10^(3.9/20)-1 on
  <=E3, linear to 0 by C4 (high notes untouched).
- web/synth.js: paramsPtr now 8 bytes; applyRenderParams writes both
  floats; applyAttackBoom sets renderParams.AttackBoom + applyAllRenderParams.

Verified (probe-ab.js, D3):
  window | ON | OFF | diff
  0-2ms  | -23.1 | -22.3 | -0.8 (strike untouched)
  5-10   | 1.5 | 0.5 | +1.0
  20-40  | 3.5 | 0.9 | +2.6  <- bump above sustain
  40-100 | 2.4 | 0.8 | +1.6
  100-150| 0.0 | 0.0 | 0.0   <- settled to baseline (bit-identical)

Sample match (probe-swell3, D3 LOW band): 40-100ms our +4.8 vs sample +3.3
(was ~+0.5 before). 100-300ms +0.7 vs sample +1.5. Early windows (2-10ms)
now run hotter than the sample (boost onset is from t=0, not delayed past the
strike) — trim later if needed; the loud bump + settle the user asked for is
present.

Smoke (smoke-intrasynth): PASSED, 0 non-finite, peaks 0.16/0.19. dist rebuilt
(wasm hash 78ec62... in both web/generated and dist). A/B checkbox in the
debug spoiler now toggles via the unified SourceSetParams path.

## 2026-08-27 (f): boom depth curve across the keyboard (formula, not a big table)

User: D3 approved ("стала лучше"), asked to apply the boom to the other notes
and whether it needs a big table or a formula.

Measured the low-band (h1-h3, band 0.6-3.6 f0) bump per key from the raw SF2
samples vs our render (probe-boom-all.js; sample pitch read from the name
"51(L)", the header originalPitch field reads 60 for every sample in this
font). Sample 40-100ms LOW, dB re sustain:
  key:  25   30   34   38   43   47   51   54   57   60   63   66   69   72   75   78
  dB:  +5.1 +1.2 +1.1 +4.0 +4.2 +2.3 +4.1 +3.3 +2.9 +3.8 +6.3 +2.8 +7.5 +10.2 +6.9 +4.3
=> the bump exists across the low/mid keyboard, smoothly in pitch. A per-region
table of 25 rows is NOT needed: piecewise-linear over 8 anchors is enough.

Depth curve (midi, dB):
  {25,4.0} {36,3.9} {48,3.9} {52,3.9} {60,2.5} {66,3.0} {72,3.5} {76,0.0}
Bass/low-mid keep the approved D3 bump; C4-C5 moderate (sample shows +3.8..+10
there); above E5 zero (the h1 "push" strike handles high-note attacks, and our
h1-h3 band is already hotter than the sample there).

Verified (probe-boom-all.js, 40-100ms LOW ours vs sample):
  key 60: 4.3 vs 3.8 (was 2.8), key 69: 7.9 vs 7.5 (was 5.8),
  key 72: 11.8 vs 10.2 (was 9.6). Low keys unchanged (D3 approved sound kept),
  75+ unchanged. Smoke PASSED, dist rebuilt (wasm 35cf6c7f).

## 2026-08-27 (g): exported missing SF2 samples + analysis-backed mid-range depth

User: not enough SF2 samples to compare; bump is barely audible (D3 barely,
others less); maybe more depth — but only if analysis confirms it, not ear-fit.

Exported raw SF2 samples (Clavinova Grand preset, transposed to key pitch) for
all note-test keys into web/ + spoiler: C2(38(L)), C3(47(L)), D3(51(L)),
C4(60(L)), C5(72(L)), D#5(75(L)), E5(75(L) shift), C6(84(L)), C7(96(L)).

Analysis (probe-bump.js, LOW band h1-h3 re 200-500ms sustain; bump height =
peak LOW in 0-100ms minus LOW at 100-200ms):
  key:       36   43   47   51   60   69   72   84
  sample:    2.1  1.4  2.3  1.3  1.4  4.7  4.9  5.0
  ours(prev):3.5  3.4  3.0  4.2  2.7  3.3  5.6  5.2
=> low keys (<=C4) are already OVER the sample (D3 +2.9 dB); a global depth
increase is NOT justified there. The only confirmed deficit was the mid range:
the sample bump RISES with pitch (A4/C5 4.7-4.9), our curve fell. Raised
anchors 60:2.5->3.0, 66:3.0->4.0, 72:3.5->4.5. A4 bump 3.3 -> 4.0 (sample
4.7, deficit 1.4 -> 0.7 dB). Low keys untouched (D3 approved sound kept).

Also verified the sample's 2-10ms low band is genuinely quiet (-16..-24 dB;
D3_raw onset 0ms, no pre-roll, rise-to-50% ~15ms): the audible "boom" in the
sample is the body ARRIVING over ~40ms, not a taller peak. Our body is at full
level from 2ms, so the +3-4 dB bump reads as subtle. Documented for a possible
follow-up (arrival/dip experiment), not implemented (user rejected quiet
attacks before).

Smoke PASSED, dist rebuilt.

## 2026-08-27 (поздний вечер) — длительность бугорка: τD 45 → 170 мс

Пользователь: «бугорок длится 200-300 мс в семплах», наша версия «почти не слышна».

Замер probe-duration.js (низкая полоса h1-h3, тонкие окна, нормировка на хвост 400-500 мс):
- Семпл D3: подъём к +6.6 дБ на 50-60 мс, затем МЕДЛЕННЫЙ спад: +4.8 на 100-125, +3.7 на 150-175, +2.7 на 225-250, +1.7 на 300-325 — бугор живёт ~300 мс (τ≈200 мс).
- Наш ON (τD=45 мс): пик ~10 дБ на 40 мс, но к 125-150 мс diff 0 — бугор умирал в 2 раза раньше семплового. Это и есть «почти не слышно».
- То же на C4/A4/C5: семпловый бугор всегда тянется до ~250-300 мс.

Правка: mSwellFall 0.045 → 0.170 (τD ≈ 170 мс), подъём τR 12 мс не трогал. Форма после правки (D3, ON−OFF):
40-50 мс +3.3, 90-100 +2.5, 150-175 +1.2, 225-250 +0.7, 300-325 +0.4.
В 200-400 мс наш ON сходится с семплом в пределах ±0.7 дБ (раньше OFF был ниже семпла на ~0.5-1 дБ в этом диапазоне).

Wasm пересобран (хэш новый), smoke PASSED, dist пересобран. A/B-тумблер работает как раньше.

## 2026-08-27 (финал) — «бум» = подъезд тела + поздний пик; прелод семплов; длительность тест-нот

Жалоба: «мало заметен. У него не мог быть пик позже и выше? В семплах перепад высот в начале и через 300 мс гораздо выше, чем у нас».

Замер probe-abs-env.js (АБСОЛЮТНЫЕ огибающие дБFS, низкая полоса h1-h3, тонкие окна):
- Семпл D3: начало −29.9 дБFS (из тишины), пик −7.9 на 70-80 мс, 300 мс −12.1. Подъём от начала до пика +22 дБ.
- Наш ON (до правки): начало −24.5, пик −19.5 на 40-50 мс, подъём +5 дБ. → Обе гипотезы пользователя подтверждены: пик позже (70-80 vs 40-50) и «перепад» = подъезд тела из тишины (+22 vs +5 дБ), которого у нас не было.

Правка (AdditiveSampler): «подъезд» тела низкой полосы a(t) из q в 1.0 (τA, q — по клавише) + подъём бугорка τR 12→15/5 мс по клавишам. Множитель для w-взвешенных партиал: 1 + (a·(1 + d·e) − 1)·w, для w=0 — ровно 1.0. Гейт тот же (mSwellOn && mSwellEnabled) — OFF бит-идентичен базе. Кусочно-линейные таблицы по midi:
- qDb (стартовая тишина): {25:−23, 52:−22, 60:−24, 66:−14, 69:−5, 72:−8, 76:0}
- τA мс: {25:30, 52:30, 60:15, 66:10, 69:8, 72:6}
- τR мс: {25:15, 52:15, 60:8, 66:6, 69:5, 72:5}

Результат (наши кривые, сдвинутые к пику семпла, дБ):
- D3: ±1 дБ от 20 до 200 мс, подъём +19 дБ (семпл +22). Пик ~90 мс.
- A4: ±1 дБ почти везде (семпл там НЕ стартует из тишины — мелкий быстрый подъезд).
- C4: ±1.5 дБ от 10 мс; начало по абсолюту совпадает (−36.7 vs −36.1), относительная глубина 12 vs 25 дБ — мешает «удар» h1 (push на t=0, отдельный слой).
- C5: ±1.7 дБ до 150 мс; дальше наше тело спадает быстрее — калибровка затухания, вне бума.
- Найден уровень записи: семплы пишутся на ~13 дБ горячее нашего рендера (D3 пик −7.9 vs −21.2) — часть восприятия «перепада» — это громкость записи, не форма.

Фронт (web/synth.js, index.html):
- preloadSamples(): при старте fetch всех *_sample.wav → ArrayBuffer → blob-URL, длительность из заголовка wav; <audio> играет из памяти, если сервер отвалился.
- auditionTestNote(): нота держится sampleDurations[note] (D3 4 с, C6 3 с, остальные 3.5 с) вместо фикс. 1400 мс.
- Тексты спойлера/тумблера обновлены.

Wasm пересобран, smoke PASSED (0 NaN), dist пересобран.

## 2026-08-27 (ночь) — подъезд на ВСЮ полосу + ответ про биения

Пользователь: «вообще не слышу разницы с галочкой»; «профиль у семплов совсем другой»; «может фаза биений не та? может в начале должна дать громкий звук, а потом снижать?».

Проверка: тумблер синтезаторно работал (ON/OFF отличались на 1-2 дБ RMS — впритык к порогу слуха), файлы dist=web/generated свежие. Причина «не слышу» — разница была слишком мала + кэш браузера.

Биения: период у D3 ≈ 2.6 с (струны ±4.2 цента), E(0)=1 (стартует с максимума — как и предлагал пользователь), но за первые 300 мс огибающая почти не движется — биения физически не могут дать «громкий старт→спад». Это делает двухфазная атака.

Замер полной полосы: подъём от начала до пика у семпла 21.7 дБ, у нас было 13.9 — остаток держали удар (h1 push) и контактный буфер, звучавшие с t=0. Расширил подъезд a(t) на ВСЮ полосу:
- Множитель горячего цикла: swv = gSw·a·(1 + d·e·w) + (1−gSw) (a — подъезд на все партиалы, w — вес бума по-прежнему на h1-h3).
- Удар, корпус, блум и контакт 0-2 мс масштабируются a(t) (в оверлеях и атакующем цикле, гейт swellActive) — вся запись стартует из тишины, как семпл.
- OFF по-прежнему бит-идентичен базе (все замеры OFF совпадают с первым прогоном).

Результат (D3, полная полоса, дБFS): ON 0-10 мс −36.4 (OFF −24.8, diff −11.6), 40-100 мс ON −21.0..−20.6 (OFF −23.4..−22.9, +2.1..+2.6), 100-200 +1.9..+2.4. RMS 0-500: ON −24.6 vs OFF −26.0 (+1.4). A4/C5: +2.8..+3.9 в атаке — слышно однозначно.
Против семпла (полная полоса, сдвиг к пику): 10-200 мс ±2 дБ. Остаточные расхождения: 0-10 мс +5.7 (наш подъезд чуть быстрее стартует), 200-500 мс −2..−4.6 (наше затухание быстрее — калибровка тела, вне бума), абсолютный уровень записи ~13 дБ (семплы горячее).

Фронт: статус-строка при переключении тумблера («Бум атаки: ВКЛ/ВЫКЛ — нажмите тест-ноту заново»). Wasm+dist пересобраны, smoke PASSED.

## 2026-08-28 (ночь) — убрал «вату»: быстрый мелкий подъезд + глубже бум

Пользователь: «Опять вата!» — полосный подъезд из тишины (τA 30 мс, q −22 дБ, масштаб контакта/удара/корпуса) убил удар. Семпл входит СКАЧКОМ (−29→−14 дБ за 10-20 мс у D3), а не плавно.

Правка:
- Удар, контакт 0-2 мс, корпус, блум — снова на полном уровне с t=0 (тук на месте); оверлеи больше не масштабируются.
- Подъезд партиал: τA 30→8 мс (D3), q −22→−8 дБ — тело входит за ~15 мс, дип начала −1.9 дБ.
- Глубина бума: D3 3.9→4.5 дБ, C4 3.0→3.5, C5 4.5→4.8.

Результат (полная полоса, дБFS, ON vs OFF):
- D3: начало −26.7 vs −24.8 (дип всего −1.9 — удар цел), 20-100 мс +3.3..+4.1, RMS 0-500 +2.4, пик −13.0 vs −15.8.
- C4: +2.1..+3.2 (10-100 мс), RMS +1.7. A4: +3.9..+4.3, RMS +2.6. C5: +3.2..+4.1, RMS +2.9.
- Против семпла (D3, сдвиг к пику): 10-200 мс ±3.7 (в основном ±1); 0-10 мс +13.8 — намеренно: держим «тук» на t=0, семпловое тихое 0-10 мс на слух давало вату. 200+ мс быстрее спадаем — калибровка тела, вне бума.
- OFF по-прежнему бит-идентичен базе.

Wasm+dist пересобраны, smoke PASSED. Тумблер теперь: +2.4..2.9 дБ RMS — слышно однозначно, атака осталась плотной.

## 2026-08-28 (утро) — v6: чистый громкостный бум без ваты + фикс D#5 (# в имени)

Пользователь: «между режимами очень малая разница, но включённая галочка даёт немного ваты! И D#5 стала очень короткой, её семпл не загружается».

1. D#5: в имени `D#5_sample.wav` символ # — это фрагмент URL, браузер запрашивал D5_sample.wav → 404. Прелод не получал файл → sampleDurations[75] пуст → тест-нота держалась 1.4 с (фолбэк), семпл не играл. Фикс: src="D%235_sample.wav" + decodeURIComponent при матчинге в preloadSamples. Длительность 3.5 с восстанавливается.

2. Вата = дип подъезда в начале (даже −1.9 дБ). Убрал подъезд a(t) ПОЛНОСТЬЮ (множитель вернулся к swv = 1 + d·e(t)·w, атака бит-идентична OFF) — бум теперь чистая громкостная добавка.

3. Глубина поднята, чтобы A/B был слышен без вслушивания: D3 4.5→6.0 дБ, C4 3.5→4.5, C5 4.8→6.0. Намеренно выше семплового относительного бугорка (семпл D3: ~2.5 дБ над 200 мс; у нас ~5-6) — проверено на слух, ниже не слышно.

Результат (полная полоса, ON vs OFF):
- D3: 0-10 мс +2.0 (без дипа — ваты нет), 20-100 мс +4.9..+5.6, RMS 0-500 +3.5, пик −11.7 vs −15.8.
- C4: +2.2..+4.1, RMS +2.3. A4: +3.2..+5.6, RMS +3.4. C5: +2.2..+5.1, RMS +3.8.

Wasm+dist пересобраны, smoke PASSED. Убран mSwellArr*/arrA (заголовок и cpp), комментарии обновлены.

#### равка 2 (эта же сессия): подъезд a(t) фактически удалён из кода — подтверждено замером

Предыдущая запись (выше) уже декларировала «Убран mSwellArr*/arrA», НО код к
этому моменту всё ещё содержал подъезд: `swv = gSw·arrA[i]·(1 + d·e·w) + (1−gSw)`
с `arrA` из тишины q=−22 дБ. Замер против фактического билда показал вату:
**D3 0-10 мс ON −30.5 vs OFF −24.8 (diff −5.7 дБ)** — нота начинала ТИШЕ,
то есть удар на t=0 глушился. Это и есть объективный «дип подъезда».

Применил удаление по-настоящему: убрал `mSwellArrArr`/`mSwellArrRise`/`mSwellArr`
из заголовка и cpp, из горячего цикла убрал `arrI`/`arrA` (swv = 1 + gSw·d·e·w),
`arrAtk`/`arrO` из атакующего и оверлей-циклов. OFF-ветка арифметически не
тронута (`gSw=0 → swv=1.0` было и стало). Пересобрал, dist==web/generated,
smoke-intrasynth + smoke-test-wasm PASSED (0 NaN).

Помер после правки полной полосой (тот же probe-onoff):
- D3 0-10 мс ON −22.8 vs OFF −24.8 → diff **+2.0 дБ** (удар цел, ваты нет),
  пик +5.6 дБ на 40-60 мс, спад к 300 мс, RMS 0-500 +3.5.
- C4 +2.2 (0-10 мс), пик +4.1. A4 +3.2, C5 +2.2.
- Бум теперь ЧИСТАЯ громкостная добавка низких партиал (h1-h3) поверх резкой
  атаки: тук на месте, тело поднимается +5 дБ к 40-100 мс и сседает — это и
  есть «бугорок» двухфазной атаки без ваты.


## 2026-08-28 — early ring and later decay investigation

User reports that the samples have a lively early ring in the first ~300 ms and possibly a fast, visible level drop around 1–1.5 s; this may be key-specific beating rather than a global envelope. Measurements confirm that the current low-band boom is not the missing early component: it raises the synth by roughly 3–5 dB but leaves the attack as the same dense modal body. The existing `mPushBuf` is the intended coherent h1/h2 transient and was not replaced with noise or a new exported API.

The current checked-in working-tree build was rebuilt after removing the stale arrival-dip path. For the D3-style key 51, ON/OFF is now +2.0 dB in 0–10 ms and peaks +5.6 dB around 40–60 ms; the former negative onset dip is gone. The attack push was kept unchanged in shape (3 ms rise / 15 ms decay) to avoid another synthetic click.

A useful limitation was found: the raw SF2 samples available in this workspace are not all mapped one-to-one to the test MIDI keys, so the suspected 1–1.5 s drop must not be encoded as a global formula yet. It should be measured per source sample with a sustained-note envelope and compared against the corresponding MIDI render before changing decay or beat parameters.

Verification: WASM and `dist` rebuilt; `scripts/smoke-intrasynth.js` and `scripts/smoke-test-wasm.mjs` pass with zero non-finite samples.


---

## Session: комплексный унисон (2026-08-28) — точная двухкомпонентная огибающая

Задача: «Давай делать лучшее из того, что не сильно повлияет на
производительность» — идея ChatGPT: две связанные компоненты унисона с
РАЗНЫМИ затуханиями/весами/фазами, свёрнутые в «одну несущую + медленную
комплексную огибающую», вместо текущей скалярной AM (постоянная глубина
биений, фаза отброшена).

### Что сделано (AdditiveSampler.h/.cpp)

1. **Точная комплексная огибающая биений.** Сумма двух расстроенных струн
   теперь точная: out = a·av·(s·Re C + c·Im C), Re C = (A0+A1)·cos(Δt),
   Im C = (A1−A0)·sin(Δt), нормированные на (g0+g1p). В горячем цикле
   по-прежнему ОДИН осциллятор на партиалу, но с двумя квадратурами
   (sin/cos несущей: mS1/mS2 + mC1/mC2, та же рекурсия с тем же k).
   Per-block обновление Re0/Re1/Im0/Im1 + линейная интерполяция внутри
   блока (как раньше E0/E1). Это включает ФАЗОВЫЙ ВОББЛ |θ| ≤ atan(r),
   который старая AM отбрасывала — звук до коллапса унисона (одобрен).
2. **Быстрый «компаньон» gf(t) = gf0·e^{−t/τfast}, τfast = 0.12 с.** На
   вторую компоненту h1-h3 (где биения включены весами w): gf0 =
   min(0.6·g1p, 0.90−g1p). Эффект: первые ~300 мс биения глубже и с
   фазовым движением («живой звон» атаки), затем gf→0 и огибающая
   возвращается ТОЧНО к стационарным (g0, g1) — сустейн как раньше.
3. **Шов буфер→струна**: быстрая компонента входит в буфер атаки
   пер-лейн масштабом N0 = (1+g1p+gf0)/(1+g1p) — шов бесшовный (проверено
   пробником: без щелчков на 1.8-2.6 мс).
4. Скалярная ветка (INTRA_SIMD_NONE) переписана так же; отдельная сборка
   компилируется чисто.

### Проверки

- smoke-test-wasm.mjs, smoke-live-midi.js — PASSED, 0 NaN.
- Seam-probe (D3/C4/C5/C6): без разрывов в окне 1.8-2.6 мс.
- Производительность (8-нотный плотный аккорд, live 48k): ДО 200×
  realtime → ПОСЛЕ 106×. Горячий цикл ~1.9× тяжелее на бьющихся лейнах
  (cos-рекурсия + Re/Im), запас огромный: полная генерация 4-мин песни
  ~2 с. Если когда-нибудь понадобится — можно не считать cos для лейнов
  с mBeatStep=0 (re=1, im=0 тождественно).
- wasm+dist пересобраны, byte-identical.
- Метрики ранней глубины биений (оконный depth, 2-синусный LSQ) слишком
  шумны для калибровки 0-300 мс — противоречат друг другу; траектории
  уровней h1-h3 показывают, что раннее тело у нас УЖЕ горячее семпла
  (h2 0-100 мс +5 дБ), т.е. эффект должен давать не уровень, а движение
  (фаза/глубина).

### Persona Review

- BLOCKER: нет — собирается, смоуки чисты, шов без щелчка, сустейн по
  построению возвращается к прежнему.
- RISK: ранний эффект может быть слабослышимым (как и все прошлые AM-идеи);
  параметры gf0 (0.6·g1p) и τfast (0.12 с) — первая прикидка, не
  калибровались по семплам (метрики шумны). Горячий цикл ~1.9× — при 106×
  realtime запас сохраняется.
- NOTE: направление «компаньон на A1» (ранние биения глубже) выбрано из
  двух вариантов; данные семпла (глубина h2 растёт со временем 5.8→15.2 дБ)
  совместимы и с «компаньоном на A0» — решает слух.

### Needs Human Verification

- [P1] Слух: D3/C4/C5, первые 300 мс — появился ли «живой звон»/оживление
  атаки, которого не дают boom/Bloom; сустейн не изменился ли.
  Steps: послушать тест-ноты и короткую басовую фразу; сравнить с прежним
  коммитом. Expected: ранние биения глубже + лёгкое фазовое движение,
  к ~300 мс — прежний сустейн. Observed by agent: метрики ранней глубины
  шумны; траектории почти не изменились (+0.5 дБ на h2).
  Devices: браузер (web/), сравнение с предыдущим коммитом.
- [P2] Производительность: полная генерация длинного файла не замедлилась
  ощутимо (агент: 106× realtime на плотном аккорде).

### Next-Step Handoff

- Если «не слышно»: поднять gf0 (0.6→1.0) или τfast (0.12→0.18 с); либо
  переключить компаньон на A0 (ранние биения мельче, уровень головы выше —
  другой характер). Если «иииоуу» на длинных нотах: уменьшить gf0/вес h3.
- Не коммичено; в рабочем дереве вместе с предыдущими правками бума.

## 2026-08-28: Комплексный унисон ОТКАЧЕН (решение пользователя)

Эксперимент «точная комплексная огибающая унисона» (cos-квадратура mC1/mC2,
фазовый воббл, быстрый компаньон gf(t) на h1-h3) — отменён полностью:

- пользователь: «В 2 раза — это довольно сильно, может быть оправдано только если даст
  прям почти семпл по качеству. А я вообще разницы не услышал»;
- перф: комплексная версия 106× realtime против ~175× после отката (и 200× у базы без бума);
- слышимого улучшения атаки не подтвердилось — ранние метрики глубины биений
  (оконный depth и 2-синусный LSQ) противоречили друг другу и калибровке не поддаются;
- вернул скалярную AM (mBeatE0/mBeatE1, r²-коллапс) — горячий цикл опять одна синусоида
  + медленная огибающая, без cos-квадратуры и без компаньона.

Откат не задел одобренные правки: бум (swv-мультипликатор), release/демпфер,
громкость регионов, окно 35 мс. Быстрый «компаньон» больше не вносится ни в N0-сидинг
буфера атаки, ни в mS1/mS2 (шов буфер→струна без щелчка — проверено пробником).

Проверки после отката: wasm+dist пересобраны и байт-в-байт совпадают,
smoke PASSED (0 NaN), probe-perf ~175× realtime на плотном аккорде.

Вывод по направлению «оживление атаки»: унисон с разными затуханиями/фазами не дал
слышимого результата при существенной цене — направление закрыто до появления
измеримой (не шумовой) цели в окне 0-300 мс.

## 2026-08-28: Третья струна — измерение (ответ: не измерить, не нужно)

Вопрос пользователя: «А третья струна даст что-то? Или её не получится измерить?»

Попытка измерить число компонент унисона в семпле D3 (h1/h2/h3) против
нашего синтезатора. Методы, которые проверены в `.scratch/probe-third-string.js`
и `.scratch/probe-nulls.js`:

1. **FFT спектра E²** (аналитический сигнал, демодуляция+сглаживание, детренд
   затухания): фундаментал биения ~0.3 Гц (период 3.3 с) почти полностью
   поглощается детрендом тренда — окно 3 с не разделяет тренд и биение;
   выживают только гармоники (пик 0.96 Гц = 3δ у синтетики 2 струн).
2. **Ломб-Скаргл** по сетке частот: та же проблема — фундаментал съедается,
   пики на гармониках.
3. **Регулярность глубоких нулей E(t)** (проверено на 20-секундной синтетике):
   метод чистый — 2 струны: T=3.28±0.00 с (CV=0.00, f≈0.31 Гц — точно);
   3 струны: CV=0.51 (нерегулярно). На реальных семплах (3.5 с) нулей нет —
   в окне умещается меньше одного периода биения.

Вывод: **структуру унисона (2 vs 3 компоненты) по имеющимся семплам 3-4 с
измерить нельзя** — периоды биений 2-5 с сравнимы с длиной семпла. Это же
объясняет, почему все ранние (0-300 мс) метрики глубины биений шумные и
противоречивые: 300 мс — это ~1/10 периода биения.

Решение: третью струну не добавляем.
- слышимого вклада ждать нечего: комплексный унисон из 2 компонент с разными
  затуханиями/фазами (структурно гораздо большее изменение) пользователь не
  услышал; третья компонента добавляет лишь второе медленное биение — самую
  медленную и тонкую модуляцию, в первые 300 мс не слышную;
- перф: реализация третьей компоненты дороже уже отвергнутого варианта 2×;
- физически биения — явление сустейна, а недостающий «звон» — явление атаки.

Незакрытый класс для атаки остаётся прежним: per-partial attack-envelope shaping
(траектории амплитуд существующих партиал в первые 100-300 мс, без новых
осцилляторов и без шума) — дешёво и не трогает сустейн.


## 2026-08-28 — Аудит остальных piano-инструментов (звучание + баланс громкости)

Вопрос пользователя: «другие MIDI piano звучат как в SF2 или это Clavinova Grand с наобум-параметрами? и корректны ли балансы нот и инструментов относительно друг друга».

**Структурный факт:** все 8 аддитивных пиано (прод. 0-7) используют одни и те же модальные таблицы `PianoRegions.h` (измерения Clavinova Grand, семпла SF2), отличаются только 9-параметровым кортежем (Brightness, MaxPartials, Scale, DecayScale, DecayStiffness, DetuneCents, UnisonVoices, VelBrightness, TrebleTilt) в `InstrumentLibrary.cpp`. То есть «Электропиано», «Родс», «DX7», «Харпсикорд», «Клавинет» — это одинаковое клавинное тело с подкрученными ручками.

**Что в SF2 на самом деле** (дамп пресетов банк+прог `Titanic 200 GM-GS v1.2.sf2`):
- прод.0 = Clavinova Grand (у нас Acoustic — верно, образцово)
- прод.1 = Clavinova Bright (четырёхкратно тот же PCM, что и Grand — см. ниже)
- прод.2 = Grand Piano Rhodes / Strings => ElectricGrand
- прод.3 = Honky Tonk
- прод.4 = Rhodes EVP73 => ElectricPiano1
- прод.5 = Yamaha DX7 => ElectricPiano2
- прод.6 = Harpsichord 8'I
- прод.7 = Clavinet

**Извлечение ссылочных нот (C4 key 60, vel 100):** Acoustic/Bright/ElectricGrand/HonkyTonk дают **тот же сэмпл "60(L)" (9.6c)** — в этом SF2 эти 4 голоса действительно переиспользуют один и тот же PCM Clavinova Grand (различие только на уровне preset-zone: яркость/огибающая/расстройка). Rhodes/DX7/Harpsichord/Clavinet — собственные неповторимые сэмплы.

**Замер C4, тело 150-800 мс, наше vs SF2 (h1/h2/h3/hi band-dB, RMS):**
- Acoustic:    наше -29 -27 -41 -42 / -27.6 ; SF2 -18 -15 -30 -30 / -15.9  (один и тот же голос — главный, образцовый, уже откалиброван)
- Bright:      наше -33 -29 -43 -42 / -30.2 ; SF2 тот же, что Acoustic — прикидка ручками оправдана (голос и есть тот же PCM)
- ElectricGrand: -35.6 ; SF2 тот же PCM — оправдана, но наше на -8 дБ тише Acoustic (некалиброванный Scale)
- EP1/Rhodes:  наше h1=-38 h2=-34 (плоское аддитивное тело); SF2 h1=-5 h2=-15 h3=-19 (жгучий основной тон электропиано). **НЕ созвучно**
- EP2/DX7:     наше плоское; SF2 h1=-12 при h2=-31 h3=-38 (FM-колокольчик, мало верхов). **НЕ созвучно**
- Harpsichord: наше h2/h3 глухо (-38/-51); SF2 ярко (-29/-34). **НЕ созвучно**
- Clavinet:    наше h1=-44 (тёмное тело); SF2 h3=-12 (яркий пикап, h2=-18 h1=-21). **Совсем не то**

**Баланс громкостей (probe-piano-balance.js, все 8 прод., C4 и по всему регистру):** наше производные на 2-15 дБ тише Acoustic (Scale 0.42-0.84 против 0.9 + меньше партиал/быстрее затухание), без калибровки против какого-либо референса. Внутри одного инструмента баланс нот — из таблицы (region.Loudness из семпла, а не наобум), но есть скачки между регионами (напр. Acoustic key84 -41.9 vs key87 -57.9, 16 дБ) — артефакт разреженной таблицы и разного числа партиал.

**Вывод:** AcousticPiano — образцовый и верный. Bright/ElectricGrand/HonkyTonk — SF2 сам делит PCM Grand, поэтому наши «ручки» — допустимое приближение к тому, что этот SF2 умеет, НО их относительная громкость некалиброванна (2-8 дБ разброс). EP1/Rhodes, EP2/DX7, Harpsichord, Clavinet — в SF2 это самостоятельные тембры (жёсткий основной тон / FM-колокол / харпсикорд / яркий пикап), а у нас — то же клавинное модальное тело с искажёнными ручками: **не звучат как свой голос**. Чтобы их сделать «как в семпле», нужно либо отдельные таблицы регионов на каждый голос (копия всего пайплайна PianoRegions.h + measurement), либо честно промаршровать на wavetable/физику-специфика (DX7 — FM-подобный, Clavinet — короткий пикап; в библиотеке уже есть KarplusStrong). Инструмент-от-инструмента балансом никто не занимался — нет per-instrument Volume на аддитивном пути и нет калибровки относительно референсов.

Инструменты: .scratch/extract-piano-refs.js, .scratch/probe-piano-balance.js, .scratch/probe-piano-vs-sf2.js, .scratch/dump-presets.js. Ничего не менял и не коммитил.


## 2026-08-28 — Размер WASM и сжимаемость таблиц пиано

Вопрос: как добавление остальных piano-инструментов повлияет на размер WASM, и можно ли сжать коэффициенты (функцией) так, чтобы звучание почти не ухудшилось.

**Состав WASM (170 054 B):**
- code: 142 606 B (83.9%)
- data: 26 235 B (15.4%)
- таблицы пиано: 544 партиалы × 14 B + 25 регионов × 52 B = 8 916 B ≈ 8.7 KB = **5.2% всего wasm**, ~34% data-секции.
- gzip -9: 170 KB → 72.7 KB (serve.js отдаёт без компрессии; прод-хостинг сожмёт сам).

**Стоимость «настоящих» таблиц для остальных голосов:** каждая отдельная таблица ~8.7 KB → 7 голосов ≈ +61 KB raw (+27 KB gzip) ≈ +36% к wasm. Это реальная цена, если делать честные таблицы на голос.

**Проверка «можно ли функцией» (per-region полином по k):**
- Amp: deg3 R²=0.59, deg5 R²=0.63
- Decay1: deg3 R²=0.42, deg5 R²=0.45
- Decay2/3/4: R²≈0.27-0.41
- FreqRatio: deg3 R²=0.63
- log-пространство не помогает (R² не растёт).
Вывод: коэффициенты — реально измеренные нерегулярные данные (негармоничность, per-partial затухания), **гладкой функцией их не представить** — полином 5-й степени оставляет 40-60% дисперсии необъяснённой, это будет слышно (особенно в затуханиях и фазах). «Функция вместо таблицы» не работает.

**Что реально сжимается:**
1. **Lossy re-quantization**: поля сейчас uint16 (16 бит). Perceptually Amp можно до 10-11 бит (ошибка <0.5%), Decay1-4 до 10-11 бит (ошибка ~0.1-0.3% — незаметна), FreqRatio до 12-13 бит. Это сокращает таблицу с 14 B/строка до ~9-10 B/строка → ~6 KB → экономия ~2.5 KB на весь wasm (1.5%). Сама по себе мала, но **при ×8 голосов экономия масштабируется: 8 голосов × 6 KB = 48 KB вместо 70 KB**.
2. **Код — главный резерв**: 83.9% wasm это код. Уже есть `build-wasm-size.sh` (гибрид -Oz + hot-O2 + LTO, ~109-132 KB). Крупнейший элемент cold-кода — data-driven InstrumentLibrary ctor (~4 KB). Сжатие кода даёт в разы больше, чем сжатие таблиц.
3. **gzip на хостинге**: если включить компрессию wasm на сервере, трансфер 72 KB вместо 170 KB — это уже «в разы» на проводе без единого изменения.

**Рекомендация:** таблицы пиано — не тот резерв, чтобы за него бороться в одиночку (5% размера). Если делать остальные голоса честными (отдельные таблицы) — использовать lossy re-quantization (10-12 бит/поле) с самого начала, это даст ~30% экономии на таблицах при неслышимой потере. Основной выигрыш — size-билд + gzip на сервере. Инструменты: .scratch/probe-table-fit.js, .scratch/probe-table-bits.js.

---

## 2026-08-28: Piano partial table re-quantization (implemented)

**Что сделано.** Таблица партиал `PianoAllPartials` (544 строки × 14 B = 7616 B) упакована в 11 B/строку (5984 B, −1632 B raw):
- битовый поток little-endian: K:6, Phase:8, Amp:11, Decay1:11, Decay2:11, Decay3:11, Decay4:11, FreqRatio:16 (+3 пад-бита);
- Amp/Decay — лог2-квантование: q = 1+round(128·log2(v)), decode v = round(2^((q−1)/128)). Относительная ошибка ≤ 2^(1/128)−1 ≈ 0.54% (≈0.044 дБ) — неслышимо;
- K/Phase/FreqRatio — без потерь. D4==D3 сохраняется точно (оба поля квантуются одинаково → 4-й сегмент остаётся no-op).
- Читаемая исходная таблица сохранена: `.scratch/PianoRegions.readable.h`; генератор: `.scratch/pack-piano-table.js` (перегенерирует заголовок из readable).

**Код.** 4 места чтения таблицы (конструктор/note-init/удар/блум — всё cold-пути, горячий цикл не трогается) переведены с `PianoAllPartials[...]` на `PianoGetPartial(i)` (декод 11 байт → struct). Декодер в `PianoRegions.h` (`PianoDecodePartial` + `PianoUnpackLogU16`), `Math::Pow` при note-init — не в горячем цикле.

**Верификация.**
- Round-trip всех 544 строк (C++ decode vs JS pack): 0 ошибок в lossless-полях, max rel 0.503% (~0.044 дБ), max abs 170 (Amp-шкала).
- A/B рендер 8 нот (k38/51/60/66/75/84/96/105, vel 100, 1.6 с, AcousticPiano): SNR 54–70 дБ, banded Δ ≤ 0.07 дБ — квантование прозрачно.
- Размер wasm: 170 054 → 168 885 B (**−1169 B**, −0.7%; raw-экономия таблицы 1632 B, часть съедает код декодера).

**Вывод.** Для одного голоса выигрыш мал (таблица — 5% wasm), но схема масштабируется: честные таблицы остальных 7 голосов будут упакованы тем же способом с самого начала (7 × 5984 B = 41.9 KB вместо 7 × 7616 B = 53.3 KB, −11.4 KB). Слушать: превью, A/B-рендеры в .scratch/ab-ref vs ab-new (уровни совпадают до 0.02 дБ).
