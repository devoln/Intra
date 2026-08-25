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

## Changed files

- `intrasynth/src/Intra/Synth/AdditiveSampler.cpp` — remove the 0.9 ms unison phase delay (comb fix); decode `Decay4` with the D3 scale (5461.25) so `D4 = D3` is a no-op; contact-force attack (attack buffer, Tikhonov G, `driven` seeding); fix `driven` to exclude silent/padding lanes (perf gate); per-key attack/sustain normalization + bloom fade-in; cabinet «рокот» layer (4 body resonances, impactV).
- `intrasynth/src/Intra/Synth/AdditiveSampler.h` — attack-buffer members (`mAttackBuf`/`mAttackLen`/`mAttackPos`), per-partial `mAtk` docs; remove `PianoEnvelope` include and envelope-correction fields; remove 0.9 ms delay note.
- `intrasynth/src/Intra/Synth/PianoRegions.h` — region-75 (D#5) D2 (λ2) re-fit per harmonic + high band (h9+) decay (D3 273→6000) + h7/h8 amp.
- `intrasynth/src/Intra/Synth/InstrumentLibrary.cpp` — drop `AttackBoost`/`HammerLevel` args (contact force is the only attack).
- `intrasynth/src/Intra/Synth/EmscriptenInterface.cpp` — remove stale debug-hook comment.

WASM is rebuilt in-sandbox and staged to `web/generated/` + `dist/` by `scripts/build-wasm.sh` (gitignored; the deploy pipeline builds from source).

## Known remaining differences vs reference

- The unison detune is slower/deeper than the sample's (sample beats within the first second; ours has a ~8 s period). Candidate follow-up: widen `DetuneCents` per key or add per-string decay differences to match the sample's fast shimmer.
- Attack transient: our h2–h6 reach full level in 5–10 ms; the sample's mid harmonics bloom over ~0.2–0.5 s. Fitting this needs per-partial attack shaping, not just table amps.
