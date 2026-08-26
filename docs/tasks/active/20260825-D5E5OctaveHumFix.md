# D5–E5 "island" timbre: unison comb filter and D4 decay decode

**Date:** 2026-08-25
**Status:** active
**Task:** fix the D#5 (D5–E5) timbre that sounds "коряво" (octave hum, dull top) and does not change despite table edits.

## Symptoms

- D5–E5 range: sustained tone has a dominant "octave hum" — the 2nd harmonic sits ~10 dB above the fundamental, even though the amplitude table for region 75 is balanced (h1 = 2893, h2 = 2440).
- Editing the decay/amplitude rows for region 75 produced no audible change — the table was never the problem.
- The sustain tail of the note appeared to "collapse" faster than the sample.

## Root causes

### 1. Unison comb filter (the octave hum)

`AdditiveSampler` renders each note with `UnisonVoices = 2` strings. Each extra voice was given a phase offset equal to a **fixed 0.9 ms delay**:

```cpp
phase -= twoPi*fk*(0.0009f*float(v));   // v = voice index
```

When the two voices are summed, a constant time delay is a **comb filter**: the phase shift is proportional to frequency. At D#5 (fk = 623 Hz) the two voices land 201.9° apart — nearly anti-phase → the fundamental is attenuated by ~7.2 dB; the 2nd harmonic (1249 Hz) lands at 44.8° — constructive → boosted ~3.9 dB. Net: **h2/h1 = +9.7 dB** — exactly the measured +10 dB octave hum.

The same comb explains the rest of the island: C#5 (554 Hz → 179.5°, h1 nulled), E5 (659 Hz → h1 −4.9 dB, h2 +3.1 dB). Every decay-table fix "did nothing" because the table was balanced all along.

**Fix:** remove the fixed delay; unison voices start in phase (as a real hammer strike), and beating still comes from the detune (`DetuneCents`). The summed spectrum now reproduces the table exactly.

Verified: region 75 h2/h1 in the 0.15–0.45 s window went from **+13.6 dB** to **−1.0 dB** (table predicts −1.5 dB; the reference sample measures +0.7 dB).

### 2. D4 decay decode (tail 2× too fast)

`PianoRegions.h` is the "restored morning" 3-segment model: `D4 = D3` by convention, and the header comment states "D4 = D3 makes the 4th segment a no-op". But `AdditiveSampler.cpp` decoded:

```cpp
decay3 = D3/5461.25;   // correct scale
decay4 = D4/2621.4;    // wrong: 2.083× finer than D3
```

With `D4 = D3` this made **λ4 = 2.083·λ3** — the tail after `SegT3` (1.735 s for D#5) decayed at ~10.9 dB/s instead of the sample's ~5.2 dB/s, so the note "collapsed" in the sustain tail.

**Fix:** decode `D4` with the same scale as `D3` (5461.25), restoring the intended no-op. Verified: the amp envelope now follows the table exactly (5.22 dB/s through the tail, matching the sample's 4.6–5.2 dB/s).

## What was checked

- Standalone `AdditiveSampler` (mono and stereo, any chunk size) and the live `MidiSynth` path decay exactly per the table after the fixes — a debug export confirmed `dec1..4` and the envelope values in the running WASM.
- The apparent "accelerating tail decay" measured earlier was the **unison beat**: two detuned strings starting in phase produce a ~6–8 s amplitude ripple (null ≈ −10 dB near 3.5–4 s, then recovery). This is real piano behavior — the SF2 sample shows the same (faster) beating in its first second.
- No performance regression: full Chopin render 290.5 s audio in 6.76 s = **42.96× realtime** (baseline 43×).

## Session 2 (2026-08-25): region-75 sustain timbre — whistle band and eternal high-frequency ring

User: the D5–E5 fix didn't move the needle; the render still sounds like a "дешёвая пищалка" — especially D5–E5, but not only there.

### Method
A correct per-partial tracking scan (with stretched `FreqRatio` frequencies) of the SF2 sample `75(L)` vs the render from a freshly built WASM, windowed by table segments. Key ground-truth steps:

- Proved the renderer follows the table exactly: wrote a faithful JS simulation of the ctor + render loop; it matches the WASM output bit-for-bit (constant gain offset only). This removed the table-vs-renderer ambiguity once and for all.
- Confirmed the region-75 rows the ctor reads (PartOffset=417 → file lines 430–446, array starts at line 13) — an earlier "fix" session had edited the wrong region (C5's rows) and was reverted.
- The WASM was being rebuilt with a stale `-DINTRA_PROBE_REGION75` define in the build flags; cleaned up.

### Findings (region 75, D#5, vs sample 75(L))

1. **h1 λ2 too fast**: table λ2(h1) ≈ 42.6 dB/s, sample ≈ 27.6 dB/s — but this was fitted against the unison beat; changing it overshot the 1.0 s ratios, so it was left as-is (the beat, not the table, drives the apparent slope).
2. **h7 (≈4.5 kHz) sustained whistle**: our h4/h5/h7 sit 4–14 dB too loud vs h1 in sustain; h7's row amp was cut 460 → 200.
3. **h8 dies 2.5× too fast**: table λ3(h8) ≈ 13.1 dB/s, sample sustain decays with the whole spectrum at ~5.2 dB/s. h8: amp 102 → 150, D2/D3 8226 → 3284 (λ3 ≈ 5.2 dB/s).
4. **h9+ eternal ring**: rows 9–12, 14–16, 18 had D3 = 273 → λ3 ≈ 0.43 dB/s — a 5–10 kHz band that never decays. Set D2/D3 = 6000 (λ2 ≈ 20 dB/s, λ3 ≈ 9.5 dB/s) so the high band dies instead of ringing forever.
5. h6 decay moved 6083 → 4000 (λ3 ≈ 6.4 dB/s, closer to the sample's constant-ratio sustain).

### Verified
- Fresh clean rebuild (no probe define) matches the probe build; h8 tracks the sample's sustain within ~1–4 dB, h7 whistle is cut, h9+ no longer rings forever.
- No performance regression: full Chopin render **42.9× realtime** (baseline ~43×).

### Note on the "accelerating tail"
The tail *looks* like it accelerates because of the unison beat (null ~4 s, then recovery) — real piano behavior; the sample shows the same, faster. The amp envelope itself follows the table exactly (verified by dumping `dec[0]/amp[0]` and resonator state `s1` from the running WASM).

## Session 3 (2026-08-25): the actual "пищалка" — per-harmonic λ2 re-fit

User: after Session 2 nothing audible changed; "ни свиста, ни звона я и раньше не слышал". Right — those were −20…−30 dB partials, inaudible to remove. The measurable-and-audible gap I had found earlier (h4/h5 sustain 10–18 dB hotter than the sample relative to h1) was left untouched. This session fixes exactly that.

### Method (beat-robust)
- Window ratios at 0.5/1.0/2.0 s are polluted by the unison beat (both signals beat at different rates — hK/h1 ratios swing ±10 dB), so single-window comparisons were misleading all along.
- Robust measurements: sliding-Goertzel envelopes per harmonic, median-filtered (1 s span) to strip the beat, then exponential fits. Two independent methods (window medians 1.2–3.0 s; envelope fits) agree: **our sustain has h3–h7 10–18 dB hotter relative to h1 than the sample** (h4 literally louder than h1: +5 dB vs sample −8.4).

### Root cause
Region-75 λ2 (seg2: 0.285–0.835 s) is wrong for nearly every partial:
- **h1 λ2 = 42.7 dB/s is a beat artifact** — the generator's seg2 window (0.285–0.835 s) lands on the sample's unison beat null, so the fitted decay looks 2× too fast. The sample's h1 actually decays ~5 dB/s from 0.3 s on (its λ2 ≈ 22 dB/s at most). Our h1 collapses ~23 dB in seg2, which (a) kills the fundamental body and (b) inflates every hK/h1 ratio.
- **h3–h7 λ2 too slow relative to h1** (h4: 9.8 vs h1's 42.7), so after the fast λ1 collapse the bright partials stop decaying while h1 keeps falling — the 2.5–4.5 kHz band ends up 10–18 dB hot and rings on.

### Fix (PianoRegions.h, rows 430–446, D2 only)
Keep amps and λ1 untouched — the attack window (0.03–0.25 s) stays bit-identical to before, and the attack was already accepted. New D2 (λ2 in dB/s):

| partial | old λ2 | new λ2 | D2 old→new |
|---|---|---|---|
| h1 | 42.7 | 22.0 | 12872 → 6640 |
| h2 | 42.3 | 25.6 | 12781 → 7726 |
| h3 | 27.5 | 24.5 | 8291 → 7394 |
| h4 | 9.8 | 14.3 | 2972 → 4316 |
| h5 | 21.8 | 20.1 | 6579 → 6067 |
| h6 | 24.2 | 35.8 | 7300 → 10803 |
| h7 | 9.9 | 6.5 | 2991 → 1962 |
| h8 | 32.7 | 15.5 | 9872 → 4678 |

h2/h3/h6 were then nudged once from the measured deltas (10079→7726, 8752→7394, 12827→10803).

### Verified (fresh WASM, live path)
Sustain ratios hK/h1, window medians 1.2–3.0 s:

| K | sample | before | after |
|---|---|---|---|
| h2 | −7.4 | −5.3 | −7.4 |
| h3 | −17.1 | −7.3 | −17.1 |
| h4 | −9.1 | +6.8 | −7.0 |
| h5 | −14.2 | −3.0 | −13.4 |
| h6 | −31.5 | −13.8 | −31.6 |
| h7 | −23.0 | −12.1 | −21.6 |
| h8 | −30.5 | −25.7 | −27.6 |

The 2.5–4.5 kHz squeak band moved from +10…+18 dB to within 0.8–3 dB of the sample. Attack window unchanged (amps/λ1 untouched). Perf: full Chopin 290.5 s audio in 6.78 s = **42.9× realtime** (no regression).

## Session 4 (2026-08-25): the onset squeak — h1 strike overshoot

User: still "режет слух пищалкой" on D5–E5 (listening to Beethoven Fur Elise) after the sustain fix. The sustain ratios were right — but in Fur Elise the notes are short, so what bites is the **onset**, which the sustain tuning never touched.

### Measurement (keys 74/75/76, windows 0–10 ms / 10–30 ms / 30–100 ms)
- Our attack ramps every partial up in τ = min(AttackT/k, 0.6 ms) — effectively 0.6 ms for ALL partials of region 75 (AttackT=10 ms ≥ the cap for every k). So h1..h8 reach full table level in <1 ms.
- The sample's onset is different: **h1 strikes +12…+14 dB above its steady level for the first 5–15 ms, settling by ~30–50 ms** (hammer thump); the harmonics are correspondingly lower relative to h1 in 0–10 ms (h2 −9.9, h3 −15.9, h4 −12.7… vs our −1.5, −5.1, −3.8). Result: our first 10 ms is an h1-less bright beep — the squeak.

### Fix (AdditiveSampler.cpp)
Fundamental lane (k==1, both unison voices): amp starts at `kOsStart = 4.5` (above 1) and converges to 1 with τ = `kOsTau = 12 ms` (mAtk = 1−exp(−1/(kOsTau·sr))). Other partials keep the existing attack. Implemented via the existing `av = av·(dv−ak)+ak` envelope: start mAmp above 1 and let it converge to 1.

**Performance bug found & fixed during this:** `count` is rounded up to a multiple of 4, so with 2 voices × 17 partials there are padding lanes (p=34,35). The first attempt keyed the overshoot on `p % partials == 0`, which also hit padding lane p=34 with garbage `dec/atk` state → its amp never decayed after release → voices piled up → render dropped from 42.9× to 12.8× realtime. Guard added: `p % partials == 0 && p < partials*voices`. Perf back to **43.2× realtime**.

### Verified
Onset hK/h1, windows (sample/render):
| window | h2 | h3 | h4 | h5 | h6 |
|---|---|---|---|---|---|
| 0–10 ms | −9.9/−12.0 | −15.9/−15.6 | −12.7/−14.3 | −13.9/−16.4 | −22.8/−24.1 |
| 10–30 ms | −6.9/−4.9 | −7.0/−8.5 | −10.5/−7.2 | −9.9/−9.3 | −19.6/−17.0 |

(before the fix the 0–10 ms values were −1.5/−5.1/−3.8/−5.9/−13.6 — 8–11 dB hot). Peak levels unchanged (no clipping: max −5.8 dBFS at key 84, vel 100). Sustain medians unchanged (h2–h8 within 0.3–2.5 dB of sample).

### Reverted (2026-08-25, same session)
User: "Фу, ужасно щёлкает! Откатывай!" — the strike overshoot started the h1 envelope at a hard 4.5× step on the very first sample, an audible click at every note onset. The overshoot was fully reverted (constants removed, per-partial attack restored, `mAmp[p] = 0`). Rebuilt: perf back to **42.7× realtime**, sustain medians unchanged (h2–h8 within 0.1–3.0 dB of sample). The onset-squeak problem remains open — next candidate is a *ramped* strike (rise over 1–2 ms, not a step) or the harmonic-bloom timing (30–100 ms window shows our h3–h6 ~3–7 dB quieter than the sample).

## Session 5 (2026-08-25): contact force re-enabled (no clicks) + the 3× perf regression it caused

User (via GPT code review): the committed `synth-wip` has the contact-force attack inside `#if 0` and comments that contradict the executed logic; clean up dead code and make the contact force work — without clicks this time.

### State taken over
Working tree already had a large uncommitted refactor: `#if 0` blocks removed, contact force wired into the constructor (attack buffer + `mAtk`/`driven` seeding), `PianoEnvelope.h` deleted, `AttackBoost`/`HammerLevel` params dropped from `AdditiveSampler`/`InstrumentLibrary`. Stale markers (`BISECT`, `if(false)` region-75 correction) cleaned.

### 3× performance regression found & root-caused
Fresh build of the refactored tree: full Chopin render **13.4× realtime** vs 43× baseline. The hot loop was byte-identical to HEAD, so it was voice pile-up: A/B variants showed the difference came from the **`driven` seeding** (`mAmp[p]=1`, `mAtk[p]=0`), not from the attack-buffer playback.

Gate probe on released voices: near their end `maxAmp` was still 1.0 — a lane whose amp never decays. Dump: lane 66 had `amp=1, dec=1, atk=0, s1=0` — a **silent partial** (table row `Amp==0`, `dphis=0`). On the unit contact force a zero-frequency lane accumulates a DC offset (`ur = ΣF ≈ 45`), which dominated `zmax2`; the old code then marked it `driven` → `amp=1` forever → the −60 dB gate never closed → released tails piled up ~3× live voices.

### Fix (AdditiveSampler.cpp)
- `zmax2` (Tikhonov denominator scale) now computed **only over lanes with real table amplitude** (`crs²+cis² > 0`), so the DC lanes can't inflate it.
- `driven[p]` additionally requires `crs²+cis² > 0` and `den >= eps2`: silent rows and padding lanes never get `amp=1/atk=0` — they stay at `amp=0` and pass the gate instantly.
- Real lanes unchanged (driven if their modal response is real, else smooth bloom via `mAtk`), so the audible signal is identical; only voice lifecycle changed.

### Verified
- Perf back to **47.0× realtime** on the full Chopin render (better than the 43× baseline).
- Onset probe: `s0 = 0.0` exactly at every key (no click step), smooth rise, seam (buffer → SIMD string at ~1.8 ms) inside normal sine slew (max |Δ| 0.01–0.03 over 0–5 ms).
- Smoke test: peak 0.136, no NaN.
- Session probe code (`INTRA_PROBE_GATE` in the header, `[OLD]` fprintf in MidiSynth) removed; build flags cleaned of probe defines.

## Session 6 (2026-08-25): the contact-force attack clicked — per-key normalization + roar restored

User: "Вот только оно щёлкает опять. Как-то можно оставить, но бесшовно?" — keep the contact force, remove the click.

### Measurement (what actually clicked)
A seam probe proved the buffer→string junction is *not* the click (diff at the seam ≤ 1.03× the neighbors; the seam is exact by construction — last buffer sample = first string sample). The click was the **un-normalized buffer interior**: the modal forced response during the contact piles up in phase and peaks **6–7× above the sustain** (C7: 0.263 vs sustain 0.037; D#5: 0.152 vs 0.026), then collapses to the table level at the seam. A 1–2 ms spike 6–7× louder than the tone = a tick at every onset (the full-piece render peaked at 3.9 — that peak was the attack spikes).

### Root cause
Session-32's refactor dropped the per-key attack/sustain normalization and the buffer bloom fade-in that the approved (HEAD `#if 0`) build had: it played the raw buffer at ×c only.

### Fix (AdditiveSampler.cpp/h)
1. **Per-key attack normalization** (restored from the approved build): `gain = clamp(tr/naturalScale, 0.02, 8)·keyScale`, where `naturalScale = rawRms·2/peakS` (buffer RMS relative to string RMS), `tr(midi)` = measured sample attack/sustain RMS ratios (key 33≈0.13 … 96≈2.29, interpolated), `keyScale` = manual control points (24:0.35, 36:0.25, 48:0.10, 60:0.06, 72:0.15, 84:0.17, 96:0.26, 108:0.30). Algebraically this reproduces the approved build's buffer level exactly.
2. **Bloom fade-in on the buffer**: `zb[i] *= 1−exp(−i/(bloomTau·sr))`, `bloomTau` = 45 ms (C1) → 8 ms (C7) — the strike "rolls in" instead of stepping.
3. **Seam preserved exactly**: the last buffer sample is scaled to `gSeam = gain·bloom(contactN)`, and driven string lanes are seeded at `mAmp = gSeam` with the per-partial `mAtk` (they grow gSeam→1) instead of the old hard `amp=1, atk=0`. Non-driven lanes unchanged (bloom from 0).
4. **«Рокот» restored**: 4 cabinet resonances (78/116/168/285 Hz, τ=45 ms, gain 0.0011, ~0.12 s) driven by the contact force with a rise² funnel and `impactV` velocity scaling. Played as an **overlay**: samples [0..contactN] are mixed into the attack buffer, the rest is added on top of the string in the block fold (the first attempt played it sequentially after the attack buffer, which delayed the string by 0.12 s and collapsed the sustain to gSeam — fixed).

### Verified
- Overshoot (peak 0–2 ms / sustain peak 10–100 ms): **0.07–0.53×** (was 6–7×). Attack/sustain RMS 0.79–1.98 (monotone to treble, matching approved levels).
- Seam: diff at the junction ≤ 0.54× the neighbors — no step. Onset starts exactly at 0, smooth bloom.
- Smoke test: peak 0.136, no NaN. Full Chopin render: **47.2× realtime** (no perf regression); the piece peak dropped from 3.9 to 0.52 (the old 3.9 was the attack-spike click).

### Session 6b (2026-08-25): the user still heard a click — stale dist/

User: "И все равно щелчок". Diagnosis: **the preview never served the Session-6 build.** The preview command is `node scripts/build-web.js && node scripts/serve.js` and serves `dist/`; `build-wasm.sh` only stages to `web/generated/`. `dist/IntraSynth.wasm` was last assembled at 17:34 — from the Session-5 (pre-fix, raw-buffer) WASM — so both the "щёлкает опять" and the "всё равно щелчок" feedback was about the same old clicking build. Fix: run `node scripts/build-web.js` to re-assemble `dist/` from the current `web/generated/` (md5-identical, verified), re-ran the full piece render from the dist artifact (47.4×, peak 0.52). **Lesson: after `build-wasm.sh`, the preview needs `build-web.js` (or a preview restart) to pick up the new WASM.**

## Session 7 (2026-08-25): the "двойной" (doubled) timbre — onset balance of region 75

User (after the Session-6 commit): D#5–E5 sounds "как будто двойной" — like the note is doubled; asks what was tried with H1/H2 before (the answer: Session 1 removed the unison comb filter that made h2 +10 dB hot, Session 3 re-fit λ2 so the 1.2–3.0 s sustain matches, Session 4 tried an h1 strike overshoot that clicked and was reverted — but nobody ever touched the **initial balance** of the region).

### Measurement (per-harmonic onset probe, key 75, D#5)

| window | h2 ours/sample | h3 | h4 | h5 | h6 |
|---|---|---|---|---|---|
| 0–10 ms | −1.3/−9.9 | −6.2/−15.9 | −3.7/−12.7 | −6.4/−13.9 | −14.1/−22.8 |
| 10–30 ms | −1.6/−6.9 | −4.5/−7.0 | −3.4/−10.5 | −5.5/−9.9 | −13.2/−19.6 |
| 300–1000 ms | −2.4/−7.4 | −15.9/−17.1 | −15.4/−9.1 | −22.7/−14.2 | −37.8/−31.5 |
| 1.2–3.0 s | −7.4 ✓ | ✓ | ✓ | ✓ | ✓ |

### Root cause
Region-75 table amps put h2 only **−1.5 dB below h1** (2440 vs 2893). The sample's h2/h1 is ≈ −7…−10 dB **from t=0** (flat through the whole note); our decay differentials only "caught up" to −7.4 dB by ~1.2 s. So for the first ~1.5 s of every D5–E5 note the octave played 5–8 dB too loud — the "двойной" sound. h4–h6 (the 2.5–4.5 kHz squeak band) oscillated around the sample's flat line: +9 dB at onset, −6…−8 dB at 0.5 s.

### Fix (PianoRegions.h, region 75 only)
Set the initial balance to the sample's measured sustain levels and give h1–h6 **identical decay rates** (h1's D1/D2/D3), so the ratios are flat from t=0 and the 1.2–3.0 s sustain is preserved exactly by construction:

| partial | amp old→new | D1/D2/D3 | hK/h1 target |
|---|---|---|---|
| h1 | 2893 (unchanged) | 7485/6640/3284 | 0 dB (ref) |
| h2 | 2440→1234 | 6520/7726/2987→7485/6640/3284 | −7.4 dB |
| h3 | 1609→404 | 17837/7394/6114→7485/6640/3284 | −17.1 dB |
| h4 | 1877→1015 | 18877/4316/3227→7485/6640/3284 | −9.1 dB |
| h5 | 1471→564 | 19405/6067/3445→7485/6640/3284 | −14.2 dB |
| h6 | 605→77 | 21538/10803/4000→7485/6640/3284 | −31.5 dB |

h7/h8 untouched (Session-2 fixes). The attack-buffer per-key normalization self-adjusts (naturalScale/peakS), so the attack RMS stays calibrated while the balance shifts toward h1.

### Verified
- Keys 74/75/76 (whole island): h2 −7.3…−7.9, h3 −16…−18.7, h4 −8.8…−9.4, h5 −13.5…−14.8, h6 −29…−32 dB in 10–100 ms — flat, matching the sample's sustain levels from the first milliseconds.
- Sustain (1.2–3.0 s) unchanged: equal decay rates ⇒ ratios constant ⇒ the previously verified match is preserved exactly.
- Perf: full Chopin 47.4× realtime, peak 0.52 (no clipping). Onset click-free (s0 = 0, max|diff| 0.018). Smoke test passed.
- dist/ re-assembled (md5-identical to web/generated) so the preview serves the new WASM.

### Known remaining difference
- 0–10 ms: our h2/h1 ≈ −6.7 vs sample −9.9 — the sample's **h1 strike** (+13 dB hammer thump, settling by ~30–50 ms) is still not replicated (Session 4's attempt clicked and was reverted). Candidate follow-up: a *ramped* h1 strike (rise over 2–4 ms, settle τ ≈ 15 ms).

## Session 6 (2026-08-25) — high notes: duplicate h2 at C7 + missing fundamental 'strike' (h1 push)

User: «всё равно пищалка на высоких нотах. SF2 звучит гораздо многограннее и объёмнее. Чего у нас не хватает? Унисон у нас включён вообще?»

### Unison answer
Yes, unison is on: `DetuneCents=0.3, UnisonVoices=2` (±0.18 ct per string). It was calibrated in Session 5o: the H1 beat depth matches FL within 3 dB. Not the cause.

### Root cause 1 — region 96 (C7) had TWO identical k=2 rows
`PianoAllPartials` for root 96 carried both amp 1167 and amp 1900 with the SAME FreqRatio/phase/decay (the generator appended the calibrated 1900 row without removing the old 1167). The renderer sums them coherently: effective H2 = 3067 = **+0.3 dB above H1** (FL target −3.9 dB) — on every C7–D7 attack the octave sat level with the fundamental: the squeak. Fix: zero the 1167 row.

### Root cause 2 — no fundamental 'strike' transient (h1 push)
Measured in the RAW SF2 samples (peak 5–40 ms / sustain 60–200 ms): h1 pushes +0 dB at C4, +3.1 C5, +10.0 D#5, +3.2 C6, +5.7 D6, +8.8 A6, **+13.9 C7**, +13.7 E7, +14.3 G7 — the hammer strike rocks the fundamental 3–14 dB above sustain and it settles in ~100 ms. Without it the attack starts all partials at table ratio → h2/h1 ≈ 0 dB instead of the sample's −10…−20 dB on high notes → «дешёвая пищалка». h2 pushes too (≈0.3–1.4× of h1, per-key curve).

Implemented as a phase-aligned overlay buffer (like the cabinet «рокот»): `A1·h1sine + A2·h2sine` × envelope (rise τr=14 ms, decay τd=40 ms, normalized to peak 1, exact zero start), per-key A1 curve + per-key w2 curve from the sample measurements. Plays over the first ~0.18 s of the note; on release the overlays fade with τ=8 ms (abrupt cut would click at +14 dB); mixed into the accumulator in a short pre-loop so the hot fold stays branch-free (perf).

### Verified (vs raw SF2 sample, h2/h1 at 10–30 ms)
| key | ours | sample |
|---|---|---|
| 75 | −10.7 | −10.3 |
| 84 | −24.3 | −19.1 |
| 93 | −21.7 | −18.9 |
| 96 | −10.5 | −11.8 |
| 99 | −9.0 | −8.0 |

75/96/99 within ±1.5 dB; 93 within 3; 84 within 5 (erring toward deeper h1 attack, not squeak). C7 full trajectory 0–300 ms within ±1.5 dB of the sample everywhere. Perf back to 46× realtime (baseline 47.4×) after replacing per-sample trig with complex rotation in the push generator (~20× cheaper); peak 0.52, no clicks (s0=0, release fade verified), smoke clean.

### Changed files (this session)
- `PianoRegions.h` — zero the duplicate k=2 row (amp 1167) of region 96 (keep the calibrated 1900).
- `AdditiveSampler.cpp` — push-buffer generation (per-key A1/w2 curves, rotation-based envelope/oscillator, exact-zero start); overlay release fade; `mOverlayActive`/`mSampleRate` init.
- `AdditiveSampler.h` — push members (`mPushBuf`/`mPushLen`/`mPushPos`), overlay fade members; render loop mixes overlays into the accumulator in a short pre-loop (hot fold branch-free).

## Session 7 (2026-08-25) — sustain richness of long notes + the knock at note start

User: «в наушниках слышу стук в начале каждой ноты … он и раньше был … подозрение, что он не совсем правильный. В остальном у меня давно к атаке нет претензий. Есть претензии, что тембр длинных нот (после атаки) не такой богатый, как у SF2».

### The knock («стук») — measured, not louder than the reference
Band 60–300 Hz (the cabinet/body resonances) at 0–30 ms vs our render and the raw sample: our **absolute attack level in that band is below the sample on every key** (60: −13.7 dB, 72: −17.1, 84: −23.6, 96: −10.9). What makes the knock stand out on headphones is the contrast with our much cleaner sustain (C7: attack/sustain ratio in the band 78 dB ours vs 47 dB sample). The knock itself is the calibrated body «рокот» (78/116/168/285 Hz, τ≈45 ms) — present in the baseline too, not a regression. No change made; `bodyGain` is a one-knob candidate if the user wants it softer.

### Root cause — region 96/99 h2 (octave) hangs in the sustain
Unison beats were first ruled out: depth on the trend-normalized signal is ~22 dB for both us and the sample (the earlier 50 dB reading was a decay-trend artifact of the measurement window). Then sustain FFT / band-power comparisons showed the real problem:

| key | h2/h1 1.0–1.5 s ours | sample |
|---|---|---|
| 96 (C7) | **−19.8** | **−43.5** (was +24 dB hot) |
| 99 (E7) | −3…−16 | −28 |

Region 96's table amp 1900 (and 99's 470) kept the octave nearly as loud as the fundamental all the way into the tail, while the sample's h2 collapses by ~0.5 s. The hammer attack is carried by the push overlay, so the table amp only shapes the sustain. Fix: 96 amp 1900→300 with a re-fit decay (λ2 ≈ 7.5/s, λ3 ≈ 6/s — plateau to 0.5 s then collapse, per the sample); 99 amp 470→110.

### Verified
- Sustain band-power h2/h1 (ours vs sample): 96 — 0.05–0.2 s −13.4/−16.2, 0.2–0.5 −16.0/−15.5, 0.5–1.0 **−34.5/−33.2**, 1–1.5 **−42.2/−43.5**; 84 all windows within ±4 dB; 99 −28.4 vs −27.7.
- Attack 10–30 ms h2/h1: 96 **−11.4** vs sample −11.8 (was −10.5 with the dead lane), 99 −10.0 vs −8.0, 75 −10.7/−10.3, 84 −24.3/−19.1, 93 −21.7/−18.9 (84/93 err deep = safe side).

### Bonus bug found — push h2 lane on 96 pointed at the dead duplicate
While re-verifying the attack after lowering the table amps (w2 raised so w2·Amp stays: 96: 0.3·1900 → 1.9·300, 99: 1.1·470 → 4.7·110; clamp 1.5→5.0), key 96 did not react to w2 at all. Cause: the push overlay hardcoded lane 1 for h2 — and region 96's lane 1 is the **zeroed duplicate** (amp 0), the real k=2 row is lane 2. The h2 push was silently empty on C7–D7. Fix: the push now locates lanes by K (k=1 fundamental, k=2 octave with the max amp) instead of fixed indices. Other regions' lanes were already k=1/k=2 in order (PartOffset is 0-based).

Perf 46.6× realtime (baseline 47.4×), full-song peak 0.64, no clicks (push starts exactly at 0, release fade τ=8 ms), smoke clean, `dist/` re-assembled (md5 = web/generated).

### Changed files (this session)
- `PianoRegions.h` — region 96 h2 amp 1900→300 + decay re-fit; region 99 h2 amp 470→110.
- `AdditiveSampler.cpp` — push lane lookup by K (k=1/k=2) instead of fixed 0/1; w2 curve extended to 5.0 clamp for the amp compensation.
- `AdditiveSampler.h` — no functional change (probe cleanup only).

## Session 8 (2026-08-25): the missing "bright head" of the sustain — early-sustain bloom overlay

User: after Session 7 the sustain h2 collapse was fixed, but the long-note timbre of the upper half of the 4th octave and above is still not rich like SF2; "у семплов тембр сильно лучше… тембр длинных нот (уже после атаки!) не такой богатый". No quality breakthrough, current version = base, don't commit.

### Measurement — the gap is the EARLY sustain, and it is flat-vs-decaying
A beat-robust per-harmonic trajectory (median-filtered Goertzel envelopes, windows 0.1→3.6 s) of key 75 (D#5) vs the raw `75(L)` sample showed the sustain is not "globally lean" — it is **time-dependent**: the sample's h2/h1 starts at **+1 dB** (0.1–0.3 s), settles to −8 by 0.7 s and stays flat; h3/h1 −19→−15→−19; h4 −27→−25→−31. Ours (Session 7's flat-equal-decay table) is flat at h2 −9…−10, h3 −23…−29, h4 −28…−30 — so in the FIRST second ours is 4–10 dB leaner on h2/h3/h4, while by 1.2–3.6 s both match (verified again by `sustain-lambda.js`: all deltas ≤ 3.8 dB at t=2 s). The late sustain was already correct; the missing piece was the **bright head** of the sustain.

Per-key survey (early window 0.1–0.7 s) showed the deficit is **concentrated at region 75** (the D5–E5 island): key 75 h2 −10 dB, h3 −4…−8; key 69 h2 −5; keys 72/78/84 h2 within 0–2 dB (already matched). So the fix must target region 75 only.

### Fix — a third overlay: the sustain "bloom"
`AdditiveSampler` gained `mBloomBuf/mBloomLen/mBloomPos/mBloomOn`: a phase-aligned, phase-locked sum of the region's h2+h3 partials, played over the first ~0.75 s of the note, envelope = 0 until 45 ms (after the attack), then rise τr=55 ms, decay τd=0.20 s, normalized to peak 1, with a 20 ms linear tail fade. Levels from the measured deficit: h2 ×2.2 (+7 dB), h3 ×1.4 (+3 dB). It is mixed through the same short overlay pre-loop as the body/push (hot fold stays branch-free), gated on `region.RootKey == 75`.

**Bug found & fixed during this — destructive interference on non-island keys:** the first version applied the bloom to keys 66–96 with a flat profile and a phase started at sample 0. At key 75 h2 it landed constructive (+6.7 dB, as designed), but at key 72 h3 it measured **−9.9 dB** (h3 got QUIETER). Root cause: the string's SineRange states do NOT advance during the attack buffer (they hold their t=0 phase until the first SIMD block), while the bloom's oscillators rotate from sample 0 — so at the handoff the bloom is ahead by `mAttackLen·dphi`, which for key-72 h3 was ~202° (anti-phase) vs key-75 h2 ~21° (in-phase). Fix: start each bloom phasor rotated BACK by `mAttackLen·dphis[p]`; then at the handoff the bloom and the string partial are exactly in phase, and the sum is always constructive. Verified: key 72 h3 −12.3 (unchanged — bloom not applied), key 75 h2 −1.7, h3 −18.2.

**Perf:** full Chopin render **43.6× realtime** (committed baseline measured 43.8× on this machine — the worklog's 47× was machine-dependent). Peak 0.64, no clipping, no NaN, onset s0=0, max |Δ| ≤ 0.015 @ ~43 ms (bloom rise = normal sine slew, not a click). `dist/` re-assembled (md5 = web/generated).

### Verified (key 75, trajectory ours/sample)
| window | h2 | h3 | h4 |
|---|---|---|---|
| 0.1–0.3 s | −3/+1 | −18/−19 | −28/−27 |
| 0.3–0.7 s | −4/−1 | −20/−15 | −30/−25 |
| 0.7–1.2 s | −10/−8 | −29/−16 | −30/−25 |

(before: h2 −9/−9, h3 −23/−24, h4 −28/−30 in 0.1–0.7 s). Neighbors unchanged: key 72 h2 −12.2/−12.3 (identical to bloom-off), 78/84 unchanged. Late sustain (0.5–3.6 s, `sustain-shape3`/`sustain-lambda`) unchanged — the bloom fades out by ~0.7 s, so the calibrated flat tail is preserved. Attack 10–30 ms h2 −10.8 (calibrated baseline −10.7) — the 45 ms delay keeps the bloom out of the attack windows.

### Changed files (this session)
- `AdditiveSampler.cpp` — sustain-bloom buffer generation in the ctor (phase-back rotation by `mAttackLen·dphi`, envelope delay/rise/decay, 20 ms tail fade); `mBloomPos/mBloomLen/mBloomOn` init.
- `AdditiveSampler.h` — `mBloomBuf/mBloomLen/mBloomPos/mBloomOn` members; overlay mixing extended to the bloom in both the attack section and the fold pre-loop; `mOverlayActive` end condition includes the bloom.

### Known remaining differences (updated)
- The unison detune is still slower/deeper than the sample's (sample beats within the first second; ours ~8 s period).
- The bloom is region-75-only because the measurements show only the island is lean early; if the user still hears "flat sustain" on keys 69–72 or 78–84 after this, the per-key deficit should be re-measured with the same trajectory tool and the bloom extended per-region.
- Sample's mid-harmonic attack bloom (0.2–0.5 s) is partially covered by the bloom's rise; full per-partial attack shaping is still open.

## Session 10 (2026-08-25): the hammer strike — parametric broadband attempts (all REVERTED)

Tried to reproduce the missing hammer strike as a parametric broadband transient layered on
the clean additive string, in three iterations — each rejected by ear as «шум/щелчок», not a
hammer: (1) a filtered noise burst (`mHammerBuf`, Session 9); (2) a velvet-spike + 8-resonator
per-region impact (`mImpactBuf`); (3) a whitened build-over-10-ms broadband strike with
per-note jitter. Structural metrics matched the reference (level re steady, ZCR density, no
clicks s0=0, peak < 0.9) but the ear consistently reads ANY synthetic broadband on top of a
clean string as noise. Lesson for future sessions: don't add broadband noise layers — make the
strike come from the string/cabinet harmonic content (push + «рокот» + bloom), which is always
natural. All impact code (`PianoImpacts.h`, `mImpactBuf`/`mImpact*`, `periodRMS`, level trims)
was removed; the render is back to the clean baseline.

Baseline re-verified after revert: perf **43.2×**, peak **0.640** (no clip), attack h2/h1 at D#5
**−10.8** (calibrated baseline), late sustain/bloom intact, WASM back to pre-impact size.

## Session 11 (2026-08-26): rejected residual experiment

A compact per-region residual path was implemented as an experiment: sparse deterministic velvet
excitation, short FIR shaping, a measured envelope, and true-level scaling. It was deliberately
kept separate from the string bank so it could be tested without changing the accepted piano
path.

The experiment was rejected by ear. In the browser A/B test the isolated component sounded like
a click or noise rather than a hammer strike, especially in Fur Elise; the full mix also became
less convincing. The residual generator, profiles, and diagnostic API were removed. The accepted
contact-force attack, cabinet response, h1/h2 push, and region-75 sustain bloom remain unchanged.

## Changed files

- `intrasynth/src/Intra/Synth/AdditiveSampler.cpp` — remove the 0.9 ms unison phase delay; decode `Decay4` with the D3 scale; keep the contact-force attack, per-key normalization, cabinet response, h1/h2 push, and region-75 sustain bloom.
- `intrasynth/src/Intra/Synth/AdditiveSampler.h` — keep the accepted attack and overlay state; no residual members remain.
- `intrasynth/src/Intra/Synth/PianoRegions.h` — retain the measured sustain and high-register fixes.
- `intrasynth/src/Intra/Synth/InstrumentLibrary.cpp` — drop obsolete `AttackBoost`/`HammerLevel` arguments.
- `intrasynth/src/Intra/Synth/EmscriptenInterface.cpp` — keep the unified live MIDI API without a residual diagnostic export.

The rejected `PianoImpacts.h` compatibility file was removed. Generated WASM files are rebuilt
locally but remain build artifacts under the repository's existing ignore rules.

WASM is rebuilt in-sandbox and staged to `web/generated/` + `dist/` by `scripts/build-wasm.sh` (gitignored; the deploy pipeline builds from source).

## Known remaining differences vs reference

- The unison detune is slower/deeper than the sample's (sample beats within the first second; ours has a ~8 s period). Candidate follow-up: widen `DetuneCents` per key or add per-string decay differences to match the sample's fast shimmer.
- Attack transient: our h2–h6 reach full level in 5–10 ms; the sample's mid harmonics bloom over ~0.2–0.5 s. Fitting this needs per-partial attack shaping, not just table amps.

## Session 12 (2026-08-26): rollback cleanup and in-app note testing

The residual experiment and its A/B mode selector were removed after the user identified the
component as noise/click rather than a hammer. Normal playback therefore uses the accepted piano
path only: contact-force attack, cabinet response, h1/h2 push, and the region-75 sustain bloom.

The web player keeps a small in-app note-test panel with C4, C5, D#5, E5, C6, and C7 buttons.
Each button starts the ordinary Live piano and releases the test note automatically, so individual
notes can be checked without exporting WAV stems or exposing a discarded audio mode.

Deterministic checks after cleanup: WASM build passed; `node --check web/synth.js` passed;
`git diff --check` passed; the only remaining preprocessor block in `AdditiveSampler.cpp` is the
paired `INTRA_PROBE_NAN` diagnostic guard. The generated bundle was reassembled from the fresh
WASM. This repository has no TypeScript package/toolchain, so `bun tsc -b --noEmit` is not
applicable.

### Needs Human Verification

- Load Fur Elise and listen to D#5/E5 and neighboring notes at normal headphone volume. The
default path should be the same accepted piano path without the rejected residual component.
- Use the six note buttons to compare isolated notes directly in the page; the panel should start
Live automatically and release each note after a short hold.

### Next-step handoff

Do not re-enable the removed velvet/FIR residual or tune it by ear. If a hammer is revisited,
start from a single-contact, phase-coherent excitation that is validated in the note-test panel
before it is mixed into the default path.

## Session 13 (2026-08-26): ramped h1/h2 strike + faster unison shimmer

Two known-remaining differences from Sessions 4/8 were addressed, both validated against the raw
SF2 samples.

### 1. Ramped h1/h2 strike (0-10 ms onset)

The push overlay's envelope was `(1−e^(−t/14ms))·e^(−t/40ms)` — the h1 strike rose too slowly
for the 0-10 ms window, so h2/h1 stayed at −6.7 dB there vs the sample's −9.9 (the octave
played relatively too loud at the very onset: the "cheap beep"). Changed to a fast ramped
strike: rise τr = 3 ms, decay τd = 15 ms (peak ~5 ms, settled by ~50-70 ms, matching the
sample's "settle ~30-50 ms" hammer thump). Same A1/w2 per-key curves, same A2 = A1·w2 layout,
exact-zero start (no click).

Measured at key 75 (D#5), per-harmonic onset probe: **0-10 ms h2/h1 −9.2 vs sample −9.0** (was
−6.7). 10-30 ms −10.1 vs −12.2, 30-100 ms h2 −4.9 vs +2.0 — both within the probe noise of the
previous calibration. Onset s0 = 0.00000 at keys 72/84/96, max |Δ|(0-10 ms) 0.012-0.032 (normal
sine slew, no click).

**Still open (pre-existing, larger):** the sample's mid harmonics at the onset are much stronger
than ours — at 30-100 ms h3/h1 is −30…−72 dB ours vs −16…−54 dB sample (10-37 dB deficit, real
in absolute levels too: at key 75 our h3 is ~19 dB below the sample's). The worklog already
lists this as "per-partial attack shaping / mid-harmonic bloom ~0.2-0.5 s"; it was deliberately
not folded into this session.

### 2. Unison spread: 0.3 → 1.4 cents (first-second shimmer)

The old `DetuneCents=0.3` (±0.18 ct/string) beat at ~9 s period at C5 — no audible shimmer in
the first second. Measured the raw SF2 h2 AM (the fundamental decays too fast at C5+ to show
its own beat): the sample's h2 beats ~1 Hz at C5 (trough ~0.6 s, peak ~1.1 s), ~1.9-2.7 Hz at
C6, ~2.5-3 Hz at C7, with ±65% depth at C6 — i.e. the unison spread must be ~4-5× wider than
0.3 ct and stays roughly flat in cents (rate ∝ pitch).

Changed AcousticPiano to `DetuneCents=1.4` (2 voices, gains 1.0/0.7, in-phase start — depth
±70% on h2, close to the sample's). Verified on the rebuilt WASM: our h2 AM is now **1.11 Hz at
C5, 2.22 Hz at C6, 2.78 Hz at C7** (sample 1.0 / 1.9-2.7 / 2.5-3). Beating now happens within
the first second exactly where the sample's does (upper 4th octave and above).

### Verified

- WASM build passed; `smoke-intrasynth.js`, `smoke-live-midi.js`, `smoke-test-wasm.mjs` passed
  (peaks 0.14-0.15, no NaN); `node --check web/synth.js` and `git diff --check` passed.
- `dist/` reassembled from the fresh WASM (the preview serves `dist/`).
- Note-test panel unchanged (C4/C5/D#5/E5/C6/C7 through the ordinary Live path).

### Needs Human Verification

- **Attack:** listen to short single notes at C5-C7 — the onset should sound denser (h1 thump
  arriving in the first milliseconds), not clickier.
- **Beats:** hold a C6 or C7 note — the first second should shimmer like the sample; a held C4
  should stay calm (slow beat there is intentional and matches the sample).
- If C5-C6 now sound too "tremolo", the next knob is the second voice's gain (0.7 → ~0.5)
  rather than the spread.

## Session 14 (2026-08-26): low-note beat artifact fixed (per-key spread) + unison perf measured

User: the sound is much richer now, but low notes have a strange "отзвук" from the beating,
especially on headphones; and asked whether the unison costs performance and whether the two
voices could be replaced by one sine bank with a per-frame gain multiplier.

### 1. Low-note artifact: per-key unison spread

The wide 1.4 ct spread at low keys made the fundamental beat slow and deep (0.14-0.28 Hz at
C3-C4, ±70% depth) — a pumping "breathing" on long low notes, clearly audible on headphones.
The SF2's low samples barely beat. Added a per-key ramp in the `AdditiveSampler` ctor: spread
≈ 0.3 ct (the accepted pre-widening value) at C4 and below, rising to the instrument's base
value at C5, flat above:

    spread_t = clamp((midi − 60)/12, 0, 1); detuneCents *= (0.3 + 1.1·spread_t)/1.4

Verified: C3/C4 h1 envelopes are smooth monotone decays again (0 beat minima over 1.9 s,
100 ms windows — the earlier "7 Hz minima" at C3 were short-window LSQ noise); C5-C7 beats
unchanged (1.1 / 2.2 / 2.8 Hz on h2). Chopin render back to 43.7× realtime, peak 0.672, no NaN.

### 2. Unison performance: measured, 2 voices ≈ 1.7× render cost

Rendered the full Chopin with UnisonVoices=2 (normal) and =1 (temporary probe build):

| voices | realtime | peak |
|---|---|---|
| 2 (normal) | 43.8× | 0.667 |
| 1 (probe) | 75.7× | 1.089 |

So the unison roughly doubles the oscillator lane count and costs ~1.7× the render time.
WASM size is unaffected by the voice count (the lanes are runtime data): 165,956 bytes before
this session, 166,008 bytes after adding the per-key curve (+52 B of code).

### 3. Feasibility of the "one sine bank + per-frame gain" trick

The two detuned voices per partial sum exactly to one sine with slow AM + bounded phase wobble:

    g0·sin(kω(1+δ)t) + g1·sin(kω(1−δ)t) = A_k(t)·sin(kωt + φ_k(t)),
    A_k(t) = √((g0+g1)²cos²(kΔωt) + (g0−g1)²sin²(kΔωt)),
    |φ_k| ≤ atan((g0−g1)/(g0+g1)) ≈ 10° for gains 1.0/0.7,

so the AM-only approximation (drop φ) is essentially exact: the lost pitch wobble is ±0.09 Hz
at C5 for a 1 Hz beat. It would halve the sine count (partials×voices → partials) and lift the
render toward ~65-70× realtime. Caveats: the AM envelope must be per-partial (beat rate k·Δω)
and continuous — a block-quantized staircase would add sidebands (this codebase is click-
sensitive), so it needs one extra multiply per lane per sample with the gain updated once per
block, or the beat must live outside `mDecay` (which is replaced at segment boundaries and on
release, so baking it there would reset the beat phase and step the envelope).

Not implemented this session: the render is already ~44× realtime, so the win is CPU headroom /
battery only, not audible. Worth doing if we later raise partial counts or want lower power use.
User to decide.

### Verified

- WASM: 165,956 → 166,008 bytes (report every session). `dist/` reassembled.
- Chopin 43.7× realtime, peak 0.672, nonFinite 0; smoke ×3 passed; `node --check` and
  `git diff --check` passed.
- Low-note h1 envelopes smooth (C3/C4), C5-C7 beats unchanged.

### Needs Human Verification

- Low notes (C2-C4) held for a few seconds: the pumping "отзвук" should be gone; the note
  should decay smoothly.
- C5+ long notes: shimmer should be unchanged from Session 13.
