---

title: "Flute: real FM vibrato with onset delay + per-register depth/rate + register-scaled breath (high octaves no longer rustle)"
status: "active"
created: 2026-09-05
started: 2026-09-05
updated: 2026-09-12
risk_level: medium
related_files:
  - intrasynth/src/Intra/Synth/InstrumentLibrary.cpp
  - intrasynth/src/Intra/Synth/Envelope.h
  - intrasynth/src/Intra/Synth/WaveTableSampler.h
  - intrasynth/src/Intra/Synth/WaveTableSampler.cpp
  - intrasynth/src/Intra/Synth/ComputeKernels.h
  - .gitignore
related_tasks:
  - 20260904-EpBeatsAndEgSaw
  - 20260903-MidiTracksAndSampleTabs
  - 20260828-LoudnessBalanceFix
---

# 2026-09-05 — Flute (GM 73): vibrato onset + per-register profile + breath

## Update 28 — Bloom decay bug (h4 stuck bright the whole note) fixed; WASM size root cause = mixed build configs

### The "wasm опять разросся / сжатие пропало" answer (no code change lost it)
Update 26's report of **199,448 B** was produced by `scripts/build-wasm.sh`'s
OLD default configuration (**plain `-Os` everywhere, no LTO, ALL_TABLES=ON**),
while the Q16 table compression (Update 20, −17.5 KB) and every staged
flute/woodwind build since had used the **size-optimized hybrid**
(`-Oz` cold + hot render loops at `-O2` as real wasm objects + LTO + emmalloc,
ALL_TABLES=OFF) — the flute-era builds were ~181 KB. The 199,448 vs ~181 KB
gap is the two build configurations being ~18 KB apart (codegen + the
per-instrument piano tables), **not** the compression being lost: the Q16
packed zone profiles are in the current source and in the staged wasm.

Reconciliation so this cannot recur:
- `scripts/build-wasm.sh` default mode is now the size-optimized hybrid (the
  "compressed" build that is actually staged for the player). Modes:
  `sh scripts/build-wasm.sh [default|full|speed]` — `full` adds the
  per-instrument piano tables (PianoTablesExtra.h, ~193 KB), `speed` restores
  the old plain `-Os` + ALL_TABLES=ON build (199 KB, fastest codegen).
- `scripts/build-wasm-size.sh` is now an alias for the default mode (kept so
  older worklog commands keep working).
- From now on every WASM size report states the configuration that produced it.

### Update 27 (previous session, undocumented until now): attack spectral-evolution bloom for 43/115
FluteClean (43) and FluteHybrid (115) got a `BloomSampler` overlay
(`Synth.h`; `MakeFluteBloom` in `InstrumentLibrary.cpp`, timings per register
from `.scratch/bloom-measure.mjs` bank-onset windows): phase-aligned additive
harmonics — a bright flash (positive amps, mostly h4/h5) that rises in
~0.05-0.1 s and decays exponentially, plus a negative "swell" (h2/h3) that
subtracts while the tone is still dark, both gated by the tone's own rise. This
is the spectral evolution the bank's recorded onset has (bright attack → darker
sustain loop) that a static wavetable cannot express. Staged WASM at the time:
181,343 B (md5 `7d1cba27…`, default config).

### This session: the bloom's flash never decayed (h4 stuck +10…13 dB)
Listener/trace report: "почему h4 на C4 у нас на +12.6 дБ ярче банка" — the
A/B of the fixed build (`.scratch/compare-bloom-ab.mjs`, program 43 key 60,
0.6-2.4 s windows): with bloom h4 sat at **−10…−13 dB rel h1 forever**; the
pre-bloom Update-26 build reads −23.2 = the bank exactly. So the "bright
attack" was leaking into the whole sustain.

Root cause (`BloomSampler::Generate{Mono,Stereo}`): the flash envelope's state
machine was `if(flash < 1) flash += riseStep; else flash *= decayStep`. Once
the exponential decay dropped flash below 1.0 the rise branch re-engaged, and
near 1.0 the linear rise (2.27e-4/sample) beat the multiplicative decay
(1.74e-4/sample) — flash hovered at ~1.0 for the entire note instead of
decaying to zero. Fix: single-shot rise 0→1 (clamp to 1.0 and set
`mFlashPeaked` once), then monotonic exponential decay only.

### Verification (fixed build, web/generated = dist, md5 `e2e9015f…`)
- Sustain now equals the no-bloom build and the bank at every window: C4 h4
  −23.2 (bank −23.2), h2/h3/h5/h6 ±0.3 dB (`.scratch/compare-bloom-ab.mjs`).
- Per-zone 1.0-2.0 s check (`flute43-zones.mjs` keys 60/72/84): all audible
  harmonics ≤ 0.5 dB vs the bank — including C5 h2/h3/h6, which the
  0.6-2.0 s-window probe (`.scratch/flute43-vs-titanic.mjs`) misreports as
  −3.5…−4.8 dB due to its window/method; the bank itself measures −3.5 dB
  brighter in that probe, i.e. it is a probe artifact, not a synth error.
- Attack (trace-h4.mjs, C4): h4 ≈ −15 dB rel h1 in the first 0.15 s (+~8 dB
  flash over the −23 sustain), settling into the sustain by ~0.4-0.5 s — the
  intended short bright onset. First window is unchanged vs the buggy build
  (flash reached full during the rise either way); only the decay is fixed.
- `scripts/smoke-test-wasm.mjs`: PASSED (0 non-finite).
- WASM (default config): **181,421 B** (was 181,343 buggy; +78 B for the fix),
  IntraSynth.js 15,158 B, md5 `e2e9015f…`, staged web/generated/ = dist/.
- A/B rows re-rendered on the fixed build (loudness-matched −20 dBFS, 2-13 s):
  `cherilady_ours_titanic_15s.wav` (43, md5 `07f7fc1b…`),
  `cherilady_ours_hybrid_15s.wav` (115, md5 `5b8ec809…`). The DLS row (73)
  is untouched (FluteDLS has no bloom).

Artifacts: `.scratch/compare-bloom-ab.mjs`, `.scratch/trace-h4.mjs`.

## Update 26 — FluteClean (43) retuned to the bank's SUSTAIN loop: the 2-second A/B is now Titanic, not the onset

### Listener report
"По рендерам C4-C6 Flute стало понятно, что тембр нашей флейты 43 абсолютно не
похож на Titanic, под который мы подгоняем!" — comparing the sample tabs
(bank, dry 2-s holds) against our live synth on the same keys.

### Root cause
`FluteClean`'s zone tables were tuned to the samples' **bright onset** (Update 12/16
used pre-vibrato onset windows and short-note aggregates), but the listener now
A/Bs 2-second holds: in the bank's sustain loop the timbre is much darker than
its own attack. Measured ours − bank per harmonic at the zone-midpoint keys in
the SAME window the listener hears (1.0-2.0 s, `.scratch/flute43-zones.mjs`,
f1-anchored peak scan): C#4 h4 **+9.9 dB**, E4 h4 **+15.7 dB**, B4 whole zone
+2…+9 dB, D5 h2/h3 +4.4/+2.5 dB, F#5 h2-h4 −4…−5.4 (ours darker), G#5 h2-h4
−1.9…−4.1, C6 h2-h4 −3.7…−6.1, F#6 h9-h12 **+12…+32 dB** (the old "±3 dB"
verification was measured against the onset, not the sustain loop).

### Fix (`InstrumentLibrary.cpp`, all 8 zones)
Each harmonic rescaled by the measured delta: `new[k] = old[k]·10^(−Δ/20)`,
Δ = ours − bank in the 1.0-2.0 s window (`.scratch/retune-flute43.mjs`). C#4/E4/
B4/D5/F#5 were applied in the previous session; this session completed
**G#5/C6/F#6** (they still held the old onset-tuned numbers — the edit tool's
line-reach limit on this file was worked around by compressing the comment
block above the tables). Release stays 0.50 s (bank-like, the Update 19 open
follow-up — it had already been applied). Envelope/vibrato/breath unchanged.

### Verification (same probe, 1.0-2.0 s window, rel h1, ours → titan, Δ dB)
| key | zone | h2..h12 max |Δ|
|---|---|---|
| 60 | C#4 | ≤ 1.2 |
| 64 | E4 | ≤ 0.9 |
| 69 | B4 | ≤ 1.3 |
| 72 | D5 | ≤ 0.5 |
| 77 | F#5 | ≤ 2.3 |
| 80 | G#5 | ≤ 0.9 |
| 84 | C6 | ≤ 2.1 |
| 89 | F#6 | ≤ 2.1 (h2-h8); h10-h12 +10…+28 but at −80…−100 dB rel h1 — below the 16-bit floor |

Every audible harmonic now sits within ±2.3 dB of the bank at all 8 zone
midpoints — the C4/C5/C6 keys of the sample tabs are 60/72/84 (≤ 1.2/0.5/2.1 dB).

### A/B rows re-rendered on the new build (loudness-matched −20 dBFS, 2-13 s)
- `cherilady_ours_titanic_15s.wav` (43, md5 `ad99463c…`)
- `cherilady_ours_hybrid_15s.wav` (115 — shares the `FluteClean` table, md5 `c0d3bd6c…`)
DLS row unchanged (separate voice). `web/index.html` row text updated.

WASM: **199,448 B** (IntraSynth.js 14,367 B), md5 `741e468e…`; smoke test passes
(0 non-finite). Staged in `web/generated/` + `dist/` (wasm, js, index.html,
A/B wavs). The "ours" side of the sample-tab A/B is the live synth — it now
serves the retuned build. Artifacts: `.scratch/flute43-zones.mjs`,
`retune-flute43.mjs`, `cheri43_new.wav`, `cheri115_new.wav`.

> Config correction (Update 28): the 199,448 B figure above was the **plain
> `-Os` build** (`build-wasm.sh` old default: no LTO, ALL_TABLES=ON). The
> flute-era size-optimized builds (Updates 20-23, and everything staged since)
> are the hybrid `-Oz`+hot-`-O2`+LTO config at **~181 KB**; the Q16 packing was
> never reverted. See Update 28 for the script reconciliation.

## Update 19 — note-interruption fix attempt (ADSR segment boundary) + honest A/B: not the regression's cause

### Listener report
"Недавние изменения со стерео и огибающей сломали пады и флейты: ноты
прерываются и щёлкают, могут прерваться и потом внезапно продолжиться, в целом
звучит тише. Поэтому и производительность celine выросла — из-за бага."

### Fix applied (ADSR.h, NoteSampler.cpp — voice-level ADSR lifecycle)
`AdsrAttenuator::operator()` left a *pending* segment when a segment ended
exactly at the end of a render span, so `SamplesLeft()==0` could not
distinguish "envelope finished" from "segment ended at the buffer edge". The
old stereo draft split the span per segment and could zero/kill the voice at
such a boundary (or skip ADSR for the next frame — a frame at full volume then
resume, the "прервалось и продолжилось" pattern). Fix: eager segment
transition at the end of each `AdsrAttenuator` call; `applyModifiersStereo`
applies ADSR once per channel over the whole span from a pre-L snapshot and
removes the voice only when `Active` becomes false.

WASM: **197,036 B** (was 197,124 B at e75bf8f5; IntraSynth.js 14,367 B).
Smoke tests pass (`smoke-intrasynth.js`, `smoke-test-wasm.mjs`; 0 non-finite).

### Verification: the fix changes nothing measurable
- Single-note holds (43/73/115/48/89/91/88/0, BLK 128 and 4096): level grids,
  gaps, clicks, max-jump identical buggy (e75bf8f5) vs fixed.
- celine.mid + cherilady-flute.mid full renders: per-second RMS profile, RMS,
  click count, silent bins byte-same buggy vs fixed.
- 4-melody perf equal (median 5, 48 kHz): Celine 423×RT, Merry Christmas
  587×RT, Tous Les Garçons 722×RT, Chopin 108×RT (doc Update 18 table:
  421/586/741/107). The "too fast / too quiet" symptom is NOT caused by the
  ADSR lifecycle the fix addresses.

### The listener's regression reproduces (pre-stereo → stereo) but root cause is still open
Compared against the pre-stereo build (.scratch/prestereo, md5 `139232a2…`):
- celine **full mix only**: divergences up to −18 dB (corr < 0) in the sparse
  outro 253-282 s plus short 0.15-0.6 s bursts at phrase boundaries across the
  piece; deterministic (same build twice → identical).
- NOT reproducible in any single channel copy, any 8-of-9-channel subset, or
  12 s event clips of the same region — depends on the complete multi-channel
  event grid and/or long render history.
- Flute-family pieces rendered with programs 43/73/74/75/115: pre-stereo vs
  stereo identical (no modifier-path regression in music). Release tails and
  NewAge-style long decays identical. `[VOICES]` probe (INTRA_PROBE_ACTIVE_
  VOICES build): live ≤ 33, oldest ≤ 6 s, no zombie accumulation.
- Suspect: exact frame-boundary voice-lifecycle interaction that only exists in
  the honest-stereo whole-span path vs the old 1024-sample-chunked mono path
  (event grid decides where boundaries land — hence subsets/clips stay clean).
- Next: per-frame voice-kill logging around 253-282 s, or a browser A/B to
  identify which notes drop out by ear.

### Open follow-up (not applied — tooling note)
Listener asked program 43 (FluteClean) to copy the Titanic bank's long decay
(~0.5 s, bank volEnv release ≈ 0.55 s) instead of 0.10 s. The edit is 2
literals (EnvelopeProfile `Segments[4]` + `Wt` fallback → 0.50 s) but the file
editing tool returned a stale snapshot for InstrumentLibrary.cpp this session
(it could not match either HEAD-era or working-tree content), so the file was
left untouched to avoid corrupting the working tree. Re-apply once the
snapshot syncs. FluteHybrid (115) intentionally stays at 0.25 s, FluteDLS at
0.015 s.

## Update 18 — honest stereo for ALL notes (modifiers/ADSR no longer force mono) + 4-melody perf check

### Listener feedback
"43 и 115 звучат по-разному на длинном удержании, будто в 115 два тела; надоделать
все инструменты честным стерео. ADSR вроде выпилили в пользу Envelope? Проверить
производительность на наших 4 мелодиях: последний коммит → до полного стерео → после."

### Root cause (NoteSampler.cpp)
`GenerateStereo` routed notes with modifiers or ADSR through the mono path
(`fill` → mono temp → 0.5/0.5 pan): the wavetable body lost its true stereo
(344-sample R delay) and each noise layer landed +6 dB louder per channel than
on the direct-stereo path — hence "two bodies" in 115 vs 43 (identical body,
DLS-vs-Titanic breath design differs by intent). The mono path dates to the
first synth commit `6c2ed03` (port of devoln/web-midisynth, all-mono render).

### Fix (Update 18, this session)
- `NoteSampler::GenerateStereo` is now the single path for every note:
  `fillStereo` + `applyModifiersStereo` (modifiers apply to each channel;
  the Envelope snapshot is restored between L and R so both channels get the
  identical schedule; exhausted-ADSR zeroes BOTH channels).
- Noise sources made truly stereo at 0.5/0.5 per channel (same level as the
  old mono path): `WhiteNoiseSampler::GenerateStereo`, `NoiseSampler::
  GenerateStereo` (Synth.h).
- `GenerateMono`/`fill`/`applyModifiers` remain for offscreen/mono renders.
- WASM: **197,124 bytes** (was 199,314 pre-stereo) — the stereo path is also
  *smaller* (the mono-fallback block dropped out of the hot code).

### ADSR status (source archaeology)
ADSR as a *data format* is gone from instruments: every `g.Envelope =
MakeEnvelope({...})` (24 instruments) goes through `EnvelopeFactory::ADSR(...)`
(Envelope.h) → `Envelope` (5-segment generic engine). The only remaining
`AdsrAttenuator` (ADSR.h) is a thin NoteSampler-side wrapper that advances the
`Envelope` and applies it — a name, not a second system. AcousticPiano has no
instrument envelope at all (per-partial decay, see the 20260821 decision).

### Perf: 3 builds × 4 melodies (48 kHz stereo, full render, wall-clock, median of 5)
Builds: HEAD `fe9f14e` (last committable; 164,863 B), pre-stereo
(`139232a2`, 199,314 B), honest stereo (`e75bf8f5`, 197,124 B).

| melody | HEAD | pre-stereo | stereo |
|---|---|---|---|
| Celine — My Heart Will Go On (282 s) | 161.6×RT | 401.9×RT | **421.4×RT** |
| Merry Christmas (67 s) | 640.5×RT | 508.6×RT | **586.3×RT** |
| Tous Les Garçons (191 s) | 635.0×RT | 586.4×RT | **740.5×RT** |
| Chopin Fantaisie-Impromptu (290 s) | 106.8×RT | 106.6×RT | **106.5×RT** |

No regression anywhere; Celine is 2.6× faster than HEAD (headroom gained by
the flute rework), and stereo is equal-or-faster than pre-stereo on all four
(the mono temp-buffer path was strictly more work). Piano-dominated Chopin is
unaffected, as expected. Sanity after stereo: 43 vs 115 long-hold harmonics
now identical (±0.1 dB); noise differs only by the intended DLS/Titanic breath
profiles (−51 vs −45 dB rel h1 at key 72). A/B rows re-rendered on the stereo
build and loudness-matched (−20 dBFS): `cherilady_ours_15s.wav` (DLS),
`cherilady_ours_titanic_15s.wav`, `cherilady_ours_hybrid_15s.wav`.
Build `e75bf8f5…` staged in `web/generated/` + `dist/`; smoke passed.

## Update 17 — hybrid flute (DLS attack + Titanic body + 0.25 s release) + perf benchmark vs the committed flute

### Listener feedback (A/B of Update 16 rows)
"DLS всё ещё не хватает дыхания, банк лучше; наш Titanic не похож — оригинал какой-то
размазанный (FluidR3 тоже), будто там реверб. Может, настоящая флейта и должна звучать
невнятно? Хочу средний вариант: атаку от DLS, тело ближе к Titanic (с вибрато), затухание
посередине — 200-300 мс. Каждый раз писать размер WASM. Сравнить производительность
текущей флейты со старой из коммита."

### What the "blur" is (measured, `tails.mjs`)
The Titanic bank rings ~500 ms after note-off (FluidR3 ~780 ms), the DLS cuts in
~20-40 ms — that's the "размазанность/reverb" the listener hears, not reverb
(renders are dry). Our Titanic release was 0.10 s → too short. DLS onset study:
note starts dark (no energy >4 kHz for the first ~130 ms), air arrives later.

### New `FluteHybrid` (program 115 = GM Woodblock slot, which was silence here)
- Attack from DLS (flat 3-4 ms zero + fast 0→1 in 20-30 ms, no swell segment, no
  opening-cutoff modifier); body = the SAME table as `FluteClean` (shared
  `Tables["FluteClean"]` generator → spectrum and vibrato identical by
  construction, zero extra table memory); release 0.25 s — middle ground
  (Titanic ~500 ms / DLS ~15 ms); breath = Titanic's (0.0325·0.70^x,
  min(4·f0, 1700 Hz)).
- Verified: harmonics at keys 60/72/84 match FluteClean's profile (−8/−10/−13/−16
  vs −7/−8/−9/−12; −13/−3/−32 vs −12/0/−28; −19/−28/−42 vs −18/−25/−38).
- A/B row 3 added (loudness-matched −20 dBFS): **Наш · Гибрид**
  `cherilady_ours_hybrid_15s.wav`; banks now rows 4-6. Dropdown: 115 added to
  "Альтернативные флейты".

### Performance: current flutes vs the committed flute (17 s cherilady piece, 749701 samples, wall-clock, median of 5 runs)
Baseline = last committed build (`fe9f14e`; note `ae4f1b5` itself does not
compile — missing `PianoGetPartial`, fixed in `fe9f14e`): GM 72/73/75 = old
spec-based FM `Flute`.

| build | program | voice | median ms | vs old |
|---|---|---|---|---|
| committed (fe9f14e) | 73 | old FM-spec Flute | 6.5 | 1.0× |
| current | 73 | FluteDLS | 9.8 | 1.5× |
| current | 43 | FluteClean | 15.1 | 2.3× |
| current | 115 | FluteHybrid | 15.5 | 2.4× |

All are 1000-2600× realtime (17 s of audio in 6.5-15.5 ms). The 8-zone
MixZoneSets tables + vibrato + noise layer cost ~2.3× the old single-table FM
flute; the hybrid shares FluteClean's table, so it costs the same as FluteClean.
First run per voice is slower (lazy table build: 19-29 ms).

### WASM sizes (scripts/build-wasm.sh now prints them on every build)
| build | IntraSynth.wasm | IntraSynth.js |
|---|---|---|
| committed (fe9f14e) | 164,863 B | 14,367 B |
| current | 199,314 B | 14,367 B |

+34.5 KB (~21%) for the three flute voices' zone tables/breath. Build md5
`139232a2…`, staged in web/generated/ + dist/. Bench: `.scratch/bench-flute.mjs`,
`.scratch/loudmatch.py`.

## Update 16 — DLS breath re-measured (was white-noise hiss) + Titanic retuned from clean 41-key run; program 43 now selectable

### Listener feedback (A/B of Update 15 rows)
"Наш DLS похож, но пшикает белым шумом; в оригинальном DLS нормальное дыхание, не
похожее на белый шум. Изучай и воспроизводи спектр шума лучше. DLS-вариант не терять,
но надо ещё лучше воспроизвести Титаник. Titanic-вариант у нас основан на косячном
анализе? Хочу проверить его в нормальном исполнении. В дропдауне нет 43."

### Noise study (`.scratch/noise-shape.mjs`, fixed FFT — bit-reversal was missing)
- **Apple DLS flute sustain has almost no breath**: steady noise −52…−55 dB rel h1
  (1-2 kHz band) on C5-G5, −70…−80 dB below C5 and above G5 (low/high readings are
  at the 16-bit render floor). No onset chiff — breath rises with the tone (~130 ms)
  and stays at a low, slightly low-passed level. Our old noise layer was a flat
  white bed at −38…−46 dB rel h1 → 10-40 dB too loud and too bright: the "пшик".
- Fix (in place, GM 73 `FluteDLS`): level = 0.0325·0.70^x · m(x), m(x) piecewise-
  linear (x = octaves over C4): peak ×0.43 at C5-G5, ×0.028 below, ×0.08-0.22
  above; cutoff lowered to min(2.5·f0, 1600 Hz). Result vs bank (1-2k band): key 72
  −50.9 vs −52.3, key 77 −51.1 vs −51.0, key 60 −73.8 vs −83, key 84 −73.1 vs
  −82 — hiss gone, tilt matches. Harmonics unchanged (±0.2 dB verification intact).

### Titanic retune (program 43 = NEW `FluteClean`; legacy `Flute` block unmapped)
- Answer to the listener: **yes, part of the old Titanic profile carried the buggy
  analysis**. Clean 41-key run of "Titanic 200 GM-GS v1.2.sf2" (fixed pipeline)
  vs the profile: C6 zone (82-84) was 7-21 dB too dark (h5 −56 vs measured −35!),
  C#4 h4 +10 dB off, G#5 h3 −5 dB, D5 h2 −3 dB. F#5/F#6 were already clean
  (±2-3 dB, re-validated). The old C6 numbers look exactly like the half-speed
  artifact (scan at k·f0 missed the real tone at k·f0/2 → ultra-dark readings).
- `FluteClean` now has all 8 real zones: C#4 (<63), E4 (63-65, h2 +3 dB! — was
  interpolation), B4 (66-71, was interpolation), D5, F#5, G#5, C6 (corrected),
  F#6. Verified per key vs bank: every zone within ±3 dB (key 60 h2..h12 deltas
  −0.3…+1.3, key 84 −1.6…+3.6, key 72 −2.7…+2.4). Envelope/vibrato kept from
  the old Flute; breath level kept, cutoff darkened to min(4·f0, 1700 Hz);
  CutoffFactory attack-open modifier carried over.
- Why a new instrument instead of editing the old block: the str_replace tool's
  matcher cannot see past ~line 675-680 of InstrumentLibrary.cpp (content below
  is unreachable; the FluteDLS noise fix was done in place after compressing the
  doc comment above it). The legacy `Flute` block (old profile) is now unmapped
  dead code; `FluteDLS` (fixed noise) stays at GM 73.

### UI
- Program 43 is now its own optgroup "Альтернативные флейты" in both instrument
  dropdowns (it was already in the list — buried at the end of "Флейты"; a hard
  refresh of the preview is needed to pick up the new synth.js).

### A/B rows refreshed (loudness-matched −20 dBFS, 2-13 s window)
1. **Наш · DLS** — GM 73 `FluteDLS`, new breath (md5 `be4c1b08…`)
2. **Наш · Titanic** — program 43 `FluteClean`, retuned profile (md5 `c87f3467…`)
3-5. Banks unchanged: Titanic / FluidR3 / gs_instruments.dls
Build md5 `b291b8a3…` staged in web/generated/ + dist/. Artifacts:
`.scratch/noise-shape.mjs`, `titanic-verify.mjs`, `ours_dls_new.wav`,
`ours_titanic_new.wav`.

## Update 15 — GM 73 retargeted to Apple DLS (voice verified ±0.2 dB per key); the "crushing" was a stereo-comb artifact of mono analysis, not the synth

### What shipped
- **GM 73 = new `FluteDLS` voice**, profiled from Apple's `gs_instruments.dls`
  (the system GM bank the listener likes; downloaded from public GitHub mirrors,
  md5 `94660977…`, NOT committable — Apple-proprietary). Six flat sample zones
  (A 60-69 … F 98+) with pairs of identical anchors, quick attack (~12-32 ms),
  no vibrato, ~15 ms release, breath noise layer shared with the old flute.
- **Old Titanic-profile flute kept** at program 43 (`Instruments["Flute"]`),
  reachable from the UI dropdown for the A/B "which is closer to a real flute".
- Earlier first-pass numbers (Update 13) were probe artifacts (SMF missing tempo
  meta → half-speed renders + broken WAV channel parse) and were discarded;
  the voice was retuned to the clean 41-key measurements (see Update 14 note in
  the source comment). A zone-B experiment (h2 1.12→0.89) made during debugging
  is **reverted** — final build md5 `326eda31…`, byte-identical to the
  pre-experiment corrected build.

### The stereo-comb discovery (why per-key verification looked "crushed")
The synth renders true stereo; the right channel reads the wavetable 344
samples later (`channelDelta = (sampleRate>>7) % tableLen`). Mono-averaging L+R
multiplies each partial by |cos(π·b·344/16384)| — zero when bin
b ≈ 47.6·(odd). Our probes downmixed to mono, so any partial landing near a
null read 10-40 dB dark, erratically per key (h3 at key 71 bin 550 ≈ null −17 dB;
h1 at key 80 bin 309 ≈ null −28 dB; etc.). **The voice was always correct** — the
L channel alone reproduces every zone within ±0.2 dB:

| zone | keys | h2/h3/h4/h5 measured (L) vs bank target |
|---|---|---|
| A | 60/66 | +3.0/−4.0/−4.0/−9.0 vs +3/−4/−4/−9 |
| B | 70-77 | +1.0/−2.0/−20.0/−29.0 vs +1/−2/−20/−29 |
| C | 80/84 | −7.0/−26.0/−25.0/−41.0 vs −7/−26/−25/−41 |
| D | 86/89 | −8.0/−19.0/−41.0/−42.0 vs −8/−19/−41/−42 |
| E | 93 | −20/−39/−46 vs −20/−39/−46 |
| F | 99 | −28/−51/−60 vs −28/−51/−60 |

Mono analysis scripts must measure one channel (or both separately) from now on;
`CH=L` mode added to `.scratch/single-note-keys.mjs` / `window-track.mjs`.

### A/B rows refreshed (cherilady spoiler)
- Rows now: **ours-DLS** (current GM 73, `cherilady_ours_15s.wav`, md5
  `04e1d4e6…`), **ours-Titanic** (the former GM-73 render kept at
  `cherilady_ours_titanic_15s.wav`, md5 `5f03e5a1…`), then bank rows Titanic /
  FluidR3 / gs_instruments.dls (unchanged content). The redundant pre-fix
  `ours_15s_prev` row/file was removed.
- All six rows **RMS-matched to −20 dBFS** (2-13 s window) so the listening A/B
  is not biased by the ~8-19 dB natural level gaps (fluidsynth renders hot).
- Description updated to point at the two candidates + three bank references.

Artifacts: `.scratch/single-note-keys.mjs`, `window-track.mjs`, `peak-dump.mjs`,
`inst-compare.mjs`, `table-replicate.mjs`, `render-piece-ours.mjs`,
`.scratch/ours_piece_new.wav`.

## Update 12 — Fair (dry) A/B of cherilady-flute + root cause: register zones were wrong; profile retargeted to real SF2 zone boundaries

Listener's earlier verdict: "our render has no reverb/effects but you did not turn
them off in the FL one — unfair comparison. Even under identical conditions the
sound is completely different — figure it out."

### Fair A/B in the debug spoiler

The two reference rows of `#cheriladySpoiler` were re-rendered **without** the
fluidsynth default reverb/chorus (`synth.reverb.active=0 synth.chorus.active=0`)
so all three rows are dry, same as our output:

- `web/cherilady_ours_15s.wav` — current build (md5 `065d4b54…`); the previous
  build render is kept as `cherilady_ours_15s_prev.wav` (md5 `221b2d0a…` =
  Update 11 build `80057765…`) for a was→now A/B in the same spoiler.
- `web/cherilady_titanic_15s_dry.wav` — fluidsynth + Titanic (dry).
- `web/cherilady_fluidr3_15s_dry.wav` — fluidsynth + FluidR3_GM (dry).
The old reverb-on renders were removed from `web/`/`dist/` (still in `.scratch/`
as `cherilady_{titanic,fluidr3}.wav`). `dist/` re-synced.

### What was measured (all dry, gain-matched, pre-vibrato note windows)

Per-key harmonic aggregates over every clean note of the 15 s piece
(`.scratch/cheri-zone-targets.mjs`) and 3 s holds (`flutehold_*.wav`,
`.scratch/hold-harm.mjs`), plus raw loop forensics per SF2 zone
(`.scratch/flute-zones2.mjs`). The pre-vibrato windows matter: our vibrato
(±3-6 c above C5) smears DFT peaks of h≥2 by 5-12 dB in windows past ~0.35 s,
so only onset windows (vibrato delay 0.2+ s) give clean spectra for both sides.

1. **Keys 76-81 (F#5/G#5 SF2 zones) — our h3 was +16…17 dB too bright**
   (rendered −5/−6 dB vs the bank's −22…−20 dB). Cause: the old 3-point
   C4→C5→C6 profile interpolated h3 linearly in amplitude between the C5 set
   (0.90) and the C6 set (0.006), so it only shed h3 near C6; the real bank
   switches to the much purer F#5(R)/G#5(R) samples at key 76 (loop h3 −24/−20).
   The same error made our h7/h8/h9/h10 tail ~+5…14 dB too bright there.
2. **Keys 85+ (F#6 zone) — our render was ~20…40 dB too dark.** The old
   profile clamped keys ≥ C6 to the "almost pure tone" C6 set (h3 −49 dB), but
   the F#6(R) sample the bank actually plays there is flute-like again
   (rendered h2 −31, h3 −20, h4 −33 dB). Update 4's C6 h3 −49 target came from
   a single tab window of the C6(R) loop; whole-loop and dry-render readings
   put C6(R) at h3 ≈ −30 and F#6(R) at ≈ −20.
3. **Keys 72-75 (D5 zone) — smaller but consistent errors:** our h4/h5 were
   5-8 dB dark and the odd h7/h9 airy tail ~8-10 dB bright vs the bank render
   (h4 −24, h5 −19, h7 −37…−39, h9 −41…−45 dB pre-vibrato).
4. **Release/articulation:** the reference flute rings into the inter-note gaps
   (SF2 volEnv release ≈ 0.55 s): gap RMS only −6.6 dB below note body vs
   −12 dB for ours (we cut release to 0.10 s in Update 8). The earlier
   "notes too long" complaint (Update 8) and this measured 0.55 s bleed are in
   tension; decision left to the owner (one-line knob: `Segments[4]` time in
   the flute `EnvelopeProfile`). Not changed in this pass.
5. **Intonation:** the bank renders sample-true (+6 c at C5/D5, +15 c at D#6);
   ours is equal-tempered. Small, not addressed.
6. **Short-note onset bloom** (reference short notes live inside the recorded
   pre-loop material and are brighter in h4/h5/h7) — the follow-up flagged in
   Update 10; still open, not reachable with a static table + uniform envelope.

### Change: flute register profile retargeted to real SF2 zone boundaries

`InstrumentLibrary.cpp` — the 3 reference sets (C4/C5/C6 with one linear ramp)
are replaced by sets anchored on the actual zone boundaries of the Titanic
bank (same Roland samples): C#4 (keys <63, unchanged), D5 (72-75), F#5 (76-78),
G#5 (79-81), C6 (82-84), F#6 (85+). Amplitudes come from the dry render
aggregates and whole-loop measurements; interpolation is piecewise-linear in
amplitude between adjacent anchors. The D5 zone is held flat across keys 72-75
(two identical anchors at keys 72/75) — otherwise the strong h6 of the C#4
zone profile bleeds onto C5/D5 through the ramp. Keys 63-71 (E4/G4/B4 zones,
not in the piece) still interpolate C#4→D5 as before.

New WASM md5 `de53926a…` (staged in `web/generated/` and `dist/`; smoke test
passes, 0 non-finite).

### Verification (pre-vibrato windows, rel h1, dB; ours → Titanic dry)

| key | h2 | h3 | h4 | h5 | h6 | h7 | h8 | h9 | h10 | h11 | h12 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 72 | −9/−8 | −1/−1 | −24/−23 | −20/−19 | −39/−40 | −36/−39 | −39/−37 | −42/−45 | −38/−37 | −44/−43 | −50/−50 |
| 74 | −9/−9 | −1/−1 | −24/−25 | −20/−18 | −38/−39 | −29/−32 | −38/−37 | −40/−41 | −39/−38 | −42/−41 | −48/−50 |
| 76 | −14/−14 | −22/−22 | −31/−31 | −28/−28 | −43/−43 | −47/−46 | −49/−49 | −53/−53 | −53/−55 | −55/−56 | −58/−58 |
| 77 | −15/−14 | −21/−22 | −33/−31 | −30/−28 | −43/−43 | −45/−46 | −49/−50 | −55/−53 | −56/−56 | −56/−56 | −58/−59 |
| 86 | −31/−31 | −20/−20 | −33/−33 | −46/−45 | −48/−49 | −47/−46 | −54/−51 | −57/−56 | −60/−58 | −58/−59 | −63/−60 |

The previous +17 dB h3 error at keys 76/77 and the ~25 dB darkness at key 86
are gone; every key now sits within ±3 dB of the dry reference (mostly ±1).
Keys 79-84 are interpolation anchors only (not played by this piece) and the
h4/h5 targets for keys 72-75 deliberately serve the short-note onset values
(the piece is 100% ≤150 ms notes) at the cost of ~3 dB brightness on held
notes, whose steady state still awaits the register-drift/bloom follow-up.

Artifacts: `.scratch/cheri-zone-targets.mjs`, `.scratch/hold-harm.mjs`,
`.scratch/flute-zones2.mjs`, dry renders `cheri_{titanic,fluidr3}_dry*.wav`,
`flutehold_titanic_dry.wav`, renders `cheri_ours_v2.wav` / `flutehold_ours_v2.wav`.

## Update 13 — New reference candidate: Apple macOS system GM bank (gs_instruments.dls), the "APlayMIDI" flute

Listener: the flute of a freshly installed macOS MIDI player (he calls it
"APlayMIDI"; no such app is indexed anywhere reachable — Google Play,
F-Droid, APKMirror, APKPure, App Store search, GitHub, Wayback — so the name
cannot be verified from here; it does not matter for the sound) is the best
flute he has heard; our banks + our synth make the flute "dull and
unnoticeable"; he wants all flutes retargeted to it. The app uses the OS
built-in synth (CoreMIDI/AUMIDIPlayer class of API), so the "soundfont" it
plays is Apple's system GM bank, not something the app ships.

### The bank

- **Apple `gs_instruments.dls`** — the macOS/iOS system GM bank (Roland-style
  samples; flute program 73 = samples `FLUTE65A/73A/79A/88A/96A/A5A`).
  Apple-proprietary: must NOT be committed to the repo.
- Downloaded from public GitHub mirrors (robovm/apple-ios-samples
  AVAEMixerSample; genedelisa/MIDIPlayer — byte-identical, md5
  `94660977369dd6298a16802f9ac3436a`, 1,996,068 B). Local copies:
  `.scratch/soundfonts/gs_instruments.dls` (workspace, gitignored) and
  `/tmp/sf2extract/` next to Titanic.
- Where the listener gets his own (exact-revision) copy on macOS:
  `/System/Library/Components/CoreAudio.component/Contents/Resources/gs_instruments.dls`
  (one `cp` command; file is readable from the read-only system volume).

### Measurements (dry fluidsynth render, per-key 3 s holds, `.scratch/appledls-flute.mjs`)

Probe calibrated against Titanic first: reproduces the documented h2/h3 and
onset/release numbers (e.g. key 75: h2 −14 / h3 −22 — matches the documented
F#5-zone values; release −5/−7/−11 dB at 20/40/80 ms). So the DLS readings
below are trustworthy:

- **Steady-state is much purer than ours/Titanic**: h2 −7 (C4) → −20 (F#5
  zone) → −33 (C#6) → −25 (G#6+) ; h3 −26…−41 across the whole range; h4
  −27…−49. Our profile (retargeted to Titanic) sits at h3 −1…−22 — i.e. our
  flute is far brighter in the odd harmonics than the Apple one.
- **Onset: soft breath swell, register-dependent** — 20 ms RMS rel plateau:
  −3…−8 dB (C4-F4), −8…−13 (F#4-F#5), −12…−21 (G5-C6), −16…−25 (C#6-G6),
  −6…−15 (C7+); full level by ~80-120 ms.
- **Vibrato: none measurable in the bank render** (≤3 c; the Titanic
  calibration shows real 4-17 c). Caveat: fluidsynth/libinstpatch may not
  apply DLS LFO vibrato — if the Mac's own synth sounds vibrato'd, that is a
  renderer difference, tell us the character and we'll add FM vibrato.
- **Release: instant** — 20 ms after note-off already −56…−65 dB, digital
  silence at 40 ms (Titanic rings 0.55 s). The Apple flute's articulation
  lives in the onset, not the tail.
- **Level rises with register**: −18.7 → −12.9 dBFS across C4..C7 (~+6 dB) —
  the flute climbs out of the mix in the upper register.

### A/B row added

4th row in `#cheriladySpoiler`: `cherilady_appledls_15s_dry.wav` (web/ +
`dist/`, md5 `94ae6b33…`, 15.002 s, dry, 20 ms fade — same recipe as the
other rows). Source: `.scratch/cherilady-flute-15.mid` through fluidsynth +
`gs_instruments.dls` with reverb/chorus off.

Open for the owner: (1) confirm row 4 is the Mac-app sound, (2) retarget
scope — Flute (GM 73) only or the whole flute family (pan flute/recorder/
piccolo/bottle/whistle), (3) whether the Mac version adds vibrato we should
model.

## Update 11 — cherilady-flute A/B renders in the debug spoiler (listening test)

Listener: timbre still "not it" on music even though single notes resemble the
tabs; the FL version of the same piece (cherilady-flute) sounds better. Test
requested: render the first 15 s of cherilady-flute and put it in the debug
spoiler, so we can decide whether the gap is the synth or the SF2 bank.

Artifacts (FL Studio is not available here, so the "FL render" is fluidsynth
— the same SF2 renderer the project uses for every reference A/B):

- `web|dist/cherilady_ours_15s.wav` — current synth build (md5 `80057765…`)
- `web|dist/cherilady_titanic_15s.wav` — fluidsynth + Titanic 200 GM-GS v1.2
  (the project reference bank, source of the sample tabs)
- `web|dist/cherilady_fluidr3_15s.wav` — fluidsynth + FluidR3_GM (second bank
  available on the machine)

Source: `.scratch/cherilady-flute.mid` (GM flute prog 73, single voice, ch 3,
keys D4-D7, 123 BPM), truncated to the first 15 s in the MIDI domain
(`.scratch/truncate-midi.py` → `.scratch/cherilady-flute-15.mid`, end tick
2952 = 15.000 s), rendered with default effects (reverb/chorus on). All three
are stereo 44.1 kHz 16-bit, ~15.0 s, cut with a 20 ms fade-out. New spoiler
block `#cheriladySpoiler` in `web/index.html` (synced to `dist/`) with one
`<audio>` row per version. Note: the first ~1-2 s of the piece are a quiet
intro, the music starts later.

## Update 10 — Recorded onset: per-register two-stage tone envelope (attack + swell)

After the SF2 forensics of Update 9 the listener approved measuring the real
attack profiles and modelling the recorded onset instead of a plain ADSR ramp.

Measurements (`.scratch/flute-onset-trajectory.js`, 40 ms windows/20 ms hop on
`web/generated/samples/Flute/{C4,C5,C6}.wav` vs our render):

- RMS rel to the note's own plateau: the reference rises fast to roughly half
  level and then **swells** the rest — full by ~0.2 s on C4 (−19.9 dB @20 ms,
  −9.4 @50, −6.3 @80, −2.8 @120, −0.7 @180), ~0.12 s on C5, ~0.05 s on C6.
  Our build (Update 8, 60 ms linear attack) was at full level by ~60 ms and
  flat afterwards (+2.3 dB env 0.3-0.5 s rel head vs sample +8.5 dB) — the
  static "dead" onset.
- The first ~10-15 ms of each reference note are near-silent (breath leads
  the tone) — most audible on C5/C6.
- Partial-ratio brightness also blooms at C4 (h2/h3/h5 rise ~10-14 dB rel h1
  over the onset) — not reachable with a uniform-gain envelope + one-pole
  filter; flagged as a separate follow-up, the amplitude swell is the dominant
  measured cue and is what was implemented.

Changes:

1. **`WaveTableSampler.h/.cpp`** — new per-note hook `EnvelopeProfile`
   (`Funal::CopyableDelegate<EnvelopeFactory(float freq)>`), exact mirror of
   `VibratoProfile`: if set, `WaveTableInstrument::operator()` builds the
   voice envelope from `EnvelopeProfile(freq)` instead of the fixed
   `Envelope`. Zero impact on instruments that do not set it.
2. **`InstrumentLibrary.cpp` (Flute)** — `wt.EnvelopeProfile` per register
   (x = octaves over C4, piecewise-linear C4→C5→C6), 5-segment envelope
   reusing the factory's spare slot as a flat-zero lead:
   - `Segments[0]`: flat 0 for D0 ≈ 8-13 ms (tone does not click before the
     breath);
   - `Segments[1]`: fast linear rise 0→V1 (C4 .42 @50 ms, C5 .70 @28 ms,
     C6 .80 @24 ms) — keeps short-note articulation;
   - `Segments[2]`: exponential (linear-in-dB) V1→1 swell, C4 ~0.16 s, C5
     ~0.04 s, C6 ~0.02 s — the recorded "breath" on held notes;
   - sustain 1; release 0.10 s exponential (unchanged).

Measured after (same trajectory probe, ours rel steady):

| t | C4 ref → ours | C5 ref → ours |
|---|---|---|
| 20 ms | −19.9 → −16.9 | −14.1 → −10.0 |
| 50 ms | −9.4 → −7.7 | −3.4 → −1.6 |
| 80 ms | −6.3 → −6.5 | −0.9 → −0.4 |
| 120 ms | −2.8 → −4.2 | −0.1 → 0.0 |
| 180 ms | −0.7 → −1.6 | +0.7 → 0.0 |

`ab-flute.js` env 0.3-0.5 s rel head now +9.2 dB on C4 (sample +8.5; was
+2.3) and +1.6 on C6 (sample +1.3). Sustain spectra, noise floor and release
unchanged (no regression). 16th-note phrase check (old vs new build): peaks
−29 → −36 dB, gaps −32 → −38 dB — short notes are ~7 dB quieter than the
instant-onset build *because they follow the recorded onset* (reference C4 is
≈ −6 dB below its plateau at 90 ms too), and inter-note separation got
cleaner. If staccato feels too quiet, the knobs are V1/T1 at low register.

Smoke test passes (0 non-finite). dist == web/generated (IntraSynth.wasm md5
`80057765…`).

## Update 9 — SF2 forensics: what the Flute preset actually is (analysis, no code change)

Listener asked: does the reference SF2 have a real Flute program, does any
metadata reshape the attack at render time, and why does another (unknown)
synthesizer sound far livelier while our steady-state render matches the
sample tabs.

Findings from parsing `/tmp/sf2extract/Titanic 200 GM-GS v1.2.sf2`:

- GM 73 Flute = instrument "Flute" (#108), a **pure looped-sample voice**:
  9 key-split zones over the Roland (R) flute samples C#4/E4/G4/B4/D5/F#5/
  G#5/C6/F#6 (shdr ids 687-695). No separate attack/release samples, no
  chiff/breath layer in this preset (a "Chiff Flute" sample exists elsewhere
  in the bank but is not referenced by program 73).
- shdr shows every sample is a full recorded note: real onset material sits
  **before** the loop point (C#4(R): 0.91 s total, 0.66 s pre-loop + 0.25 s
  loop; E4(R): 0.98 s / 0.73 s + 0.25 s; C6(R): 0.74 s / 0.51 s + 0.23 s;
  F#6(R): 0.68 s / 0.43 s + 0.25 s; loops 0.20-0.44 s, type=1 continuous).
- Preset metadata is deliberately plain. 9 velocity layers (bands up to
  121-127) all point at the **same** instrument: velocity only raises
  reverbSend 70→150 and a per-layer envelope-delay gen; it never switches
  samples or articulation. Instrument zones differ only in keyRange,
  initAtten (−10 dB on the C#4 zone covering keys 0-62), a ~11.9 kHz
  initFilterFc lowpass on all zones, a tiny 6.95 Hz amplitude LFO with
  200 ms delay, exclusiveClass per zone, and sampleModes=loop.
- Consequence: an SF2 note-on plays the sample **from its start**, so the
  heard attack *is* the recording's own onset (up to ~0.4-0.7 s of evolving
  tone before the loop), not an ADSR. For this preset there is effectively no
  "attack metadata" — the sample start is the attack.
- The reference tabs (`web/generated/samples/Flute/*.wav`) are the same raw
  samples decoded start→end (onset + loop) and transposed to the label via
  measured TRUE_PITCH {60:61, 72:74, 84:84}, so tabs and a direct SF2 note
  share the same onset.

Why an unknown other synth may sound "an order of magnitude livelier"
(candidates, in order of likelihood; none verifiable without that bank):
1. different samples (a bank with real velocity/articulation layers or
   livelier recorded loops); 2. reverb/chorus — the Titanic flute is nearly
   dry (reverbSend 70-150); 3. our synthetic attack vs the recording's own
   onset (steady-state spectrum matches the loop, the transient is where we
   diverge).

## Update 8 — Attack/release shaping across the flute family (articulation in fast music)

Listener: "Атака и note off вообще не похожи — все флейты бесформенные в
музыке и намного более длинные, чем в других синтезаторах".

Diagnosis (A/B phrase render, same MIDI through our wasm vs fluidsynth
reference; 50 ms RMS rows):

- Our flute tone attack was a **150 ms linear** ramp from silence. On melodic
  notes shorter than ~150-200 ms (16ths/staccato) the tone never bloomed
  (16th peaks only reached −30 dB) and the passage rendered as a flat
  −33 dB wash with no per-note articulation; the reference reaches ~−6 dB
  within the first ~50 ms and each 16th is a distinct −24…−35 dB pulse.
- The ADSR release stage was hard-coded **linear** in `EnvelopeFactory::ADSR`
  (``{false, 0, releaseTime}``), so a note-off faded at constant amplitude
  and every note audibly "hung" at near-full level until the final
  milliseconds — the linear fade that makes notes feel longer than a
  sample-based reference with an exponential tail.

Changes:

1. **`Envelope.h` — ADSR release honors the exponential flag.**
   `Segments[N-1] = {exponential, 0, releaseTime}` (was always `false`).
   Exponential release kills the onset of the fade faster (SF2/fluidsynth
   style) and tails off softly; from zero volume it cannot start (release is
   only entered on NoteRelease, never for already-released voices).
   `Envelope::Point::CalcDU` clamps a zero start volume to the 1/256
   asymptote so an exponential segment can never divide by zero.
   Instruments that already set `Exponential=true` (reverse cymbal,
   applause/helicopter/seashore pads, phone ring) inherit the exponential
   release; everything else is untouched (flag stays false).
2. **`InstrumentLibrary.cpp` — tone + noise envelopes of the flute family**
   (attack shortened so short notes bloom; release shortened and made
   exponential so note-off doesn't drag):
   - Flute (GM 73) tone: attack `0.15 → 0.06 s`; release `0.12 → 0.10 s` exp;
     `breathEnv` (noise): attack `0.03 → 0.025 s`, release `0.09 → 0.07 s`
     exp (chiff still leads the tone).
   - Pan flute (GM 75): attack `0.05 → 0.04 s`, release `0.10 → 0.08 s` exp;
     `puffEnv` release `0.06 → 0.05 s` exp.
   - Recorder (GM 74): attack `0.04 → 0.035 s`, release `0.08 → 0.06 s` exp;
     `chiffEnv` release `0.06 → 0.05 s` exp.
   - Piccolo (GM 72): release `0.06 → 0.05 s` exp (attack 0.02 s kept).
   - Bottle (GM 76): attack `0.06 → 0.05 s`, release `0.08 → 0.06 s` exp;
     `blowEnv` release `0.06 → 0.05 s` exp.
   - Whistle (GM 78): release exponential (0.02 s kept).
3. **Flute onset-brightening sweep tightened.** The leftover
   `CutoffFactory(200, 20000, 20000, 100, {0.07s attack})` modifier (from the
   web-midisynth spec section) low-passed the whole Flute voice from **200 Hz**
   upward over 70 ms at every note-on — with the 150 ms amplitude ramp this
   muffled onsets even more. Sweep now starts at 600 Hz and opens in 35 ms
   (`CutoffFactory(600, 20000, 20000, 100, {0.035s, 0.05s decay})`).

Measured after (same phrase, 50 ms RMS; 16ths at 3.2-3.95 s): note peaks
reach −25 dB (was −30) and inter-note dips are −33…−38 dB (was a constant
−33 dB wash), matching the reference articulation pattern (−24/−25 peaks,
−32/−35 dips). Release after the held note ends ~50 ms sooner.

Smoke test passes (0 non-finite). dist == web/generated (IntraSynth.wasm md5
`66d45575…`). Reference tabs live in `web/generated/samples/Flute/{C4,C5,C6}.wav`.

Family-wide re-check (same 16th-run phrase per GM program 72/74/75/76/78,
vs the fluid render of the same files): every program renders 0 non-finite
samples, and the 16th peaks land on the notes with inter-note gaps −46…−56 dB
(piccolo/recorder/bottle) — i.e. each family voice now separates fast notes
instead of smearing. Our gaps are deeper than fluidsynth's (−20…−39 dB):
that is the intended direction ("notes no longer longer than other synths"),
and is the knob to relax if the tail feels too dry on long notes.


## Update 7 — Breath-noise recipe: attack ÷4, air bed ÷2 (flute AND pan flute)

Listener pass on the ÷5 flute noise (Update 6): attack noise is now fine, but
the sustained air bed got too quiet — request was "attack ÷4 from the
original, air bed only ÷2 from the original; same for pan flute". Applied in
`InstrumentLibrary.cpp`:

- Flute noise (was 0.13 → 0.026 at C4 after Update 6): level back up to
  `0.0325` (= 0.13/4, i.e. attack ÷4 from the original) and `breathEnv`
  Sustain `0.30 → 0.60` so the bed = 0.0325·0.60 = 0.13·0.30/2 (only ÷2 vs
  the original, not ÷5). Register scaling ×0.70/octave and the envelope
  shape are unchanged.
- Pan flute puff (scale 0.06, `puffEnv` Sustain 0.10 — untouched by
  Update 6): scale `0.06 → 0.015` (attack ÷4 from its own original) and
  Sustain `0.10 → 0.20` (bed = 0.015·0.20 = 0.06·0.10/2, ÷2). Same envelope
  shape and cutoff `min(4·f0, 1600 Hz)` kept.

Measured with `.scratch/probe-breath-ab.js` (inter-harmonic 10th-percentile
floor in an attack window and a late sustain window; before = Update 6 build
`471da669`, after = `b5ed6883`):

| instrument | window | before | after | delta | theory |
|---|---|---|---|---|---|
| Flute C4 | attack | −84.6 | −82.4 | +2.3 dB | +1.9 (÷4 vs orig.) |
| Flute C4 | bed | −97.3 | −89.0 | +8.3 dB | +8.0 (÷2 vs orig.) |
| Pan flute A3 | attack | −75.3 | −88.1 | −12.8 dB | −12.0 (÷4) |
| Pan flute A3 | bed | −96.4 | −101.4 | −5.0 dB | −6.0 (÷2) |

Smoke test passes; dist re-synced (IntraSynth.wasm md5 `b5ed6883…`).

**Provenance corrections (owner review of Update 5/6 wording):** the whole
`intrasynth/` module — including the native `WaveTable`/`WaveTableSampler`/
`Envelope.h` — arrived in this repo in one commit (`6c2ed03 "Add intrasynth
synth module"`), so there is no older in-repo history to attribute it from;
"IntraSynth already had WaveTable" is correct. What the renamed-away `Web*`
prefix actually marked is the *JS-faithful building-block layer* declared in
`Synth.h`'s header comment ("JS-faithful building blocks ported from
devoln/web-midisynth (instruments.js, wavegen.js, utils.js): wavetable
generator, envelope mapping, noise sampler, two time-varying filters") — the
Web* names meant "matches the web JS". Also: no legacy Intra-master ADSR is
in play. The wavetable tone + noise voices run on the native `Envelope`
(`WaveTableSampler` holds `Envelope mEnvelope`; `NoiseSampler` builds it via
`EnvelopeFactory::ADSR(...)`); `EnvelopeDesc` (declared in `Synth.h`) is only
a compact 7-field descriptor that `MakeEnvelope`/`Wt()` immediately converts
into that real table-based `Envelope` (per-segment linear/exponential, the
superset envelope the owner prefers). `Intra/Audio/Synth/ADSR.{h,cpp}` (Intra
master's legacy synth) is not referenced anywhere under `intrasynth/`.

## Update 6 — Flute: breath-noise level ÷5 (attack too noisy on fast short notes)

Listener: "в атаке слишком громкий шум — в быстрых мелких нотах кроме него
ничего не слышно; в оригинале его почти нет или он очень тонкий" and asked to
try making it ~5× quieter in the attack. Lowered the flute's whole noise-layer
level by ÷5 (0.13 → 0.026 at C4, register scaling ×0.70/octave kept) in
`InstrumentLibrary.cpp`. Measured with an inter-harmonic noise-floor probe on
the pre/post wasm builds: uniform −14.0 dB across every window 0–0.6 s at C4 and
C5 (attack 0–90 ms: −67.8 → −81.8 dBFS). Smoke test passes; dist re-synced
(md5 `471da669…`). The envelope shape (`breathEnv` attack/decay/sustain) is
unchanged, so the sustained air bed dropped with it — if long notes now feel too
dry, raise `Sustain` in `breathEnv` (that restores only the bed, not the
attack).

## Update 5 — "Web" prefix purge in the wavetable synth (pure rename, no behavior change)

The wave-table subsystem was still full of `Web*` identifiers left from when it
was ported out of devoln/web-midisynth (the prefix meant "matches the web JS").
The owner had already ordered them eradicated earlier; the last stragglers
lived in `InstrumentLibrary.cpp` (pan flute / piccolo / recorder / bottle /
whistle-family blocks + the noise-instrument section). Renamed to the
plain names now declared in `Synth.h` (the renamed `WebSynth.h`):

- `WebSynth.h` → `Synth.h` (file move + include fix)
- `WebHarmonicDesc` → `HarmonicDesc`, `WebHarmonicSet` → `HarmonicSet`
- `WebEnvelope` → `EnvelopeDesc`, `MakeWebEnvelope` → `MakeEnvelope`
- `WebBuildWaveTable` → `BuildWaveTable`, `CreateWebWaveTables` → `CreateWaveTables`
- `WebNoiseSampler` → `NoiseSampler`, `WebNoiseInstrument` → `NoiseInstrument`
- `WebResonanceDesc` → `ResonanceDesc`, `WebCutoffFactory` → `CutoffFactory`,
  `WebExpExpModifierFactory` → `ExpExpModifierFactory`, `WebVibrato` → `Vibrato`
  (done in an earlier pass in `WaveTableSampler.h/.cpp`/`ComputeKernels.h`)

The only remaining "Web" mentions in `intrasynth/src` are prose comments about
the browser WebAudio / Web MIDI APIs (and the source project name
web-midisynth) — legitimate text, not code identifiers. Verified: clean
emscripten build (zero errors), `scripts/smoke-intrasynth.js` passes
(`SMOKE TEST PASSED`, 0 non-finite samples), dist re-synced from
`web/generated/`. The `Wt()` helper is a local factory that fills a
`WaveTableInstrument` from a `WaveTableCache*` + volume/exp/env/vibrato — the
shorthand predates the Web era and stays.

## Update 4 — Pan flute (GM 75): per-register timbre, no vibrato, capped breath

User asked earlier to make pan flute a *distinct* instrument ("разберись, чем
отличается этот инструмент и сделай правдоподобно") — the fixed-table version
had the same two latent problems the flute pass just fixed: always-on fixed
vibrato (5 Hz / ±0.4 % from t=0) and an uncapped noise cutoff (4·f0 — at G4+ it
opens to 1.6-3 kHz of white over a pure tone), plus a static harmonic table
whose rendered h3 stayed ~−10 dB in registers where the real instrument is far
purer.

Measured the real SF2 samples (`panflute a3/d4/g4`, sample IDs 1998-2000;
loops, originalPitch A4/D5/G5 → timbre transposes onto zones A3..G4):

- A3: h2 ≈ −28, h3 ≈ −6, h5 ≈ −18, h7 ≈ −29, h9 ≈ −46 dB rel h1 — strong
  odd-partial stopped-pipe stack.
- D4: h2 ≈ −37, h3 ≈ −15 (loop has heavy frozen pitch smear ±10 Hz — readings
  uncertain by several dB), h5/h7/h9 in between.
- G4: nearly pure — h2 ≈ −28, h3 ≈ −25, h5 ≈ −45, h7 ≈ −50 dB.
- Noise floor 2-8 kHz ≈ −90…−96 dB rel h1 (essentially clean loop).

Changes in `InstrumentLibrary.cpp`:

1. **Per-register harmonic profile** — the fixed `panFluteH` table is replaced
   with a custom `WaveTableCache::Generator` (same pattern as Flute): three
   reference sets A3/D4/G4 with log-linear amplitude interpolation between
   them, clamp outside the range. Tuned by render-and-measure iteration
   (`.scratch/probe-panflute-ab.js`, Hann window + peak picking — the pipeline
   renders table harmonics through a frequency-dependent transfer that must be
   absorbed empirically; fixed-frequency DFT readings are unreliable because
   the table bin quantization detunes each partial). Final rendered match:
   A3 within ±2 dB (h3 −4.6 vs −5.7, h5 −18.7 vs −17.5, h9 −46.4 vs −45.5),
   D4 within ±2.5 dB (h3 −12.5 vs −14.9), G4 within ±2 dB (h3 −26.9 vs −25.3,
   h5 −47 vs −45.3, h7 −51.4 vs −49.7). Residual D4 h6/h8 (+6 dB at
   −50…−60 dB rel h1) are inaudible and the D4 sample itself is wobbly.
2. **Vibrato removed** (`Wt(...)` without the 5 Hz / 0.4 % args) — pan flutes
   don't vibrate (no LFO in the SF2 loop, unlike flute). Fixed sine from t=0
   would repeat the "механически" complaint; its absence is also the main
   audible difference from the flute.
3. **Noise cutoff capped** — noise is now a per-freq `GenericInstrument`
   lambda with `min(4·f0, 1600 Hz)` (like Flute's 1.9 kHz cap), level 0.06
   and the short puff envelope kept (0.02 s attack chiff, 0.14 s decay, 0.10
   sustain bed). High octaves no longer rustle; the attack puff stays.

Verification: `scripts/smoke-test-wasm.mjs` passes; loudness at A3/D4/G4
−26.8/−24.7/−20.0 dBFS (≈ previous, within ~2 dB); dist == web/generated
(IntraSynth.wasm md5 `cb9fbdd9…`).

## Update 3 (evening) — listening pass 2: "вибрато слишком частое, должно
появляться после атаки; на C4 слышно, на C5 нет; выше октава — хуже шуршит"

New per-register design instead of the fixed 6.5 Hz / ±0.9 % sine from t=0:

1. **Vibrato onset gate (sampler level).** `WaveTableSampler` gained
   `mVibratoDelaySamples` / `mVibratoRampSamples` + `mElapsedSamples`; the
   depth fed to the FM oscillator is multiplied by `VibratoGate()`
   (`0 → linear 0..1 over [Delay, Delay+Ramp] → 1`). Both vibrato kernels in
   `ComputeKernels.h` now take `vibGate`/`vibGateStep` (default 1/0 — all
   fixed-vibrato instruments unchanged). Oscillator phase keeps running from
   note start, so the onset is coherent (no phase jump).
2. **Per-register vibrato profile.** New `WebVibrato {Frequency, Value, Delay,
   Ramp}` + optional `WaveTableInstrument::VibratoProfile` delegate (used by
   `WaveTableInstrument::operator()`, which knows the note frequency); flute's
   `Wt(...)` call no longer passes a fixed vibrato — instead:
   - rate: 5.2 Hz (C4) → 4.8 Hz (C5) → 4.6 Hz (C6) — slower than 6.5,
     "несколько раз в секунду";
   - depth: ±13 c (C4) → ±6 c (C5) → ±3 c (C6) — matches "на C5 не особо";
   - onset: 0.28 s delay after note-on (attack is 0.15 s) + ~0.15 s ramp-in
     (fully developed by ~0.44 s on C4), shorter on higher notes.
   Instrument-side foundation for the sample analysis: analytic-signal
   per-partial tracking of the tab h1 shows the fundamental moves only
   ±3-5 c at ~2-4 Hz (earlier whole-band zero-cross numbers ~7 Hz/20+c were
   crossing-count artifacts) — hence gentler depths than the first pass.
3. **Register-scaled breath noise.** Flute noise is now a per-freq
   `GenericInstrument` lambda instead of a fixed `WebNoiseInstrument`: level
   `0.13 × 0.70^octavesAboveC4` and cutoff `min(5·f0, 1900 Hz)` — the air band
   no longer opens to 2.6/5.2 kHz on C5/C6 (that white hiss over a pure tone
   was the "шуршит пакетом"); C4 settings unchanged vs the approved pass.

Verified on the rebuilt WASM (`probe-vibro-ours.js`, h1 phase tracker):

| note | onset/ramp (bins 100 ms RMS c) | steady bins | rate |
|---|---|---|---|
| C4 | 3→6→10 by 0.4-0.5 s | ~8-11 c alternating (5.5 Hz period ≈ 2 bins) | 5.5 Hz |
| C5 | ramps by ~0.35 s | ~2-7 c | 4.7 Hz |
| C6 | ramps by ~0.3 s | ~2-3 c | 4.7 Hz |

Smoke test passes. dist == web/generated (md5 `666e87db…`).

## Original entry — vibrato, per-register spectra, breath noise

User A/B report against the SF2 tab WAVs ("Flute C#4(R)"/"Flute D5(R)"):

- "У семпла флейты вибрато — звук меняется несколько раз в секунду, а у нас
  ровный звук" (sample vibrates ~7 Hz / ~10–24 cents; ours was dead steady).
- "У нас какой-то шелест целлофанового пакета, практически белый по ощущениям.
  А у семплов он очень далёк от белого шума — привязан к самим гармоникам"
  (our 4–16 kHz noise plateau vs the sample's harmonic-attached air).
- Earlier: C5 sample is bright/overblown (h3 ≈ h1), ours was a pure tone;
  C4 fine-ish through the mid partials but with the intrusive noise on top.

## Part 1 — Measurement pitfalls first

- **Octave "bug" was a probe bug**: the WASM pull buffer is channel-major
  (`[L block][R block]` — `web/synth.js` reads `outL[i]=heap[off+i];
  outR[i]=heap[off+n+i]`). Probes that read `f[2i]+f[2i+1]` decimated a channel
  by 2 and faked a one-octave-up render on *every* wavetable instrument
  (confirmed against the reference build). Fixed decoders in all flute probes.
- Render must be pulled in ≤4096-sample chunks; one 132 300-sample request
  renders silence through `SourceCreateFromMidiFileData`.
- Sample vibrato measured with per-window f0 tracking (`.scratch/probe-flute-fm.js`):
  C4 ≈ 7 Hz / up to ~20–24 cents, C6 ≈ 7 Hz / ~10 cents, C5 in between.

## Part 2 — Root causes

1. **Vibrato never reached audio.** `WaveTableSampler` seeded
   `mFreqOscillator(vibratoValue, 0, 2π·f0/sr)` but `renderDirect()` always
   rendered at the constant `mRate` (SIMD kernels with a fixed per-sample step);
   the "varying-rate" `Generate()` path was a stub. So `Wt(... vibrato 6.5f,
   0.009f)` did nothing audible.
2. **White noise bed.** The flute noise instrument had a flat, bright bed:
   measured 4–8 kHz / 8–16 kHz ≈ −59/−63 dB rel h1 while the sample sits at
   −70/−78 — that wide flat plateau is the "cellophane". The sample's air is
   concentrated *in the harmonics* (C4 keeps rolling partials h8…h12 at
   −18…−42 dB = an ~2–3 kHz chiff presence band) with almost nothing above.
3. **Register profiles were wrong.** C5 (overblown, h3-dominant, tail to ~h9)
   and C6 (nearly pure: h2 −29, h3 −49 dB) were not captured; C4 tail was too
   dark above h8.

## Part 3 — Changes

1. **`ComputeKernels.h`**: added `MultiplyAddVibrato` / `MultiplyAddVibratoStereo`
   — read rate is `baseRate·(1 + vibrato.Next())` per sample (FM), same
   exp/lin envelope stepping and linear interpolation as the constant-rate
   kernels; mono variant folds channel gain into `lin`. SIMD not applicable
   (rate is not constant).
2. **`WaveTableSampler.h/.cpp`**: new `mHasVibrato`
   (`vibratoValue != 0 && vibratoDeltaPhase != 0`) picked up in the ctor;
   `renderDirect()` dispatches to the vibrato kernels when set, keeping the
   existing SIMD path untouched for all non-vibrato instruments.
3. **`InstrumentLibrary.cpp` — Flute**: measured per-register harmonic sets
   (C4/C5/C6 reference points) with log-freq interpolation between them, from
   steady-sustain windows of the tab loops (`.scratch/probe-flute-windows2.js`):
   - C4: h2 −5, h3 −6, h4 −19, h5 −10, h6 −7, h7 −23, h8 −18…−24,
     h9 −30, h10 −28, h11 −39, h12 −42 dB (airy harmonic-bound tail kept).
   - C5: h2 −12, **h3 ≈ −1** (flute overblow), h4 −31, h5 −29, h7 −37,
     h8 −48, h9 −41.
   - C6: nearly pure — h2 −29, h3 −49, h4 −48.
   Vibrato 6.5 Hz / ±0.9 % (±15 c) wired through `Wt(...)`. Noise instrument:
   level 0.21 → 0.13, cutoff stays tied to the tone at 5×f0, attack chiff
   (0.28 s swell) kept for the breath onset.
4. **Pan flute (GM 75)** stays a separate voice (stopped-pipe odd partials,
   h3 −10 dB, lighter vibrato 5 Hz / 0.4 %, small attack puff) — unchanged this
   pass.

## Part 4 — Verification (dist rebuilt 2026-09-05, IntraSynth.wasm md5
`2e7e55241cfb892284d75ab8d0de2cad` = web/generated = dist)

- Vibrato confirmed rendering: per-window f0 tracker reads steady FM at the set
  6.5 Hz rate with ~±15 c depth (`.scratch/probe-vibcheck.js`); sample reads
  ~7 Hz. Octave verified correct: C4 ≈ 261 Hz, C5 ≈ 523 Hz, C6 ≈ 1047 Hz.
- C4 spectrum medians now within ~3 dB of the sample through h12 (was −8…−10
  dB dark at h8/h10); C5 h2…h11 all within 0…2 dB (h5 fixed −23→−29, tail
  h7/h8/h9 −49/−52/−52 → −38/−47/−42 vs sample −37/−48/−41); C6 h2 −29
  (was −17), h3 −48 (was −32) — the "слишком чистый" C5/C6 and bright C6
  fixed.
- Noise: 4–8 kHz −59 → −66, 8–16 kHz −63 → −70 dB rel h1 at C4 (sample
  −70…−78); 8–16 kHz at C5 −62.6 vs sample −68.2 (still ~5 dB high — the
  residual knob is the noise sustain level/cutoff).
- `scripts/smoke-test-wasm.mjs` passes on the staged build.
- `.gitignore`: added `node_modules/`, scratch/env noise (the "364k
  uncommitted lines" was an untracked playwright install).

Knobs for the listening pass: vibrato depth/freq are fixed per instrument in
`Wt(...)` (register-dependent depth would need a per-note param); noise floor
residual at C5 8–16 kHz; chiff-attack loudness on short repeated notes.

## Update 20 — compact Q16 flute profiles + per-zone level alignment (table compression, −17.5 KB)

### What changed (`InstrumentLibrary.cpp`)
All 20 active flute zone profiles (FluteClean ×8, FluteDLS ×6, legacy Titanic
"Flute" ×6) converted from `HarmonicDesc` float arrays to `uint16` Q16
amplitude arrays (0..2.0 in 1/32768 steps) unpacked once at init via
`Harms16(SpanOf(...), volumeScale)`. `volumeScale` carries the NEW per-zone
level alignment: each zones

## Update 20 - compact Q16 flute profiles + per-zone level alignment (table compression, -17.5 KB)

### What changed (InstrumentLibrary.cpp)
All 20 active flute zone profiles (FluteClean x8, FluteDLS x6, legacy Titanic
"Flute" x6) converted from float HarmonicDesc arrays to uint16 Q16 amplitude
arrays (0..2.0 in 1/32768 steps), unpacked once at init via
Harms16(SpanOf(...), volumeScale). volumeScale carries the NEW per-zone level
alignment: each zone's gain was fit so measured RMS lands on the least-squares
dB-vs-octave register trend of that flute (previously each profile's spectral
shape changed absolute RMS at every zone root, giving 1.2-2.1 dB steps at zone
boundaries). The original banks have the same per-zone level behavior - our
earlier measurement was correct, not mis-normalized; we are deliberately
smoothing it toward the register trend.

Zone gains (x, relative to trend): Clean C4 0.9425, E4 1.0466, B4 1.0821,
D5 1.0337, F5 0.9038, G5 0.9728, C6 0.9650, F6 1.0683 (trend -25.44 dB +
3.20 dB/oct). DLS A 0.9981, B 1.0355, C 0.9311, D 1.0613, E 0.9642, F 1.0155
(trend -24.75 dB + 2.12 dB/oct). Within-zone flatness +/-0.05 dB preserved;
residual boundary steps 0.8-1.6 dB span several semitones and are the
intentional "sample-switch step" of the bank layout (MixZoneSets pairs
duplicate sets on zone edges).

### Size + verification
- WASM (size build): 197.0 -> 179.6 KB (-17.5 KB). Profiles are now 24 B/zone
  of uint16 data instead of 48 B of float HarmonicDesc with relocations.
- scripts/build-wasm-size.sh builds clean; .scratch/flute-rms-audit.mjs re-run
  for programs 43/73/115: all zone gains land within +/-0.03 dB of design
  (post-audit in .scratch/flute-rms-audit-after.out).

## Update 21 - PanFlute/Whistle in the audit; cherilady renders moved to gitignored web/renders; 2-s sustained-note timbre A/B

- Per-key RMS audit (.scratch/flute-rms-audit.mjs) extended to programs 75
  (PanFlute) and 77/78 (Whistle); audit now covers 43/73/115/75/77.
- All cherilady A/B renders moved from web/ to web/renders/ (new .gitignore
  entry /web/renders/ - they were untracked WAVs cluttering the player dir).
- New timbre A/B probe .scratch/flute-family-ab-timbre.mjs: 2-second
  sustained notes (window 0.6-2.0 s, past attack), our synth vs Titanic SF2
  vs Apple DLS via fluidsynth, dry. Replaces the short-sample comparisons.

Findings (steady-window spectra, rel h1, dB):

PanFlute (75), keys 60/67/74:
- Titanic panflute samples are also dull/weak: h2 -29..-39, h3 -4..-16,
  h5..h8 mostly -23..-67, noise 2-8k about -48..-60. Our render is comparable
  in brightness (h2 -27..-51, h3 -14, h5 -7 at key 60 - actually BRIGHTER on
  h5), so "too dull vs DLS" is NOT us copying Titanic badly: DLS is even
  cleaner/duller on h2/h3 (-37/-27 at key 60) but has near-zero noise
  (-77..-79). The main difference from DLS is its absence of air noise and
  slightly stronger h2. Titanic and ours both carry a 2-8k noise bed around
  -48..-60 dB.
- Conclusion: our PanFlute matches the Titanic character; if DLS-like is the
  target, the knobs are: raise h2 by ~6-9 dB, cut noise 2-8k by ~15-20 dB.

Whistle (77), keys 60/67/74:
- Ours is clearly wrong-shaped vs both banks: at key 67 h2 is -51 (banks:
  -24 Titanic / +5 DLS!), h3 +5.8 (banks: -25/-47); at key 60 h4 -18 vs
  -33/-9. DLS whistle is a strongly odd-harmonic ("hollow") spectrum with h2
  ABOVE h1 at some keys; Titanic's is more balanced. Ours is also fully
  noise-free (-240 in 2-8k) while both banks have -58..-75 dB noise beds -
  that is a big part of the "artificial" impression.
- Conclusion: whistle needs re-tuning to DLS profiles (strong h2/h3, weaker
  h1-relative balance varies per register) plus a small breath-noise layer.
  Titanic copy is faithful to Titanic but Titanic itself sounds different
  from DLS here.

## Update 22 - Woodwind tabs as 2-s dry renders; cherilady renders into web/generated/samples; PanFlute rows in the spoiler

Listener feedback: flute samples in the "Сырые сэмплы" panel were too short to
compare timbre; PanFlute/Whistle were missing; the cherilady A/B renders
should live in the existing samples folder (web/generated/samples), not a new
web/renders/ dir.

- scripts/generate-sf2-samples.js now has a RENDER_2S set (Flute/PanFlute/
  Whistle): those tabs are 2-second DRY renders of the Titanic SF2
  (fluidsynth, `-o synth.reverb.active=no -o synth.chorus.active=no`,
  note-on 0.2 s / note-off 2.2 s = 2 s hold, vel 100) instead of raw
  extracts. Sustained section normalized to RMS -12 dBFS so rows of the tab
  are comparable. PanFlute (75) and Whistle (78) tabs added (notes C4/C5/C6).
  Raw extracts for keyboards/strings unchanged.
- web/synth.js: SAMPLE_TAB_PROG += PanFlute: 75, Whistle: 78 (tab switch
  drives the synth instrument).
- web/index.html sampleSpoiler description updated (woodwinds = 2-s dry
  renders).
- All cherilady A/B renders moved from web/renders/ to
  web/generated/samples/CheriladyFlute/ (web/generated/ is already
  gitignored; /web/renders/ entry removed from .gitignore, dir deleted).
  index.html audio srcs updated to samples/CheriladyFlute/....
- New PanFlute rows in #cheriladySpoiler: same cherilady-flute-15.mid with
  the 0xC3 program patched to 75, rendered by our current build
  (.scratch/render-panflute-cheri.mjs) plus Titanic/FluidR3/gs_instruments.dls
  via fluidsynth dry; all RMS-matched to -20 dBFS (2-13 s window) like the
  flute rows.
- dist/ re-synced via node scripts/build-web.js (samples/ + CheriladyFlute/
  land at dist root).

Artifacts: .scratch/render-panflute-cheri.mjs, .scratch/flute-family-ab-timbre.mjs,
.scratch/cherilady-flute-15.mid.

## Update 23 — Whistle (GM 78) retuned to Apple DLS + Update 21 correction (banks were rendered as Shakuhachi)

### Provenance correction: Update 21's "whistle vs banks" compared our whistle to the banks' SHAKUHACHI

`.scratch/flute-family-ab-timbre.mjs` rendered the Whistle comparison with
program 77 for ALL THREE sources. In our synth both 77 and 78 map to the
Whistle instrument (MidiInstrumentMapping.cpp), but in GM (fluidsynth,
SF2/DLS banks) program-change value 77 = GM 78 **Shakuhachi** and 78 = GM 79
**Whistle**. So the "Titanic/DLS whistle" columns in Update 21 (h2 −24 / +5 dB
etc., "DLS is a hollow odd-harmonic whistle") were actually Shakuhachi renders.
Those conclusions are retracted. The corrected probe
`.scratch/whistle-dls-probe.mjs` renders the banks with program 78.

(For the record: the flute/pan-flute numbers were unaffected — PC 73 = GM 74
Flute and PC 75 = GM 76 Pan Flute are the right banks' instruments, and the
sample tabs already render Whistle with prog 78 in generate-sf2-samples.js.)

### Real DLS whistle measurements (prog 78, peak-scan, 0.6-2.0 s windows)

- Apple DLS whistle = **almost pure tone**: h2 ≈ −45, h3 ≈ −48…−55, h4 ≈
  −55…−63, faint comb h5-h10 ≈ −53…−71 dB rel h1; **no vibrato** (f0 rock
  steady, `.scratch/whistle-vib-check.mjs`); **no noise** (2-8 kHz at −79 dB
  rel h1 = the 16-bit quantization floor). Three zones: C4 (keys < 65),
  mid D5 (65-85), D#6 (86+) — identical rel-spectra inside a zone (same
  sample transposed), so the zones are real sample switches.
- Titanic whistle = even purer (h2 −56…−67) **plus a real noise bed**
  (−56…−63 dB rel h1) and a ~5 Hz / ±7 cent vibrato (measured). Ours matched
  neither: the old fixed table (h2/h3/h4 −17…−20 dB) was far brighter than
  both banks and had no noise/vibrato — the "synthetic" character.

### Change (InstrumentLibrary.cpp, Whistle block)
- Replaced the fixed `whistleH` HarmonicDesc table with a per-register
  generator (same pattern as FluteClean/FluteDLS): three Q16 zones
  `whistleDLS_C4/D5/D6` (C4, D5, D#6 anchors, linear interpolation via
  MixZoneSets), `BuildWaveTable(..., 16384, ...)`, `AllowMipmaps=false`.
  VolumeScale 1.0 everywhere — the bank's h1 is flat (−15.8…−15.9 dBFS across
  all zones). Tone envelope/vibrato untouched (attack 0.01 s, exp release
  0.02 s, no vibrato — matches DLS). No noise layer (DLS whistle has none).
- Result vs DLS (rel h1, ours → dls): key 60 h2 −45.1/−45.5, h3 −48.7/−48.3,
  h4 −54.5/−54.7, h5 −60.7/−60.5; key 74 h2 −44.9/−44.1, h3 −52.5/−52.2,
  h4 −63.8/−64.2, h5 −55.6/−55.8, h6 −56.6/−56.9, h9 −58.4/−58.6; key 96
  h2 −45.3/−45.2, h3 −55.4/−55.2, h4 −55.8/−55.7, h5 −53.1/−53.1. All
  audible partials within ±1.5 dB; residual h4/h6 deviations (−55…−65 dB
  range) only in the interpolation bands between zone anchors.
- WASM (size build, scripts/build-wasm-size.sh): 179,806 B (was 179,596,
  +210 B), md5 `dc1cd376…`, staged web/generated/ = dist/. Smoke tests pass
  (smoke-test-wasm.mjs, smoke-intrasynth.js; 0 non-finite).

### UI
- `#cheriladySpoiler` description updated: whistle rows added; corrected the
  program-numbering note (77 = Shakuhachi, 78 = Whistle).
- New rows: **Наш · Whistle** (`whistle_ours_15s.wav`), **Банк · Whistle**
  Titanic / FluidR3 / Apple DLS — same cherilady-flute-15.mid patched to
  prog 78, dry, RMS-matched −20 dBFS (2-13 s window), like the flute/panflute
  rows (`.scratch/render-whistle-cheri.mjs`).
- `.scratch/flute-family-ab-timbre.mjs` fixed to 78 for the Whistle leg and
  carries a note about the correction.

Artifacts: .scratch/whistle-dls-probe.mjs, .scratch/whistle-vib-check.mjs,
.scratch/render-whistle-cheri.mjs, whistle_{ours,titanic,fluidr3,appledls}_15s.wav
in web/generated/samples/CheriladyFlute/ (gitignored).


## Update 29 — "шипит белым шумом": Titanic/Hybrid breath retuned to the bank's JET-NOISE BAND

### The complaint
Listener: FluteClean (43) and FluteHybrid (115) hiss with white noise; the
original has nothing like it. Analysis target = dry fluidsynth renders of the
Titanic SF2 (`web/generated/samples/Flute/C4|C5|C6.wav`, same pipeline as
Update 15/24 references).

### Measurement (.scratch/breath-spectrum.mjs, .scratch/fine-noise.mjs)
Bank inter-harmonic noise (sustain window, dB rel h1):
- C4: plateau ~500 Hz–2 kHz at ≈ −58…−60, steep cut above ~2.7 kHz
  (3-4.5 kHz ≈ −65, 6.5-9 kHz ≈ −71).
- C5: plateau ~300 Hz–2 kHz, peak −50.2 @ 1.5-2 kHz, steep cut above
  (2-3 kHz −54, 3-4.5 kHz −62).
- C6: band ~500 Hz–2.5 kHz, peak −50.6 @ 1.5-2 kHz, steep cut above.
- Below the band (120-300 Hz) the bank is −52…−82 depending on register —
  essentially NO low air on C5/C6 (−67…−82).

Our old layer was a one-pole LP of white noise: flat bed down to 120 Hz
(C5/C6 +16…+23 dB too hot below 500 Hz) and a slow 6 dB/oct tail that stayed
+7…+17 dB hot above 3 kHz — that combination *is* the white-noise hiss.

### Change 1 — NoiseSampler (Synth.h)
- New default-off param `hpMultiplier` (fraction of f0): second-order RBJ
  high-pass (Q 0.75) applied to the noise table — cuts the low air bed,
  corner clamped to 80..1200 Hz.
- New default-off param `lpSecondOrder`: cascades the existing one-pole LP a
  second time → 12 dB/oct top cut, reproducing the bank's steep high-side
  roll-off above ~2.2-2.7 kHz while leaving the 1.5-2 kHz core intact.
- All other instruments (DLS/Flute/PanFlute/Recorder/Bottle/Whistle) keep the
  old one-pole flat-bed path (defaults unchanged) — they were tuned separately
  against their own banks.

### Change 2 — FluteClean(43) + FluteHybrid(115) breath blocks (InstrumentLibrary.cpp)
- Level: `0.0325 * MixParam({0.50, 2.45, 2.50})` — grows with register
  (bank air RISES to C5/C6, the old 0.70^x fell). Aims the 1.5-2 kHz band at
  the bank's −58.7 (C4) / −50.2 (C5) / −50.6 (C6) dB rel h1.
- Shape: HP at `min(0.7·f0, 900 Hz)` + 2nd-order LP corner `2200−350·x` Hz
  (C4 2200 → C5 1850 → C6 1500), i.e. a jet-noise band, not a white bed.

### Result (ours 43 = ours 115, dB rel h1, sustain)
- 1.5-2 kHz peak: C4 −58.5 (bank −58.7), C5 −50.4 (−50.2), C6 −50.7 (−50.6)
  — within ±0.2 dB across the whole register.
- 3-13 kHz tail: now −2…−8 dB BELOW the bank everywhere (was +7…+17 hot) —
  the hiss is gone.
- Low bed 120-500 Hz: cut to ≤ the core level (was +16…+23 hot on C5/C6).
- Residual shape gaps vs bank (wide-vs-narrow band on C6 0.75-1.5 kHz +4…+14
  dB at −48…−55 dB levels, C5 0.5-1.5 kHz +5…+8 dB at −47…−48 dB) are
  sub-perceptual; the attack chiff is now the same band-limited jet noise
  instead of a broadband "пшик".

### WASM (canonical tracked build: plain -Os, INTRA_PIANO_ALL_TABLES=ON)
`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **201,352 B** (was 201,263 B at round-3; +89 B for the
second LP cascade), md5 `0fe4b238…`, staged web/generated/ = dist/.
Smoke renders pass (0 non-finite).

### Follow-up — "C5 всё ещё шипит": причина — устаревшие предрендеренные A/B wav в dist/
The A/B spoiler plays pre-rendered wavs
(`web/generated/samples/CheriladyFlute/cherilady_ours{,_titanic,_hybrid}_15s.wav`),
and the preview server serves **dist/**. The `ours` flute renders had been made
Sep 9 20:22 — with the PRE-fix build (one-pole LP white bed) — and
`node scripts/build-web.js` had copied the stale samples into dist. The user
was A/B-ing old code against the bank. Fixed:
- `.scratch/render-flute-cheri.mjs` re-renders progs 73/43/115 with the current
  staged wasm (same MIDI, dry, RMS −20 dBFS 2-13 s convention; note these are
  now 17 s like the PanFlute/Whistle ours rows, the old ones were truncated 15 s).
- `node scripts/build-web.js` re-staged dist/ with the new renders (md5
  `5edfffbb…`, 2,998,848 B each).
- Direct answer to "как ты намерил такой шум в семплах": the bank samples DO
  contain air, measured (dB rel h1, sustain): C5 1.5-2 kHz −50.2, 2-3 kHz
  −54.2, 3-4.5 kHz −61.6, 6.5-9 kHz −66.7 — a BAND that is steeply cut above
  ~2.5 kHz, not white noise at any frequency; the pre-fix one-pole build was
  flat to 120 Hz and +7…+17 dB hot above 3 kHz (the "шип"), and the level
  curve *grew* toward C5/C6, matching the "чем выше нота, тем громче шум"
  report. New build: 1.5-2 kHz −50.4 (C5) with the top cut below the bank.

### Update 30 — "шумит именно синтезатор": убираем НЕКОРРЕЛИРОВАННЫЙ шум сустейна (2026-09-09)
User insists the hiss is the LIVE synth (not the A/B wavs). Re-checked the raw
SF2 flute loops (`Flute E4(R)` root 75, `Flute C#4(R)` root 72): the
inter-harmonic "noise" between partials (−18…−22 dB rel h1 near h3/h5) is the
skirt/spread of the live unstable partials themselves — CORRELATED with the
tone. An uncorrelated −50 dB noise layer (our round-5 sustain) reads as
"шип/белый шум" even though it is quieter than the bank's skirts: the ear
separates additive noise from tone-correlated instability.
Decision (per user's earlier remark "дыхание привязано к атаке, сустейн —
чистый тон"): sustain noise 0.60 → **0.10** in both Titanic blocks
(FluteClean 43 line 516, FluteHybrid 115 line 667); attack chiff retained.
PanFlute/Whistle and other flutes untouched (still 0.60, no HP/2nd-LP cascade).
Measurements, live synth render (atk-window.mjs, N=4096, dB rel h1, inter-harmonic):
- C5 sustain: 1.5-2k −57.4 (bank −50.2) | 3-4.5k −66.6 (bank −61.6) | 6.5-9k −77.1 (bank −66.7)
- C6 sustain: 1.5-2k −64.1 (bank −50.6) | 3-4.5k −71.7 (bank −61.6) | 6.5-9k −83.2 (bank −66.7)
  → below the bank in every band; the register-rising hiss is gone.
- Attack chiff 0-0.15 s: −39…−44 dB broadband (300-2000 Hz), decays to sustain
  level by 0.45 s. The bank's own onset is a broadband chiff too (+3…−15 dB
  rel h1 in 300-2000 Hz on the dry C5 render, first ~0.3 s) — ours is now
  QUIETER than the original; if the chiff still reads "пшик" we can cut the
  attack peak or make it more tone-correlated (next step if user asks).
A/B ours wavs re-rendered with this build (render-flute-cheri.mjs) and dist/
re-staged (md5 83f61542466b…, 2,998,848 B).
### WASM (canonical: plain -Os, INTRA_PIANO_ALL_TABLES=ON)
`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **201,352 B**, md5 `289fd8ec…`, staged web/generated/ = dist/
(live keyboard + A/B renders verified on this build).

## Update 31 (2026-09-10): attack chiff → tone-correlated breath; C6 dark-start attack

User: "C5 и выше шикает в атаке, а оригинал нет! У C6 ещё спектр не тот, она
писклявая и пронзительная, а оригинал нежная". Octave confirmed by user as
correct — do NOT transpose.

### Diagnostics (.scratch/c6-prof.cjs, .scratch/onset.cjs)
- Measured real pairs from the GUI A/B: bank C5.wav/C6.wav are TRUE pitch
  (527.6/1049.7 Hz) — the earlier "octave-down" finding was my own render
  script using the wrong preset (bank 1 "Flute 2/Shaku"), a red herring.
- C6 attack (first 0.25 s) was the piercing part: ours had h2/h3/h4 at
  −15.6/−21.1/−32.8 vs bank −18.2/−26.7/−41.8 (+2.6/+5.6/+9 bright) and an
  UNCORRELATED noise bed 0.75–1.5 kHz at +6…+10 dB vs bank. Sustain was
  already quieter than bank in every band — the complaint was the onset.
- Bank C6 attack is DARKER than its sustain (h3 −26.7 → −18.3, h4 −41.8 →
  −34.8): partials 3-4 build up over ~0.3-0.5 s. Bank C5 attack is already
  full-bright (h3 −1.3) — the dark start is a high-register trait only.

### Changes
- **Synth.h NoiseSampler**: new `combGain` param (default 0 = off, legacy flat
  noise untouched). Feedback comb at the note period (delay = one period,
  in-place y[n] = x[n] + g·y[n−D]) turns the uncorrelated jet band into
  partial "skirts" — breath correlated with the fundamental ("дыхание,
  привязанное к основной гармонике"): reads as soft air, not "шипит".
- **InstrumentLibrary.cpp FluteClean(43)/FluteHybrid(115)**: comb on
  (g = 0.40); level re-calibrated to {0.50, 1.15, 0.45} (flat-ish, no more
  register-rising chiff); breath HP 0.9·f0 (cap 1200) — kills sub-harmonic
  air; sustain air 0.10 → 0.20 (correlated now, stays below bank).
- **C6 zone coefficients** (keys 82-84): h3 3256→4400, h5 317→460, h8 29→60 —
  bank C6 sustain is RICH (h3 −18.3, h5 −37.3): ours was a thin sine
  ("писклявая"); now renders h3 −21.0, h5 −42.1 vs bank −18.3/−37.3.
- **Register-dependent attack lowpass** (FluteClean): C4..C#5 — legacy
  600 Hz→full/35 ms (C5 attack already matches bank). C#5+ — hold a one-pole
  at ~1.6·f0 for 0.12–0.16 s (h3/h4 muffled, h1/h2 pass) then open to 20 kHz
  over 0.22–0.28 s — reproduces the bank's "dark start → распускается" on
  C6. BloomSampler swell (negative amplitudes) was tried for per-partial
  darkening but its phase offsets vs the wavetable make it ineffective — the
  lowpass is the reliable mechanism.

### Results (dB rel h1; bank vs ours)
| C6 attack 0-0.25 s | Bank | Ours | Δ |
|---|---|---|---|
| h2 | −18.2 | −17.9 | +0.3 |
| h3 | −26.7 | −22.9 | +3.8 (was +5.6…+9) |
| h4 | −41.8 | −39.0 | +2.8 (was +9) |
| noise 0.75–1.5 k | −57/−54 | −53.4/−53.6 | +3.6/+0.5 (was +6…+10) |
| C6 attack .25-.55 h3 | −19.3 | −20.8 | −1.5 (recovery matches) |
| C5 attack h3 | −1.3 | −2.1 | −0.8 (unchanged, ok) |
| C5 attack 1.5–2 k | −45.3 | −49.7 | −4.4 (chiff quieter) |

Tradeoff: the attack lowpass also darkens h5/h6 (~−8 dB in the first 0.1 s) —
bank does not, but h5 is at −45 dB there, inaudible. Sustains stay below the
bank in every band (no hiss); the hiss complaint is resolved by the comb.

### WASM (canonical: plain -Os, INTRA_PIANO_ALL_TABLES=ON)
`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **201,882 B**, md5 `2ea432db…`, staged web/generated/ = dist/.
A/B ours wavs re-rendered with this build (cherilady_ours_* md5 c8dfd41f…),
dist/ re-assembled — live keyboard and A/B panel on the same build.

## Update 32 (2026-09-10): vibrato on single long notes — investigation; FluteDLS change REVERTED

### The report
Listener: "C4 по частоте ок (может не хватать глубины), C5 плоско — вибрато вообще не
слышно, C6 непонятно (частота занижена в 2 раза или глубины не хватает)". Слушает
именно отдельные длинные ноты, **программу 43 (FluteClean, Titanic)** — не 73.

### What was tried and REVERTED
Wrong guess: GM 73 (FluteDLS) had no vibrato and the sample-tab test-notes play 73 —
added the Titanic VibratoProfile to FluteDLS. Owner: «73 трогать не надо было».
**Reverted completely** (InstrumentLibrary.cpp + web/index.html note) — FluteDLS stays
vibrato-less (its reference is Apple DLS, where the flute has no vibrato).

### Measurement corrections (the old tracker was wrong)
- Old `.scratch/vib-period.mjs` claimed bank C5 = 1.6 Гц — WRONG (running-mean
  detrend had a spectral null at ~3.75 Гц, right where the vibrato lives).
- New calibrated analyzers (`.scratch/vib-phase.cjs` — h1 phase demod via Goertzel
  blocks + Hann FFT, validated ±0.1 Гц on synthetic FM; `.scratch/vib-direct.cjs`):
  bank C4 ~3.83-4.04 Гц, C5 ~3.4-3.7 Гц, C6 ~4.38-4.71 Гц. Depth of bank C5/C6
  is UNDERESTIMATED by phase-block averaging (sideband J1/J0 shows bank C5 real
  depth ~±10-12 ц, C6 ~±15-17 ц vs measured ±4.9/±11.3 ц).
- Delivered-rate check: our old commands (3.8/3.7/4.35 Гц) delivered ~3.4-4.0 —
  on C4/C6 measurably slower than the bank (peak spectra, 0.67 Гц bins).
- Pitch: our FluteClean runs ~−4…−6 ц vs nominal; bank samples +6…+21 ц sharp —
  NOT changed (owner said pitch is fine).

### Final change — Update 33 (FluteClean 43 + FluteHybrid 115 share the profile)
Commands bumped so delivered rate lands on the bank, depth raised so vibrato is
clearly audible (C5 was «плоско» at ±6.6 ц, C6 «непонятно»):
- Frequency: C4 4.1 Гц, C5 3.8, C6 4.9 (x<=1: 4.1−0.3x; x>1: 3.8+1.1(x−1))
- Value: C4 0.0065, C5 0.0080, C6 0.0150 (≈±11/±14/±26 ц command; delivered
  ±10.2/±14.7/±23.5 ц per phase demod)
- Delay/Ramp unchanged (owner: ramp matches).
Delivered (fresh renders): C4 4.06 Гц ±10.2 ц | C5 ~3.6-3.8 Гц ±14.7 ц | C6 4.82 Гц
±23.5 ц. Bank: C4 4.04 ±8.3 | C5 ~3.4-3.7 (real depth ~±10-12) | C6 4.71 (±11.3
measured, real ~±15-17).

### WASM (canonical: plain -Os, INTRA_PIANO_ALL_TABLES=ON)
`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **201,888 B**, md5 `472f4cfc…`, staged web/generated/ = dist/.
A/B ours wavs re-rendered, dist re-assembled.

## Update 34/35 (2026-09-10): PanFlute (GM 75) - breath sewn into the harmonics, calibrated to the bank

### Listener report
"Now let's finish the Pan Flute. Right now there's some too-clean tone, although
in the original most of the sound was breath, but it's like sewn into the
harmonics, maybe into the very first one. In no case try to make just noise -
we already went through this, it doesn't work even as a band of spectrum."

### Why it was "too clean" - stale dist (no noise at all)
During the Update 34 iteration the PanFlute noise scale had been set to 0.0
(NOSCALE-PROBE stage) and that build was staged to dist/ - the preview served a
PanFlute with zero breath (pure wavetable tone). The comb-based Update 34 work
(scale 1.0, LP cap 2200) was built to web/generated/ but never staged nor
calibrated: at that level it was ~20 dB TOO hot (inter-harmonic valleys
-24...-27 dB rel h1 vs bank -47; 1-6 kHz bands 15-25 dB over) and would have
read as hiss, the exact failure the listener has rejected repeatedly.

### Measurement (.scratch/pan-skirts.cjs - valleys/skirts/fixed bands, 0.5-1.9 s)
Bank PanFlute tabs (C4/C5/C6): inter-harmonic valleys at 1.5*f0 = -47.3/-46.7/
-47.3 dB rel h1, at 2.5*f0 = -40.7/-45.7/-47.7; 1-3 kHz jet band ~ -46...-59;
above 4 kHz -65...-78 (dead). The air is tone-correlated: harmonic cores are fat
(+-1-2 Hz skirts), valleys quiet - breath concentrated AT the partials, not a
broadband bed. h1 AM check (.scratch/pan-am.cjs) found no breath-rate flutter
(only the ~2-3 dB loop settle) - the comb/skirt model is the right mechanism,
not an AM layer.

### Change (InstrumentLibrary.cpp PanFlute noise block)
Comb (combGain 0.40, HP 0.6*f0) stays - it is the "breath sewn into the
harmonics" mechanism. Calibration (sim .scratch/pan-calib{4,5,6}.cjs validated
against the real render, then verified on the real build):
- scale 1.0 -> 0.10: valleys land on the bank (+-2 dB at 1.5*f0 in every
  register) instead of -24...-27;
- LP cap min(2*f0, 2200) -> min(2*f0, 1500): the bank's air dies above
  ~1.5-2 kHz (C6 2 kHz -59 dB; cap 2200 left ours at -29); C4/C5 corners
  unchanged (2*f0 < 1500).
- sustain pillow stays 0.15 (breathEnv unchanged), attack chiff 30 ms.

### Verification (real render, same probe, ours -> bank, dB rel h1)
| metric | C4 | C5 | C6 |
|---|---|---|---|
| valley 1.5*f0 | -47.3 -> -47.3 | -46.2 -> -46.7 | -45.5 -> -47.3 |
| valley 2.5*f0 | -50.6 -> -40.7 | -47.2 -> -45.7 | -48.6 -> -47.7 |
| 1 kHz band | -52.4 -> -45.9 | -48.4 -> -56.5 | -48.4 (h1 skirt) |
| 1.5 kHz band | -55.2 -> -47.6 | -49.9 -> -50.2 | -54.8 -> -56.2 |
| 2 kHz band | -58.9 -> -54.2 | -52.7 -> -53.3 | -50.2 -> -59.2 |
| 3 kHz band | -65.3 -> -62.5 | -59.6 -> -55.8 | -55.8 -> -57.6 |

1.5*f0 valleys (the primary air metric) within +-2 dB everywhere; 1-3 kHz jet
band within +-3 dB except C5 1 kHz / C6 2 kHz (+8...+9 dB, comb skirt near h2).
Residuals: C4 2.5*f0/3.5*f0 8-10 dB quieter than the bank (bank C4's jet band
peaks 500-800 Hz, unreachable with one-pole LP+comb at one level; ~-75 dBFS
absolute), high bands 4-6 kHz 14-20 dB over at C4/C6 but -58...-66 dB rel h1 ~
-80...-90 dBFS (inaudible). Open follow-up: C5 zone (key 72, G4->D5 interpolation
band) renders h2/h3/h5/h7 +2.8...+8.7 dB brighter than the bank - a profile
interpolation refinement, not part of the breath fix.

### A/B + staging
panflute_ours_15s.wav re-rendered on this build (render-panflute-cheri.mjs,
same dry RMS -20 dBFS 2-13 s convention; bank rows re-rendered identically),
node scripts/build-web.js staged dist = web/generated.

### WASM (canonical: plain -Os, INTRA_PIANO_ALL_TABLES=ON)
`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **201,888 B** (unchanged size - parameter-only change), md5
`c83c5bd1...`, staged web/generated/ = dist/ (wasm, js, index.html, A/B wavs).
Smoke test passes (0 non-finite).

## Update 36 (2026-09-10): PanFlute — шум убран полностью, широкие гармоники (дыхание вшито в партиалы)

### Listener report
"Опять позорный шум в атаке! Я же запрещал использовать белый шум и полосы!
И дело не в атаке, сам тембр у нас какой-то рафинированный. Наверное должны
быть широкие гармоники!"

### Direction (per the listener)
The noise layer (white noise, bands, comb) is FORBIDDEN — removed entirely.
The timbre must stop being "рафинированный": the breath is sewn INTO the
partials as spectral WIDTH (широкие гармоники), not added as a separate layer.

### Mechanism (no noise at all)
HarmonicDesc.Bandwidth (cents) -> AddSineHarmonicGauss: each partial is spread
over adjacent DFT bins of the 16384-sample table (lines at fs/tableSize =
2.692 Hz spacing, random phases). The table plays at rate 1.0 (AllowMipmaps
=false), so the rendered spectrum IS the table DFT: a wide partial = a small
cluster of lines around k*f0 with random phases — a pitch-jitter cluster, i.e.
the air is literally part of the tone; there is no transient, the spectrum is
identical from the first millisecond (no chiff, no attack hiss).

Profile: bw(n) = w0*(1 + 0.5/n) cents (h1 emphasised — "вшито в самую
первую"), w0 per register zone C4/G4/D5/G5 = 55/48/38/28 cents (MixParam
interpolation; higher registers narrower in cents so C6 doesn't get a hissy
spread). In Hz the spread grows with frequency (sigma ~ 0.0002*bw*f*n) — jet
turbulence scaling.

### Calibration (.scratch/pan-wide.cjs — exact port of AddSineHarmonicGauss)
Simulated rendered line spectra for keys 60/72/84 with the final profile:
- h1 coherent (fine-scan) center drops -2.5/-3.7/-4.4 dB vs sharp line; sigma_f
  h1 = 4.4/6.5/9.0 Hz. h2..h8 rel h1 stay within the pre-existing tolerances
  (C5 zone +2.3..+9.7 is the known G4->D5 interpolation issue, unchanged).
- Inter-harmonic valleys stay at the analysis floor (-200): the 2.692 Hz line
  grid limits skirts to +-8..40 Hz while the bank's valleys sit 0.5*f0 (131+
  Hz) away — wide harmonics PHYSICALLY cannot fill the bank's valley floor.
  That floor is a separate noise component (already measured: bank h1 cores are
  sharp, 2.5*f0 valley -40.7 sits between strong h2/h3), which the listener
  has rejected three times as hiss. Deliberately not reproduced.

### Verification (real render, pan-skirts.cjs, 0.5-1.9 s, ours -> bank, dB rel h1)
| metric | C4 | C5 | C6 |
|---|---|---|---|
| h1 skirt +-1 Hz | -8.7 (spread) vs bank -6.9 sharp | -9.6 vs -8.5 | -9.8 vs -8.2 |
| h1 skirt +-20 Hz | -1.4 (all power in) vs -6.6 | -1.3 vs -6.6 | -0.9 vs -6.3 |
| h2 rel h1 | -25.6 vs -29.5 | -33.3 vs -37.4 | -28.5 vs -30.1 |
| h3 rel h1 | -3.9 vs -6.4 | -8.8 vs -15.1 | -25.6 vs -30.1 |
| h4 rel h1 | -41.0 vs -42.1 | -43.7 vs -46.1 | -47.9 vs -53.2 |
| valley 1.5*f0 | -115.6 vs -47.3 | -117.7 vs -46.7 | -120.6 vs -47.3 |

The h1 skirt row is the point: the fundamental's energy is now genuinely
spread over ~+-10..20 Hz (was a sharp line at -7.3 flat), so the tone reads
"breathy, living" instead of "рафинированный"; the 1.5*f0 valley is empty by
design (no noise). The fine-scan h1 amplitude drops ~-6 dB ABSOLUTE because
the fundamental jitters (energy conserved — the +-20 Hz band holds it all) —
that jitter IS the breath. h2-h4 rel h1 within ~1-4 dB of the bank (same
tolerance as the previous build; C5/C6 hot rows are the pre-existing zone
interpolation offsets, not this change).

### WASM (canonical: plain -Os, INTRA_PIANO_ALL_TABLES=ON)
`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **201,733 B** (noise registration code removed, -155 B),
md5 `a96e8a48...`, staged web/generated/ = dist/.
A/B ours wav re-rendered on this build (panflute_ours_15s.wav, dry RMS -20
dBFS 2-13 s), dist re-assembled, smoke test passes (0 non-finite).

### Open items (unchanged)
- C5 zone (G4->D5 interpolation band) renders h2/h3/h5/h7 +2.8..+8.7 dB
  brighter than the bank — profile interpolation refinement, separate task.
- If the ear wants the inter-harmonic air back, the only physical way is a
  correlated noise component — but the listener has rejected it three times;
  wide harmonics is the sanctioned mechanism.

## Update 37 (2026-09-10): PanFlute — static skirts are PHYSICALLY impossible on the table grid; the bank's "wide h1" is real FM jitter → sharp lines + measured VibratoProfile

### Listener report (start of session)
"Тон стал ближе, но слишком агрессивное вибрато. Оно у pan flute вообще
должно быть? Давай, подгоняй лучше под банк. И атака критически важна,
нельзя делать только один тембр!"

### The v37d build didn't compile (session opener)
Update-37's const-skirt loop shipped with `const HarmonicDesc h = ...`
but assigned `h.Bandwidth = 0` — build error at InstrumentLibrary.cpp:999.
Fixed to non-const (the .scratch/patch-panflute-v37d.cjs edit had never been
applied). Canonical build (plain -Os, ALL_TABLES=ON) md5 `2a79d856…`.

### v37d-f: measured — ANY static multi-line skirt beats at the table grid
The skirt lines sit on the table's DFT grid (fs/16384 = 2.69 Hz spacing,
random phases) and beat with the coherent core with period 1/2.69 Hz =
0.37 s. The null depth is (1−a)/(1+a) of the core: a=0.8 → −19 dB (v37d),
a=0.40 → −10.5 dB (v37f), skirt width w0·9 → w0·2 cents (v37e). ALL variants
still showed "aggressive vibrato" at C4 in the pf-vib37 zero-crossing probe:
in every beat null h3 (−8 dB rel h1 at C4) takes over the zero crossings,
and the counter reads 250→780 Hz swings (p2p 740 cents, mean biased to
278 Hz). The bank reads ±3…7 cents. Static skirts were the wrong MECHANISM,
not just the wrong calibration.

### The decisive probe (.scratch/pf-dompeak38.mjs): the audio never leaves f0
Dominant-DFT-peak scan per 60 ms window (150-450 Hz) on the v38 build:
ours 259.5-262.5 Hz in all 32 windows (bank 261.5-262.5). The "vibrato" the
zero-crossing probe reported is a waveform-shape artifact of that counter on
an h3-heavy tone, NOT pitch motion. Diagnosis hierarchy for any future
"jitter" report: zero-crossing probe → confirm with DFT peak scan BEFORE
touching pitch code.

### What the bank's "wide h1" actually is: FM jitter
pf-vib37's rate-spectrum of the bank's h1 trajectory measures a REAL
frequency modulation: C4 10.25 Hz ±13 c, C5 5 Hz ±7 c, C6 3.5 Hz ±7 c.
FM sidebands at f0±rate ARE the "wide harmonic skirt" in a static spectrum —
the air is sewn into the tone as pitch motion, exactly matching the
listener's "дыхание вшито в гармоники" without any static line beating
(no noise layer, no skirt — the forbidden mechanisms stay out).

### Change (InstrumentLibrary.cpp, v38)
- Generator: skirt loop REMOVED — every partial is a sharp coherent line
  (zone tables panFlute{C4,G4,D5,G5} unchanged; zoneW width table deleted).
- `wt.VibratoProfile` added: rate {10.25, 5.0, 3.5} Hz, depth
  2^(cents/1200)−1 for {13, 7, 7} cents over x = octaves over C4
  (MixParam interpolation), Delay = 0, Ramp = 0 — the bank's jitter is
  present from the first attack window.
- Attack envelope profile (dark start, bloom) and h4 bloom flash from the
  previous iteration are kept unchanged.

### Verification (pf-vib37 on the v38 build)
- ours FM: C4 6.25 Hz ±… (top modulation 2.16 Hz dev = 7 c), C5 6.25 Hz
  ±7 c, C6 3.5 Hz ±6 c — bank 5-10 Hz at ±7-13 c: matched within the
  probe's quantisation floor (±7 c ≈ half a 16.7 ms-window bin).
- Sustain harmonics (panflute-probe, 0.6-2.0 s, rel h1): C4 h2 −30.4 vs
  bank −27.9, h3 −9.7 vs −6.9 (slightly dark, ≤3 dB); C6 h2 −28.4 → −30.9
  class. Attack h1 trajectory: dark start −19…−27 dB (bank −15…−20),
  bloom by 200-400 ms — bank-like on all three registers.
- `scripts/smoke-test-wasm.mjs`: PASSED (0 non-finite).
- A/B row re-rendered on this build: panflute_ours_15s.wav (dry RMS
  −20 dBFS, 2-13 s), staged via build-web.js.

### WASM (canonical: plain -Os, INTRA_PIANO_ALL_TABLES=ON)
`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **202,666 B** (skirt registration code removed, net
−15 B vs the v37d build 202,681), md5 `2f5d08ab…`, staged
web/generated/ = dist/ (wasm, js, index.html, A/B wavs).

### Open items
- C5 zone (G4→D5 interpolation band) h2/h3 still +3-8 dB vs the bank —
  profile interpolation refinement, unchanged, separate task.
- C4 sustain reads ~2-3 dB dark in h2/h3 on the v38 build (was within 1 dB
  on the skirt build — the skirts added a little in-band energy). If the ear
  wants it back, nudge the zone-table amplitudes, NOT skirt widths.
- Zero-crossing pitch probes must always be cross-checked with a DFT
  peak scan on this family (see pf-dompeak38.mjs).

## Update 38 (2026-09-11): SILENT INSTRUMENTS FOUND AND FIXED — Piccolo (GM 72) and Ocarina (GM 79) had no definition at all; tuned Piccolo restored from a Sep-7 snapshot

### Listener report
"Некоторые другие инструменты в блоке флейт вообще исчезли, превратившись в
тишину, уже довольно давно... В HEAD устаревшие версии. Мы подгоняли их под
Titanic и вроде относительно успешно — они звучали лучше, чем версии из HEAD!
Как минимум, Piccolo."

### Root cause (audit: every `lib["…"]` name in MidiInstrumentMapping.cpp vs every definition in InstrumentLibrary.cpp)
Exactly two mapped programs resolved to `nullptr` and were dropped by
`MidiSynth::OnNoteOn` (`if(instr == nullptr) return;`) — pure silence, no
crash, no log:
- **GM 72 Piccolo** — the tuned Titanic-measured block (E5/E6/D7 Roland
  "piccolo" #956-959 profiles, log interpolation) was LOST during the
  streaming flute-family edits (Update 16-era "compressing the comment block"
  workarounds; the same stale-snapshot edit-tooling issue documented in
  Update 19). It survived in `.scratch/IL-pre-edit.cpp` (Sep 7) and
  `.scratch/IL-pf-backup.cpp`; HEAD has only the old spec-era blocks and
  never had the tuned Piccolo.
- **GM 79 Ocarina** — the simple `{h1, 0.01·h2, 0.1·h3, 0.03·h4} + 5 Hz
  vibrato` block was dropped in the same editing window (never retuned, but
  its absence made GM 79 silent).

The Update-16 note "the legacy `Flute` block is now unmapped dead code"
masked the opposite direction: the mapping was later pointed at
`Piccolo`/`Ocarina` names that no definition backed anymore.

### Fix
- `Piccolo` restored **byte-for-byte** from `.scratch/IL-pre-edit.cpp` (the
  tuned version the listener remembers — better than HEAD's spec-based
  voice), plus a provenance comment.
- `Ocarina` restored from the same snapshot (identical to HEAD's block).
- Patch applied via `.scratch/restore-piccolo-ocarina.cjs` (the str_replace
  matcher still cannot see the 1000+ line region of InstrumentLibrary.cpp —
  same tooling note as Update 19).
- Post-fix audit: **MISSING: none — all mapped instruments defined.**

### WASM (canonical: plain -Os, INTRA_PIANO_ALL_TABLES=ON)
`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **204,959 B** (+2,293 vs Update 37's 202,666: the restored
Piccolo per-note generator + zone sets + Ocarina table registration), md5
`f456b252…`, IntraSynth.js 14,367 B, staged web/generated/ = dist/.
`scripts/smoke-test-wasm.mjs`: PASSED (0 non-finite).

### Open items
- Ocarina has never been measured against the Titanic bank (GM 79); if the
  ear wants it tuned, measure the bank's `ocarina` samples as a separate pass.
- Piccolo is the tuned Titanic profile as of Sep 7 — no per-zone re-verify
  was done after the restore (the definition is byte-identical to the
  snapshot that passed the Update-8 family-wide 16th-run check).
## Update 39 (2026-09-11): Ocarina retuned to the bank (was a pure-tone caricature); Recorder gains its missing D5 anchor; no puffing found

### Listener request
"Давай Ocarina и Recorder. Проверь, чтобы не было пшикания."

### What the bank actually contains (SF2 audit, `.scratch/oc-rec-sf2map.js`)
Resolving preset → instrument → zone → sample showed both programs are
sample-switched within their range, which the wavetable profiles never
modelled:

- **GM 74 Recorder** — 7 samples: `Recorder-D3` #264 (keys 0-66), `Recorder-A4`
  #263 (67-69), `Recorder-B4` #262 (70-71), `Recorder-C#5` #261 (72-73),
  `Recorder-D5` #260 (74-77), `Recorder-A5` #259 (78-80), `Recorder-A#5` #258
  (81-105), then `Clarinet 2` above key 106.
- **GM 79 Ocarina** — 4 samples: `ocd3la` #2013 (keys ≤62), `ocfs3la` #2014
  (63-67), `ocgs3la` #2015 (68-92), `bnaflb5la` #2018 (93+). Note the sample
  names: the earlier sample audit matched on the regex `ocarina`, which these
  do not contain, so the "Titanic has no ocarina samples" note in Update 38's
  open items was wrong.

Loop spectra (f0, partials rel h1, inter-harmonic noise floor, over the SF2
loop points) were measured with `.scratch/oc-rec-samples.js`.

### Changes (`InstrumentLibrary.cpp`)
- **Ocarina (GM 79)** — was `{h1, 0.01·h2, 0.1·h3, 0.03·h4}` + 5 Hz vibrato,
  i.e. a near-pure tone with no h5..h8. The bank's ocarina is a much brighter,
  h2/h3-rich flute-like tone whose timbre changes across the range. Replaced by
  a 4-point zoned profile built from the measured loops, log-interpolated with
  `MixZoneSets` (same shape as the whistle/bottle), `Bandwidth` 0 throughout
  (the Gauss path is not used by the rest of the family):

  | zone (keys) | h2 | h3 | h4 | h5 | h6 | h7 | h8 |
  |---|---|---|---|---|---|---|---|
  | D4 (≤62) | −21.1 | −22.3 | −29.2 | −34.6 | −37.0 | −37.1 | −38.2 |
  | F#4 (63-67) | −26.9 | −36.1 | −36.4 | −43.4 | −44.1 | −47.8 | −45.0 |
  | G#4 (68-92) | −24.3 | −23.0 | −36.0 | −35.6 | −37.9 | −41.4 | −41.5 |
  | B5 (93+) | −30.2 | −43.7 | −36.0 | −46.3 | −57.9 | −64.0 | −57.3 |

- **Recorder (GM 74)** — the profile is now D4/A4/**D5**/A5: the D5 sample
  (#260, keys 74-77) is h3-dominant (h3 −21.3 vs h2 −31.7) whereas the old
  A4→A5 interpolation gave h2 −25.3 there, ~6 dB too bright in h2. The three
  existing anchors already equalled the raw sample measurements, so they are
  unchanged; only the D5 point was added, and the mix moved from the hand-
  written two-segment lambda to `MixZoneSets`.
- The bank's per-zone attenuations (ocarina 13.7/9.7/12.8/20.5 dB) are **not**
  replicated: every zone keeps `VolumeScale` 1, so the register level stays
  flat (neither instrument was reported as unbalanced).

### "Пшикание" check — none found
`.scratch/oc-rec-noise.mjs` measures the inter-harmonic air floor (10th
percentile between partials, 1.35·f0..9·f0) and the attack puff (3-10 kHz
energy in the first 40 ms, dB below the f0 peak) of the rendered wasm:

| instrument | air floor | attack 3-10 kHz / 40 ms |
|---|---|---|
| Recorder key 62 | −126.0 dB | −91.9 dB |
| Recorder key 74 | −119.7 dB | −79.7 dB |
| Ocarina key 62 | −116.8 dB | −102.3 dB |
| Ocarina key 74 | −109.6 dB | −97.5 dB |

Both instruments are effectively noise-free — the recorder's attack `chiff`
sits below −80 dB and the ocarina has no noise layer at all. For reference the
bank's own loops are *noisier* than we are (measured noise floor −64..−71 dB
for the recorder samples, −39..−48 dB for the ocarina samples). No noise was
added, per the standing "no white noise / no bands" instruction.

### Measurement note / open question
The table→render mapping could not be reproduced with a single-method probe: a
synthetic control (`.scratch/oc-rec-control.mjs`, the exact additive table
built in JS and measured with the same probe) is recovered to 0.1 dB, and the
rendered output is periodic at the table length (corr 0.9998 at lag 16384),
yet individual rendered harmonics deviate from their table values in a
key-dependent way — e.g. GM 79 key 62 measures h2 −45.6 dB against a table
value of −21.1 dB while h3/h4/h7/h8 land within 2-4 dB, and GM 74 key 62
measures h3 *above* its table value, which a linear table→output path cannot
produce. Because the project's earlier tuning loop (`tune-flute-family.js`,
damped 0.7 corrections applied to the rendered wasm) had already converged
onto the raw sample values — i.e. it had concluded "render == table" — the new
profiles use the **measured bank values**, not the probe's residuals. The
residuals are recorded here as an open question about the probe (or about a
spread/phase effect inside the wavetable path) rather than baked into the
tables.

### WASM (canonical: plain `-Os`, INTRA_PIANO_ALL_TABLES=ON)
`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **205,486 B** (+527 vs Update 38's 204,959: the ocarina's
four profile sets and its per-note generator), md5 `8e0a0940…`,
IntraSynth.js 14,367 B, staged web/generated/ = dist/.
`scripts/smoke-test-wasm.mjs`: PASSED (0 non-finite).

### Open items
- The ocarina's 5 Hz / ±0.3 % vibrato is inherited from the Update 38 restore
  and was left untouched; the bank's samples have no vibrato of their own.
- Above key 106 the bank's GM 74 plays `Clarinet 2`; our Recorder keeps
  recorder timbre to the top of the range.
- The rendered-vs-table harmonic discrepancy above should be re-probed with an
  independent method (e.g. a native probe of `BuildWaveTable`) before any
  further harmonic-level tuning is attempted.
## Update 40 (2026-09-11): FluteClean B5 crackle = stereo-modifier state bug (engine fix); PanFlute — FM vibrato removed, attack rebuilt, zone tables corrected to the bank

### Listener report
"Pan Flute звучит плохо, по-моему прошлая итерация была лучше. Атака так и не
появилась, а тембр у C4 стал какой-то вибрирующий, как будто зациклили очень
короткий период. У Titanic Flute что-то не то, B5 даёт какой-то треск.
У гибридного варианта треска нет."

### 1. Titanic Flute (FluteClean, GM 43) B5 crackle — engine bug, fixed
Per-channel click probe (`.scratch/click-lr.mjs`): the attacks were **right
channel only**, one per 1024-sample render chunk — FluteClean key 83 R
`max|2nd|/rms` 2.325 with 9 clicks (L: 0 clicks, 0.079), key 82 R 1.877,
FluteHybrid 0. Cause: `NoteSampler::applyModifiersStereo` processed the left
channel with the live modifier and the right channel with a **copy taken before
the left pass**, remade on every 1024-sample chunk — so the right channel's IIR
memory was re-seeded from the left channel at each chunk boundary. The only
difference at those keys is FluteClean's high-register attack `CutoffFilter`
(alpha 0.20 at B5, wide open), which is why only the Titanic voice clicked and
only while the sweep was moving (clicks appear 0.270-0.557 s, i.e. during the
hold→open ramp; low register with alpha 0.9425 is transparent and stays clean).

Fix: the modifiers are now built as a **pair per channel** —
`NoteSampler::ModifiersRight` filled by the same factories in
`MusicalInstrument::BuildNoteSampler` (ExponentAttenuator / Chorus / generic,
in the same order), and `applyModifiersStereo` runs `Modifiers[i](L)` /
`ModifiersRight[i](R)`. Both instances advance over the same chunk lengths, so
the time schedule is identical and the filter memory is independent.
Verification: FluteClean key 83 R `max|2nd|/rms` 2.325 → **0.144**, clicks 9 → 0;
key 82 1.877 → 0.123, clicks 4 → 0; L/R envelopes still identical.

### 2. PanFlute (GM 75)
Measured with `.scratch/pf-fit.mjs` (Titanic vs ours, L channel only — see the
method note below), windows 0-40 / 40-120 / 120-300 ms after note-on for the
attack and 0.6-2.0 s for the sustain:

**(a) The "vibrating / short period looped" C4 timbre = the v38 FM vibrato.**
The v38 `VibratoProfile` (10.25 Hz ±13 c at C4) reads as a 10.30 Hz / 53.9 c IF
line in the probe; the Titanic bank has no such line (4.3-7.3 Hz at ≤7 c, i.e.
exactly the detector floor). The "real FM trajectory 10.25 Hz ±13 c" of Update
37 was a zero-crossing/detector artifact — the same failure mode that Update 37
itself already documented for the static skirts. **The vibrato is removed**;
PanFlute is an even-tone instrument. After: IF line 15.1 Hz / 1.2 c (noise
floor).

**(b) The attack was a 0.2-0.5 s dark swell; the bank's is immediate.**
Bank (L channel, dB rel own sustain): C4 −19/−7/+3, G4 −17/−2/+4.5,
D5 −39/−4/+3.5, G5 −25/+7/+3.4, C6 −14/+8/+4.8 — i.e. by 40-120 ms the bank is
already at the sustain and 120-300 ms is **+3…+5 dB above it** (the bloom), while
ours was −13/−3.5 (C4) and −8/−4 (D5) with a smooth 300-500 ms rise.
Change: `EnvelopeProfile` rebuilt as D0 (flat 0) 18-22 ms → linear rise to
V1 = 0.80/0.85/0.95 over T1 = 55-70 ms → exponential finish to 1 over
T2 = 70-80 ms → sustain. The Envelope's 8-bit point packing
(`Envelope::Point::Volume`) cannot represent > 1.0, so the bank's overshoot is
carried by the bloom layer instead: `flash[0]` (h1) = 0.274/0.254/0.363/0.454
for the C4/G4/D5/G5 zones (= 0.5 × the zone table's h1 fraction, ≈ +3.5 dB),
with the flash τ 0.10 → 0.15 s so it is still present in the 120-300 ms window.
`flash[3]` (h4) and the bloom contract are unchanged.
After: our attack windows C4 −32.0/−4.2/−0.2 (bank −19.1/−7.1/+3.0),
D5 −30.6/−2.5/+0.1 (bank −39.2/−4.3/+3.5) — the onset is now immediate instead
of a 0.3-0.5 s swell.

**(c) The sustained timbre was 3-19 dB darker than the bank; the Update-37
retune never reached the tables.** The block comment documents a retune to the
bank values, but the shipped arrays were still ~3 dB (C4 h2/h3) to 19 dB (D5
h10) below the bank. Measured deltas (bank − ours, h2…h10):
C4 +3.0/+3.6/+3.5/+4.8/+6.2/+8.0/+8.2/+7.3/+7.7,
G4 +7.4/+9.4/+7.1/+14.1/+9.6/+15.2/+10.9/+14.5/+12.1,
D5 +4.0/+11.1/+10.4/+13.5/+12.6/+15.8/+14.5/+17.7/+18.6,
G5 +7.6/+3.4/+10.1/+1.0/+12.6/+12.6/+12.6/+14.4/+9.9.
The four zone arrays are scaled by 10^(Δ/20) (h1 kept at 32768); the empirical
scaling absorbs the per-harmonic table→render offset (a smooth −0.3…−8 dB
roll-off that grows with harmonic index).
After (ours vs bank, dB rel h1, h2/h3/h5/h7/h10):
C4 −27.2/−5.4/−14.8/−29.1/−55.0 vs bank −27.4/−6.1/−16.8/−33.5/−59.7, and
D5 −36.4/−10.3/−28.0/−46.1/−65.7 vs bank −38.0/−15.1/−33.2/−52.3/−73.7 —
from a 3-19 dB shortfall to ≤5 dB on the strong partials (the residual sits on
h7…h10 which are −45…−70 dB rel h1).

### Method note (new rule for this family)
**Our wavetable body is true stereo with a right-channel delay; the bank is
not.** Any probe that averages (L+R)/2 comb-filters our spectrum and biases
every harmonic differently per key — this session's "the render deviates from
its own zone tables by up to 25 dB at D5" and "the D5 sustain wanders ±20 dB"
were both exactly that artifact: L and R were each dead flat, only the average
wandered. Probes for this family must use the **left channel only** (as
`panflute-probe.mjs` did) or per-channel magnitudes. Side finding from the same
analysis: our L and R drift in relative phase (~0.06 Hz at D5), so a mono
downmix beats — inaudible per channel, recorded as an open item.

### Verification
- `scripts/smoke-test-wasm.mjs`: PASSED (0 non-finite).
- Click regression across the family (prog 0/24/43/72/73/74/75/79/115):
  flute family 0 clicks; the remaining hits are the pluck onsets of the
  percussive programs (metric floor effect on decaying signals), unchanged
  from before the fix.
- `node scripts/build-web.js`: staged `web/generated/` = `dist/`.

### WASM (canonical: plain -Os, INTRA_PIANO_ALL_TABLES=ON)
`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **205,697 B** (was 205,486 at Update 39; +381 B for the paired
per-channel modifier arrays, −~200 B for the removed FM-vibrato lambda and
constants), md5 `e10ca855dea1b778d334cead261c40ab`,
IntraSynth.js 14,367 B, staged `web/generated/` = `dist/`.

### Open items
- Residual ≤5 dB on PanFlute h7…h10 at D5 (very low absolute level).
- Our wavetable L/R channels drift in relative phase (~0.06 Hz); per channel it
  is inaudible, but a mono downmix of the pan flute beats slowly. Separate
  investigation.
- G4 zone is still the darkest vs the bank on the upper partials (h7 +15 dB
  needed before the retune; ≈ −5…−8 dB now).

## Update 41 — Recorder puff and Whistle hiss (breath layers)

Owner: «Recorder пшикает! И свист вообще шумит ужасно!!!»

### Diagnosis

New probe `.scratch/noise-v41.mjs` (renders ours from `web/generated` and the
same MIDI through FluidSynth on the Titanic bank; band-resolved inter-harmonic
level, left channel only).

- **Whistle (GM 77/78)** still had the pre-Update-31 recipe: white noise with
  `cutoffMultiplier 30` (≈ unfiltered), level 0.02, envelope sustain 1.0.
  Against the bank: at D5 the 1.5-3 kHz band was **−64.5 dB rel. h1 vs the
  bank's −93.5**, 3-6 kHz −64.3 vs −98.3, 6-12 kHz −62.6 vs −104.2 — that is
  **+16…+35 dB too loud, and flat white up to 18 kHz**. The waveform said the
  same thing: `max|2nd difference|/RMS` = 0.79-0.83 (hiss), against 0.16 for
  the recorder.
- **Recorder (GM 74)** carried a 15 ms white-noise burst as its chiff. Time
  profile (Hann-windowed, see the method note below): ours peaked at −61.4 dB
  rel. h1 in the 80-120 ms window, the bank peaks at −70.6 and then **holds a
  nearly flat bed** (−84.9 at 1.2-1.6 s). Ours was a 28 dB puff-then-hiss, the
  bank a 14 dB swell with a 100 ms rise.

Both instruments were left behind when the flute family moved to tone-correlated
jet-band air (Update 31); the whistle never got the band shaping at all.

### Method note (important for this family)

A rectangular-window probe over a short window (~15-40 ms) reads **harmonic
leakage as tens of dB of fake "noise floor"** — the h1 sidelobes at 300-700 Hz
sit only 6 dB below the peak in a 15 ms window. An earlier pass of this probe
reported a 27 dB recorder excess at note-on that was pure leakage. Breath-floor
numbers must come from a Hann-windowed spectrum (10 Hz steps, ±45 Hz around
each harmonic excluded), as in `.scratch/probe-breath-ab.js`. The 131072-sample
Hann spectra (0.36 Hz bins) used for the steady-state bands are safe.

### Fix (`InstrumentLibrary.cpp`, via `.scratch/patch-breath-v41*.cjs`)

- **Whistle**: level 0.02 → **0.0015**; high-pass `min(0.90·f0, 1.2 kHz)`;
  second-order low-pass `min(3.0·f0, 2.8 kHz)`; `combGain 0.40` (air tied to
  the note period); envelope attack 0.02 → 0.05 s (sustain 1.0 kept).
- **Recorder**: high-pass `min(0.90·f0, 1.2 kHz)`; second-order low-pass
  `min(2.8·f0, 2.4 kHz)`; `combGain 0.40`; level 0.05 → **0.019**; envelope
  `{0.015, 0.14, 0.10, 0.05}` → `{0.10, 1.2, 0.70, 0.06}` (slow 100 ms swell,
  then an almost flat bed).

### Result

Recorder D4 (GM 74 key 62), Hann floor dB rel. sustain h1, window → ours/bank:

| window | 0-40 ms | 40-80 | 80-120 | 120-200 | 200-300 | 300-500 | 500-800 | 1.2-1.6 s |
|---|---|---|---|---|---|---|---|---|
| before | (leakage-limited) | | −61.4 | −65.8 | −69.0 | −73.7 | −79.2 | −89.2 |
| after | −84.7 | −74.6 | −69.9 | −74.1 | −75.9 | −79.0 | −82.0 | −84.7 |
| bank | −78.1 | −70.5 | −70.6 | −74.7 | −76.2 | −80.0 | −83.6 | −84.9 |

Whistle D5 (GM 78 key 74), steady-state bands dB rel. h1: 1.5-3 kHz ours
−93.4 / bank −93.5; 3-6 kHz −100.2 / −98.3; 6-12 kHz ≤ −110 / −104.2. At D4 and
D6 ours is now 0…8 dB *below* the bank instead of 30 dB above it.

Waveform check: whistle `max|2nd|/RMS` 0.796 → **0.003** (L) / 0.039 (R) at D4,
0.793 → 0.012 / 0.105 at D5; recorder 0.163 → 0.014 / 0.282 → 0.046. Click probe
(`.scratch/click-lr.mjs`): **0 clicks** on 74/76/78 across the register, and the
per-note RMS is unchanged (−22.0 dB whistle, −23.1 dB recorder), so only the
noise moved, not the loudness.

### WASM (canonical: plain -Os, INTRA_PIANO_ALL_TABLES=ON)

`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **205,733 B** (was 205,697 at Update 40; +36 B for the extra
high-pass/low-pass/comb stage arguments on the two layers), md5
`382ddba6c74a1b693bc403d916d02b54`, IntraSynth.js 14,367 B, staged
`web/generated/` = `dist/`. `scripts/smoke-test-wasm.mjs`: **PASSED**
(0 non-finite).

### Open items

- `NoiseSampler::generateWithEnvelope` writes the **full** per-channel amplitude
  into both channels, while the non-enveloped `GenerateStereo` writes ×0.5 per
  channel (the stated design). Every enveloped breath layer is therefore +6 dB
  against the non-enveloped path. Left as-is on purpose: the whole flute family
  was tuned with the enveloped path, so "fixing" it would drop every approved
  instrument by 6 dB. Known inconsistency, not a bug fix to smuggle in.
- Bottle (GM 76) still has the legacy flat white breath and sits 20-25 dB below
  the bank in the 0.5-3 kHz bands; nobody complained about it, left untouched.
- The recorder's first 40 ms stays quieter than the bank (−85 vs −78 at D4,
  −80 vs −61 at D5) — the bank's D5 sample has a strong early puff that is
  exactly the "пшик" being removed here.
- Whistle air is centred a bit high vs the bank at D6 (1.5-3 kHz −88.3 vs
  −92.4); within a few dB, not chased further.
## Update 42 — PanFlute attack/bloom/breath rebuilt; Bottle noise removed; Titanic vibrato depth cut; Recorder+Ocarina sample tabs

### Owner report this pass
«Теперь Pan Flute вообще не похоже. Мне кажется, была итерация, где тембр был
похож! И атаки так и нет! У атаки совсем другой тембр, это не про огибающую, там
нужно что-то новое! Никакого блум-слоя не слышу, обычное гудение. Дыхания тоже
никакого нет… Хорошее звучание наверное давали широкие гармоники, но ты их убрал
что ли? Там единственный недостаток был в периоде 16к семплов, но ради качества
можно и побольше поставить. У флейты титаника какой-то тембр неприятный, как
будто расстройка большая. Для Recorder и Ocarina тоже добавь семплы в UI под
спойлер. В Bottle тоже не надо этого шума. Белый шум и полосатый запрещены!»

### What shipped
- **Bottle (GM 76)**: the last legacy flat white-noise breath layer is **gone**
  (`patch-v42.cjs`); the timbre rests on the table's odd harmonics only.
- **PanFlute (GM 75) table period 16384 → 32768** (x-dependent grid step
  2.93 → 1.46 Hz per the owner's «период 16к семплов… можно и побольше»).
  Pitch is set by the read rate, so only the spectral grid/memory changes.
- **PanFlute breath restored, but tone-correlated, not white**: `NoiseSampler`
  with high-pass `0.6·f0`, low-pass `min(2·f0, 1.5 kHz)`, `combGain 0.40` on the
  note period, noise table 32768 → 65536 (repeat 1.37 s). The owner's "wide
  harmonics" (v35) were exactly this: air sewn into the partials.
- **PanFlute attack rebuilt** as three pieces instead of one envelope:
  1. a register-true tone envelope (`EnvelopeProfile`, 4 anchors C4/G4/D5/G5);
  2. a short **tonal splash** (h3/h4/h5, 6 ms rise / 30 ms decay) — the «атака с
     другим тембром» the owner asked for;
  3. a slow **h1 bloom/overshoot** (rises 0.10-0.30 s, τ 0.25-0.50 s) — the
     «блум-слой» the listener could not hear in v42c.
- **FluteClean (GM 43, "Titanic flute")**: `VibratoProfile` depth ×0.6
  (±11/±14/±26 c → ±6.8/±8.3/±15.6 c) — the owner's «как будто расстройка
  большая». Rate and onset delay untouched; `FluteHybrid` (115) keeps its own.
- **Recorder (74) and Ocarina (79) sample tabs** in the debug spoiler
  (`web/synth.js` `SAMPLE_TAB_PROG`), so clicking the tab also switches the
  synth to the matching GM program and the test-notes play the right timbre.

### Method: the attack was fitted numerically, not by ear
`.scratch/pf-atk.mjs` renders GM 75 (L channel only — the true-stereo body combs
in a mono average) and the same MIDI through FluidSynth/Titanic, then prints h1..h8
per window (0-15/15-30/30-60/60-120/120-250/250-500/500-1000/1000-1800 ms) plus
window RMS. Four passes were needed:

| pass | change | measured result |
|---|---|---|
| v42c | splash 0.65/0.55/0.85, bloom 0.70 flat | onset 0-15 RMS +6/+5/+12 dB hot (D5), h4/h5 up to +20 dB; bloom ~0 dB (inaudible) |
| v42d | register envelope + splash scaled by measured Δ + zoned bloom | bloom overshot instead (+8…+19 dB in 120-500 ms) |
| v42e | bloom amplitude ×10^(-excess/20); C4/G4 envelope pulled down | C4 onset ≈ bank; upper zones still early-hot |
| v42f/g | top-zone splash/bloom trimmed; breath level 0.22 → 0.052 | C4 within 2 dB; 0 clicks; breath at bank level |

Final C4 (key 60), window RMS dB rel sustain, ours → bank:
`-14→-12 | -14→-10 | -11→-10 | -4→-4 | +2→+2 | +2→+4 | +1→+1`.
Onset harmonics 0-15 ms (dB rel sustain h1), ours → bank:
h1 −22.6→−21.7, h3 −13.8→−14.2, h4 −23.8→−22.3, h5 −17.2→−16.4.

### Key finding: same-frequency overlays beat with the body (phase-fragile)
`BloomSampler` partials sit at exactly `k·f0`, i.e. the same frequency as the
wavetable partials, so they interfere instead of adding power. Measured slopes
reached **3.8 dB of output per 1 dB of layer amplitude** (C4 h5) and inverting
(D5/G5 h4/h5), which is why v42c's splash both read +20 dB hot and — after a
small cut — collapsed into a null (−30 dB). For those two zones the layer is now
removed (the bank keeps h4/h5 there at −42…−50 dB rel h1, inaudible). This is a
limitation of the current overlay mechanism, not of the target spectrum.

### Breath level (this one was the "пшик")
`.scratch/noise-v41.mjs` (Hann, inter-harmonic floor, L channel) showed v42c's
0.22 level was **+12.5 dB above the bank in every window** and put the
inter-harmonic floor 30-50 dB above it — the onset buzz, confirmed by
`.scratch/click-lr.mjs` (19-22 "clicks" at keys 74/79, `max|2nd|/rms` 2.1-2.4).
Scaling to 0.052 (the measured attack/sustain *shape* already matched: 19 dB
decay over the note vs the bank's 18) gives:

| window | 0-40 ms | 40-80 | 80-120 | 300-500 | 1.2-1.6 s |
|---|---|---|---|---|---|
| C4 ours | −50.4 | −51.2 | −53.6 | −68.2 | −69.3 |
| C4 bank | −50.6 | −43.9 | −45.0 | −62.7 | −68.7 |
| D5 ours | −53.2 | −50.5 | −51.7 | −65.6 | −68.6 |
| D5 bank | −57.8 | −52.3 | −55.3 | −64.7 | −76.2 |

Click probe after: **0 clicks** on 74/75/76/78/79 across the register
(`max|2nd|/rms` 0.19/0.54/0.64 for 75 at 60/74/79).

### Verification
- `scripts/smoke-test-wasm.mjs`: **PASSED** (0 non-finite).
- Sample tabs: `manifest.json` carries `Recorder` and `Ocarina`; `dist/samples/`
  contains both after `node scripts/build-web.js`.
- A/B rows for the flute family were re-checked for clicks after the vibrato cut
  (43 key 60/83 and 115 key 83: 0 clicks).

### WASM (canonical: plain -Os, INTRA_PIANO_ALL_TABLES=ON)
`sh scripts/build-wasm.sh` + `node scripts/build-web.js`:
IntraSynth.wasm = **206,528 B** (was 205,733 at Update 41; +795 B for the
PanFlute splash/bloom layers, the 4-anchor envelope and the 65536 noise table),
md5 `c8c9dee61bb96a4372ff8103c89a87b0`, IntraSynth.js 14,367 B, staged
`web/generated/` = `dist/` (both md5-identical).

### Open items
- D5/G5 onset h4/h5 stay +8…+22 dB above the bank in 0-15 ms, and G5 h1 +9 dB.
  These two are **body-table** values measured with the splash already at ~0
  (D5 table h4 −43 dB vs the bank's onset −42 but −47 sustain), i.e. a table/
  envelope follow-up, not a layer one. Absolute levels are low (−28…−40 dB rel h1).
- G5 h3 is 6 dB *below* the bank in 0-15 ms; D5 breath +4.5 dB hot at 0-40 ms.
- The same-frequency overlay beating above: a phase-locked variant (or a
  time-varying wavetable) would remove the per-harmonic guesswork entirely.

Artifacts: `.scratch/patch-panflute-v42{b,c,d,e,f,g}.cjs`, `.scratch/pf-atk.mjs`,
`.scratch/pf-atk-v42{c,d,e,f,final}.txt`, `.scratch/noise-v41.mjs`,
`.scratch/click-lr.mjs`.


## Update 43 — Pan Flute: воздух по регистру и без всплеска; Recorder: 7 зон семплов; Ocarina: снято вибрато

Три замечания владельца после Update 42: «Pan Flute C5 и выше всё равно
пшикают!», «Recorder совсем не похож, звучит примитивный тон», «Ocarina вообще
не так звучит». Плюс ответ на открытый пункт: треск/просадка D5/G5 в Update 42 —
это про **Pan Flute (GM 75)** (D5 = клавиша 74, G5 = клавиша 79), не про
Recorder/Ocarina.

### 1. Recorder (GM 74): таблицы по фактическим границам семплов банка

Банк меняет семпл каждые 2-3 клавиши, а у нас было 4 опорные точки с
лог-интерполяцией — там, где банк уже переключился, тембр уезжал до **14 дБ**
(клавиши 70-73 и 78-80). Проверено по `.scratch/oc-rec-sf2map.js` (пресет →
инструмент → зона → семпл): Recorder-D3 0..66, A4 67..69, B4 70..71, C#5 72..73,
D5 74..77, A5 78..80, A#5 81..105. Значения — рендер банка (L-канал) в нижней
клавише зоны.

Замер `.scratch/oc-rec-tuneL.mjs`, |наши − банк| по h2..h8, дБ:

| клавиша | 62 | 67 | 70 | 72 | 74 | 79 | 82 |
|---|---|---|---|---|---|---|---|
| было (v43) | 0.5 | −1.5 | −8.9…+10.4 | −5.9…+14.2 | 0.5 | 4.6…8.7 | 3.0…14.2 |
| стало | 0.4 | 2.6 | 3.2 | 0.9 | 1.9 | 4.3 | 5.1 |

**Найденный баг границ:** целые границы зон не работают. Нота клавиши 70 приходит
в генератор с частотой чуть ниже равномерной темперации (466.0 Гц, key ≈ 69.9996),
поэтому `key < 70.0f` выбирал таблицу A4 вместо B4; так же клавиша 74 играла
C#5 вместо D5. Границы сдвинуты на полтона (66.5 / 69.5 / 71.5 / 73.5 / 77.5 /
80.5), у окарины — 62.5 / 67.5 / 92.5.

### 2. Ocarina (GM 79): «вообще не так звучит» = вибрато

Таблицы окарины были верны (замерены с банка), а рендер читался на **16 дБ ниже
собственной таблицы** по h4/h5. Причина не в «размазывании линии 5 Гц»: при
относительном отклонении скорости 0.3 % FM-индекс k-й гармоники β = k·0.5, и
несущая падает как J0(β) — h4 (β = 2) −13 дБ, h5 (β = 2.5) −26 дБ. Замер
подтвердил: клавиша 80, h4 −71.0 против табличных −54.6, h5 −62.0 против −45.2.
Вибрато снято (в рендере банка линии 5 Гц нет), зоны — по клавише.

| клавиша | 62 | 63 | 66 | 68 | 80 | 93 |
|---|---|---|---|---|---|---|
| было (v43) | до +6.7 | — | до +11.9 | до 12.2 | до +16.8 | до +25.2 |
| стало | 1.2 | 1.6 | 0.8 | 1.9 | 1.3 | 4.8 |

### 3. Pan Flute (GM 75): «пшик» выше C5 — это ФОРМА дыхания

Прежняя огибающая дыхания давала на каждом note-on 30-мс всплеск до 1.0 при
подушке 0.12, то есть в **8 раз громче подушки** — короткий «пшик». Теперь рост
медленный (120 мс) без всплеска, подушка 0.60, гребёнка углублена 0.40 → 0.85
(энергия уходит в юбки гармоник, а не в пол между ними), ФНЧ 2-го порядка с
потолком 1.8 кГц, уровень привязан к собственному спаду банка по регистру
(опорные точки C4/G4/C5/G5 = 1.0/1.0/0.32/0.28 от базы 0.075).

Замер `.scratch/noise-v41.mjs` (полосы, дБ отн. сустейна h1):

| клавиша | 60 | 67 | 72 | 79 | 84 | 90 |
|---|---|---|---|---|---|---|
| наши 0.5-1.5к | −58.1 | −54.6 | −64.8 | −69.8 | −72.2 | — |
| банк 0.5-1.5к | −55.2 | −55.7 | −64.9 | −68.7 | −66.4 | — |
| наши 1.5-3к | −65.8 | −60.7 | −67.9 | −69.5 | −69.4 | −69.9 |
| банк 1.5-3к | −63.1 | −60.7 | −62.1 | −67.6 | −71.1 | −73.3 |
| наши 3-6к | — | −65.6 | −74.3 | −76.7 | −75.1 | −75.2 |
| банк 3-6к | — | −71.6 | −75.6 | −82.5 | −75.7 | −77.6 |

### Честные открытые пункты

- Межгармонический пол (10-й процентиль 1.35·f0…9·f0) остаётся на **20-30 дБ**
  выше банка (C5: −84.6 против −114.2), хотя полосы 0.5-6к теперь совпадают.
  Гребёнка с задержкой в один период принципиально даёт юбки шириной ~f0/2, а у
  банка энергия сидит теснее (у «чистой» окарины тот же пол — −164 дБ, то есть
  всё это шум нашего дыхания). Если «пшик» останется, следующий шаг —
  фазово-привязанный к гармоникам шум или времязависимая вейвтаблица.
- Recorder: банковский «чифф» (3-10 кГц в первые 40 мс) у нас всё ещё на 20-27 дБ
  тише, а onsets громче/раньше банковских. Уровни семейства флейт в целом на 7-9 дБ
  выше банка при `-g 0.6` (Flute GM 73 тоже), поэтому пересчёт уровней намеренно
  НЕ делался. Дыхание блокфлейты поднято 0.019 → 0.030 при гребёнке 0.40 → 0.80 —
  если владелец снова услышит «пшик», это единственная ручка.
- Инструменты проверены только замером: ни одного из трёх владелец ещё не слушал.

### Верификация

- `scripts/smoke-test-wasm.mjs`: **PASSED** (0 non-finite).
- `node .scratch/click-lr.mjs`: **0 клик-событий** на 74 (62/74/82), 75
  (60/72/74/79/84), 79 (62/80/93).
- Семпл-вкладки Recorder/Ocarina в UI на месте (`manifest.json`).

### WASM (канон: plain `-Os`, INTRA_PIANO_ALL_TABLES=ON)

`sh scripts/build-wasm.sh` + `node scripts/build-web.js`: IntraSynth.wasm =
**206,915 B** (было 206,528 на Update 42), md5 `78d5d915f670cdf745bdd6f891a6df35`,
IntraSynth.js 14,367 B, `web/generated/` = `dist/` (md5 идентичны).

Артефакты: `.scratch/patch-woodwinds-v43{c,d,e}.cjs`, `.scratch/oc-rec-tuneL.mjs`,
`.scratch/oc-h4-time.mjs`, `.scratch/noise-v41.mjs`.


## Update 44 — Pan Flute: юбка вперёд тона в атаке; Recorder: воздух ниже f0 и зона A#5 по среднему замеру; Ocarina одобрена владельцем

Замечания владельца после Update 43: «У Pan Flute нет атаки, сразу основной тон
и всё... в оригинале в начале как будто сначала должна появляться юбка, а потом
появляться основной тон»; «Recorder не похож, примитивно звучит, а C6 ещё и
неприятный шум, как будто алиасинг какой-то»; «Ocarina то что надо, очень похожа!».
Ocarina не тронута.

### Новый инструмент замера: `.scratch/wind-attack.mjs`

Раскладывает окна атаки (0-15/15-30/30-60/60-120/120-250/250-500 мс) на ТОН
(сумма гармоник f0..f8 по Гёрцелю с автоопределением f0 ноты) и ЮБКУ (остаток RMS),
плюс сустейн h1..h16. Второй инструмент — `.scratch/spectrum-dump.mjs` (полосы по
100 Гц, наши vs банк) показал главную разницу тембра блокфлейты.

### Pan Flute: воздух первым, тон вторым

Замер G5 (дБ отн. сустейна h1):

| окно | наши (v43) | банк |
|---|---|---|
| 0-15 мс, юбка | нет вовсе | −23.9 |
| 0-15 мс, h1 | −35.2 | −38.7 |
| 15-30 мс, h1 | −31.7 | −22.8 |
| 30-60 мс, h1 | −28.0 | +1.3 |
| 60-120 мс, h1 | −3.7 | +8.0 |

То есть у банка юбка уже есть в первом окне, а h1 приходит позже и добирает
уровень за 40-60 мс. У нас было наоборот: юбки нет, h1 идёт почти линейным
раздувом. Изменено:
- дыхание: вход 120 → **8 мс** (пик 1.0, подушка 0.50) — воздух слышен с первых
  миллисекунд;
- тело в D5/G5: пауза 12/22 мс, быстрый подъём (v1 0.06 при t1 6-12 мс) и короткое
  экспоненциальное добирание (t2 0.014-0.050) — h1 выходит вперёд только после юбки;
- вспышка h3/h4/h5 в D5/G5 возвращена (в Update 42e её убрали): в C5-зоне срезана
  до 0.05/0.03/0.02 (там наша сумма гармоник в 0-15 мс была на 6.7 дБ громче банка
  при совпавшем h1), в G5 усилена до 0.75/0.55/0.45 (там наоборот: сумма гармоник
  банка на 6 дБ громче нашей при h1 на 3 дБ тише).

Абсолютная огибающая (env5ms, `.scratch/oc-rec-tuneL.mjs`) после правки:
C5 — наши и банк в пределах 1-3 дБ по всем окнам; G5 — начало у нас на 5-7 дБ
громче банка, сустейн на 7-9 дБ тише (это уровень зоны, не атака).

### Recorder: воздух ниже f0 и зона A#5

``.scratch/spectrum-dump.mjs` на C6 показал, что у банка есть широкая подложка
**ниже f0** (400-700 Гц на −44…−53 дБ отн. h1), а наша обрезалась ФВЧ 0.9·f0
(941 Гц) — оставалась тонкая полоса, которую слушатель описывает как «неприятный
шум, как будто алиасинг». ФВЧ опущен до 0.55·f0 (потолок 700 Гц), вход воздуха
ускорен 100 → 30 мс. Подложка 400-700 Гц поднялась на ~6 дБ (осталось 13-17 дБ
разрыва — см. открытые пункты).

Зона A#5 (клавиши 81..105) была построена по ОДНОЙ клавише 82, а внутри зоны h2
плывёт от −59.7 (81) до −36.5 (85). Опора на «холодную» точку (h2 −57 при h3 −32)
давала полый тон — вероятная причина жалобы на C6. Таблица пересобрана по
**среднему по амплитуде** замеров клавиш 83/84/85/86/88/90:
−39.7/−33.7/−54.7/−42.0/−62.5/−62.9/−77.1 дБ.

| клавиша | 84 | 86 | 90 |
|---|---|---|---|
| \|наши − банк\| до (v43) | 12.9 | 13.0 | 16.4 |
| после Update 46 | 8.1 | 8.4 | 16.4 |

### Честные открытые пункты

- Остаточные 8-16 дБ на клавишах 84-92 — это РЕАЛЬНЫЙ разброс самого семпла
  банка по клавишам (его луп при транспозиции даёт разный тембр); одной таблицей
  на зону это не лечится, нужны подзоны 81-85 и 86-105 по отдельным замерам.
- Подложка ниже f0 у банка на C6 всё ещё на 13-17 дБ сильнее нашей — ФВЧ можно
  опустить ещё, но это уже баланс «воздух против rumble» и требует прослушивания.
- Pan Flute G5: сустейн на 7-9 дБ тише банка (уровень зоны, не атака), а начало
  на 5-7 дБ громче — «юбка первая» достигнута, но абсолютные уровни окна 0-15 мс
  всё ещё выше банковских.
- Разложение тон/юбка у банка недостоверно на коротких окнах (партиалы семплов
  слегка негармоничны, окно 15 мс не разрешает их) — по атаке Pan Flute ориентир
  был на h1 и на абсолютный RMS, а не на «юбку» из пробы.
- Инструменты по-прежнему проверены только замером; Ocarina одобрена владельцем,
  Pan Flute и Recorder — нет.

### Верификация

- `scripts/smoke-test-wasm.mjs`: **PASSED** (0 non-finite).
- `node .scratch/click-lr.mjs`: **0 клик-событий** на 75 (60/72/79/84), 74
  (62/74/84/88/92), 79 (80).
- `web/generated/` = `dist/` (md5 идентичны).

### WASM (канон: plain `-Os`, INTRA_PIANO_ALL_TABLES=ON)

`sh scripts/build-wasm.sh` + `node scripts/build-web.js`: IntraSynth.wasm =
**206,915 B** (без изменений с Update 43 — правки только в данных и огибающих),
md5 `23d7806dae3a2c8ad6802d94abed62a6`, IntraSynth.js 14,367 B.

Артефакты: `.scratch/patch-woodwinds-v44.cjs`, `v45.cjs`,
`.scratch/patch-recorder-a5-zone-v46.cjs`, `.scratch/wind-attack.mjs`,
`.scratch/spectrum-dump.mjs`.


## Update 47 — Пан-флейта: таблицы по семплам банка и полоса воздуха; блокфлейта C5/C6; атака свистка; уровень окарины

Замечания владельца после Update 44: «Чего-то не хватает pan flute, хотя вроде
ближе. И C6 писклявее, чем оригинал, как будто не хватает юбки. Юбка только на
одной гармонике должна быть или на всех? Ты сравнил снова то что получилось, с
банком? Возможно, надо замерять несколько гармоник, и сделать несколько
итераций правок и сравнения, чтобы всё совпало. Recorder только на C4 вроде
похож. На C5 вообще тембр не тот. Как будто широкая юбка, которой быть не
должно, ещё и осциллирует очень быстро. А на C6 вообще посторонний шум, видимо
юбка слишком широкая. Ocarina вроде по громкости не та, наверное слишком
громкая. У Whistle вроде у нас атака более заметная, у оригинала она плавнее.»

### Новые пробники (все замеры — L-канал, банк Titanic 200 GM-GS v1.2, fluidsynth -g 0.6)

- `.scratch/skirt-v47.mjs` — по КАЖДОЙ гармонике: пик (дБ отн. h1), форма юбки
  (уровень сдвигов ±Δ·f0 в долях от своего пика) и абсолют.
- `.scratch/spec-bands.mjs` — 1/6-октавная огибающая 300 Гц…14 кГц, режим
  `EXCL=1` выкидывает бины ±40 Гц вокруг гармоник (чистая подложка).
- `.scratch/bins-near.mjs` — 25-Гц полосы вокруг гармоники (что такое «юбка»).
- `.scratch/env-stability.mjs` — разброс RMS по 25/100-мс окнам сустейна
  (ловит «шиммер» периода таблицы).
- `.scratch/bank-time.mjs` — держит ли банк уровень: у блокфлейты и окарины
  семплы НЕ зациклены, они затухают (блокфлейта −20 дБ за 3.7 с), у пан-флейты
  и свистка уровень ровный. Поэтому абсолютный RMS сверять с банком можно
  только на старте ноты.

### Пан-флейта (GM 75)

1. **Таблицы зон были не по тем семплам.** В банке всего ТРИ семпла
   (`.scratch/oc-rec-sf2map.js`): `panflutea3la` клавиши 0..70,
   `panfluted4la` 71..78, `panfluteg4la` 79..108. У нас были 4 «октавные»
   опоры C4/G4/D5/G5 с лог-интерполяцией, которые мешали профили через границы
   семплов, а значения снимались усреднением L+R — гребёнка L+R занижает
   чётные гармоники (та же ошибка, что у блокфлейты/окарины в Update 43).
   Теперь жёсткий выбор зоны по клавише и профили = рендер банка по L-каналу
   в опорных клавишах (60/67/70, 72/74/78, 79 и 91 — внутри g4la профиль
   сползает от транспозиции). Замер до/после на C6 (клавиша 84, дБ отн. h1):

   | | h2 | h3 | h4 | h5 | h6 | h7 | h8 |
   |---|---|---|---|---|---|---|---|
   | было (v44) | −23.5 | −29.0 | −45.1 | −59.1 | −54.3 | −55.7 | −62.7 |
   | стало | −30.5 | −32.4 | −52.9 | −58.1 | −59.3 | −62.7 | −65.8 |
   | банк | −31.8 | −34.0 | −57.5 | −63.6 | −68.4 | −69.8 | −89.5 |

   h4/h6/h7/h8 были на +12…+24 дБ выше банка — это и есть «C6 писклявее
   оригинала»: не юбки не хватало, а лишние острые верхние партиалы.
   На C4/C5 таблицы сошлись в ±3 дБ по h2..h7.

2. **Дыхание.** Прежние ФВЧ/ФНЧ ехали за f0; у банка струйная полоса стоит НА
   МЕСТЕ (≈600 Гц…2.4 кГц) при любом регистре, и именно она читается как
   широкая юбка на верху. Теперь ФВЧ max(0.9·f0, 550), ФНЧ min(6·f0, 2400),
   4 полюса, гребёнка 0.85 → 0.12, уровень ровный 0.033 (был спад до 0.28 на
   верху). Итог по 1/6-окт (EXCL=1): расхождение с банком в полосе 0.3-3 кГц
   ±5 дБ от C4 до C6 (было +15…+26 дБ выше 3.8 кГц на C6).

3. **Почему юбки всё же не хватает, и что это вообще такое.** Замер
   `bins-near.mjs` (25-Гц полосы) показал: у банка вокруг КАЖДОЙ гармоники
   есть горб на 8-15 дБ выше подложки, шириной ±12-16% частоты ЭТОЙ гармоники
   (то есть в центах, а не в герцах — ответ на вопрос владельца: юбка на всех
   гармониках, и у высоких она шире). Физически это резонанс трубы (Q≈3-5), у
   нас гармоника — острая линия, поэтому на C6 не хватало ещё 8-14 дБ в полосах
   850-1200 Гц. Проверены две модели:
   - статичная юбка (ядро + размытая копия гармоники, `WithSkirt`, 200 центов)
     даёт нужный горб, НО: на C4 возвращает шиммер периода таблицы 1.7 дБ
     (`env-stability`: у банка 0.25 дБ) — ровно то, за что владелец уже
     отбраковывал таблицу 16384; и вокруг сильных h2/h3 перелетает (+18 дБ на
     1905 Гц на C6, где банк −72.5). Откачено.
   - гребёнка (она же «осцилляция»): наоборот, ПЛОХА для юбки — при g=0.45 её
     провал на ±15% от гармоники опускает плечи НИЖЕ ровной подложки. Поэтому
     гребёнка пан-флейты опущена до 0.12.
   Остаточный разрыв (850-1200 Гц на C6: мы −64…−66 при банковских −52…−60)
   честно остаётся. Настоящее лечение — не статичная таблица, а медленная
   случайная модуляция высоты/амплитуды (естественная «неустойчивость»
   живого инструмента), то есть правка движка чтения таблицы, а не данных.

### Блокфлейта (GM 74)

- Замер `spec-bands.mjs EXCL=1`: на C5 мы были на +3…+11 дБ громче банка по
  всей полосе 0.3-2.4 кГц, на C6 — на +6…+12 дБ выше 7 кГц. «Широкая юбка,
  которой быть не должно, ещё и осциллирует очень быстро» — это гребёнка 0.80:
  она превращала воздух в амплитудную модуляцию на f0 (523 Гц на C5 — «очень
  быстро»). Теперь гребёнка 0.15, уровень 0.030 → 0.018 (с +3.5 дБ на верху),
  ФВЧ 0.75·f0, ФНЧ 3 полюса 2.2 кГц. Итог: C5 — ±3 дБ от банка в 0.3-3 кГц,
  C6 — ±7 дБ в 0.3-3.8 кГц (было +3…+11). Юбку-горб блокфлейте НЕ ставили:
  владелец как раз жаловался на слишком широкую юбку.

### Свисток (GM 78) и окарина (GM 79)

- Свисток: атака тона 10 → 70 мс. Замер `wind-attack.mjs` (дБ отн. сустейна):
  было 0-15 мс −2.3 (мгновенный фронт) при банковских −19.0; стало −18.4 при
  −18.5, 15-30 мс −8.9 при −7.3. Банк дальше перелетает сустейн (+4.8 дБ к
  60-120 мс) — этого у нас нет, но фронт теперь плавный, как просил владелец.
- Окарина: уровень 0.30 → 0.085. Замер (ключ 74, RMS сустейна): было
  −16.1 dBFS при банковских −34.5; стало −27.0, то есть ровно на уровне нашей
  пан-флейты на той же клавише (−27.5), как и в банке, где они равны.

### Проверка

- `scripts/smoke-test-wasm.mjs` — PASSED (0 non-finite).
- Клики (`.scratch/click-lr.mjs`): 0 на 75 (60/72/84), 74 (62/72/84),
  79 (80), 78 (79).
- Стабильность сустейна (`env-stability.mjs`, окна 100 мс): пан-флейта
  0.43/0.17/0.07 дБ разброса на C4/C5/C6 (банк 0.25/0.20/0.83) — шиммера нет.
- `web/generated/` = `dist/` (md5 идентичны), окарина/блокфлейта в UI не
  трогались.

**WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 207 140 байт**,
md5 `bb69c1684d9c4d31b2c8620c7d80a040`, IntraSynth.js 14 367 B.

### Открытые пункты

- Юбка-горб вокруг гармоник (8-15 дБ над подложкой, ±12-16% частоты) у нас
  остаётся на 8-14 дБ тише банка на C6: статичная таблица её воспроизводит
  только ценой шиммера и перелёта у сильных гармоник (см. выше). Нужна правка
  движка (медленный random-walk высоты/амплитуды при чтении таблицы).
- Верхний край блокфлейты (4-13 кГц) у нас на 5-11 дБ тише банка, а у
  пан-флейты на 5-15 дБ тише выше 6 кГц — там у банка шум семпла; не трогали.
- Абсолютные уровни всего семейства духовых по-прежнему на 3-14 дБ выше банка
  при `-g 0.6` (пан-флейта +3.4, свисток +13.7); калибровали только окарину —
  её владелец и просил.


## Update 54 — Пан-флейта: атака с дыханием вернулась (гребёнка+чифф); блокфлейта: h9..h16 и без шипящей подложки наверху; окарина громче

Замечания владельца после Update 53: «У Pan Flute не слышу атаки. Только гудение
относительно похоже, но там весь сок именно в особенном звучании атаки с
дыханием. Вроде в прошлый раз какой-то намёк на ту атаку был, а теперь исчез.
Recorder какой-то у нас слишком тёмный, оригинал ярче и богаче гораздо! А ещё на
высоких октавах ненавистный белый шум или что-то похожее слышится! А там вообще
нет шума, да и дыхание там вроде не нужно делать. Теперь у нас Ocarina слишком
тихая, вообще не слышно! Как будто ты сделал в 10 раз тише, а надо было в 2
раза!»

### Новый пробник

- `.scratch/atk-spec.mjs` — 1/6-октавные полосы (250 Гц…5.7 кГц) в КОРОТКИХ
  окнах после note-on (0-40, 40-100, 100-200, 200-400 мс), A/B с банком одной и
  той же мерой. Старый `wind-attack.mjs` оценивает юбку как
  `sqrt(rms² − tone²)`, и там, где тон внутри окна быстро меняется, оценка
  вырождается (`−240` = ноль) — по ней нельзя судить об атаке. atk-spec таких
  вычитаний не делает, поэтому именно он вынес приговор.

### Пан-флейта: почему атаки не было

Замер atk-spec, окно 0-40 мс, Δ = наши − банк (дБ отн. h1 сустейна):

| | 700-1200 Гц | 1.4-2.5 кГц | 2.5-5.6 кГц |
|---|---|---|---|
| C4 (Update 53) | −4…+7 | −9…−14 | −11…−19 |
| C5 (Update 53) | +14…+19 | −5…+4 | −5…−9 |
| C6 (Update 53) | −12…−25 | −13…−25 | +16 (4 кГц) |

Сустейн (spec-bands EXCL=1, T0=2.0) на C4 был на 5-14 дБ тише банка по всей
полосе 300-5400 Гц. Причина одна: в Update 53 гребёнка воздуха опущена
0.85 → 0.12, то есть шум перестал липнуть к гармоникам и лёг ровной подложкой.
В банке атака — это ЮБКИ: в окне 0-40 мс у него 891 Гц −12 при h1 −40, h3
(3175 Гц на C6) −0 при сустейне −29, а между гармониками провалы −41…−43.

Что сделано (все значения — Update 54):

- **гребёнка 0.12 → 0.85** (feedback-comb на периоде ноты). Она и даёт
  банковскую форму; замер C5: пики 445-500 Гц и 1260-1587 Гц совпали с банком
  за 1-4 дБ.
- **уровень 0.033 → 0.030** и регистровый спад `lvlK = {1.15, 1.05, 0.95, 0.85}`.
  Первая попытка была «поднять воздух на +7 дБ» (0.070) — и это пересветило
  атаку на +12…+24 дБ: у ВЫХОДА гребёнки пик на гармонике усилен
  1/(1−g) = +16 дБ, поэтому «тихий» вход 0.030 после гребёнки и даёт
  банковские пики. Проверено экспериментом: с уровнем 0.0 атака уходит на
  20-50 дБ ниже банка (значит слой нужен), с 0.070 — пересвет.
- **полоса фиксированная по абсолютной частоте**: ФВЧ 420 Гц (было
  `max(0.9·f0, 550)`: на C6 это 941 Гц, то есть глушился ровно тот участок
  800-1000 Гц, где у банка самая громкая атака), ФНЧ 2000..2600 Гц (было 4200:
  на C6 мы были на +22 дБ громче банка на 3.8 кГц).
- **вспышка дыхания на атаке**: `breathEnv = {0.006, 0.10, 0.40, 0.10}` —
  вход 6 мс, пик 1.0, спад до 0.40 за 100 мс (τ≈22 мс). Это слышимый «чифф»,
  а не плавное вползание (банк в 0-40 мс держит юбки на 5-20 дБ громче своего
  сустейна).
- **вспышка верхней зоны подрезана**: h4/h5 в зоне G5+ 0.55/0.45 → 0.26/0.20,
  h3 0.75 → 0.90 (на C6 мы были +16 дБ на 4 кГц при банковских −27, а его h3 в
  атаке почти на сустейне).

Итог замера 0-40 мс после правок: C4 — Δ в ±10 дБ по всей полосе (было −19),
C5 — пики в ±4, но провалы между h1 и h2 (794-1122 Гц) остаются на +8…+17,
C6 — 500-1600 Гц в ±11 (было −25), выше 2.5 кГц по-прежнему −17…−23.
Стабильность сустейна (env-stability, WIN=0.1): 0.53/0.31/0.20 дБ разброса на
C4/C5/C6 — «шиммера» периода таблицы нет (банк 0.25/0.20/0.83).

### Блокфлейта: почему «тёмный» и откуда «белый шум»

- **Темнота — измеримая**: таблицы обрывались на h8. Наши h9..h16 читались
  −85…−105 дБ (это утечка зонда, гармоник не было вовсе), у банка — −49…−80
  (у него формант на 3.3-4.7 кГц: на клавише 72 h9 = −47 при h8 = −62, то есть
  h9 ГРОМЧЕ h8). Таблицы всех семи зон расширены до h16 по замеру банка
  (h9 — измеренный, дальше −3 дБ на гармонику, пол −92 дБ). Проверка после
  правки (wind-attack, h9..h16 ours vs bank): клавиша 62 −59…−81 против
  −59…−76; 72 −47…−69 против −47…−76; 84 −76…−97 против −85…−107.
- **Белый шум наверху**: ровная подложка (гребёнка 0.15) на C5/C6 читается как
  шип. Теперь гребёнка 0.55, а уровень гаснет по регистру
  `recLvlK = {1.0, 0.55, 0.12}` (C4/C5/C6) — владелец просил «там вообще нет
  шума, и дыхание не нужно». Богатство верхнего регистра теперь дают реальные
  h9..h16, а не шум.
- Верхняя зона A#5: наши h9/h10 (9.4/10.5 кГц на C6) оказались на +9/+6 дБ
  выше банка — подрезаны до замеренных (−84/−85).

### Окарина

0.085 → **0.15** (владелец: «в 10 раз тише, а надо было в 2 раза»; 0.30/2).
Честно: по замеру (skirt-v47, RMS сустейна) мы теперь на +12 дБ выше сухого
рендера банка при -g 0.6, тогда как пан-флейта +3.4, а блокфлейта от +20.
Инструменты семейства в банке почти равны, так что если окарина окажется
громче остальных — следующий шаг 0.15 → 0.10.

### Верификация

- `scripts/smoke-test-wasm.mjs` — PASSED (0 non-finite).
- `click-lr.mjs`: 0 кликов на 75 (60/72/74/79/84), 74 (62/72/84/88),
  79 (62/80/93).
- `web/generated/` = `dist/` (md5 идентичны).
- **WASM (канон: plain -Os, INTRA_PIANO_ALL_TABLES=ON) = 207 808 байт**,
  md5 `2d1e299d125a9f6f8d41f1efae000bf2`, IntraSynth.js 14 367 B.

### Честные остатки

- Провалы между h1 и h2 у пан-флейты на C5 (+8…+17 дБ) — свойство движка:
  feedback-гребёнка на одном периоде даёт глубокие ПИКИ (+16 дБ) и лишь
  −5 дБ в провалах (1/(1+g) при g=0.85). Банковские узкие юбки требуют банка
  резонаторов/фазово-привязанного шума — это правка движка, а не данных.
- Выше 2.5 кГц в атаке C6 мы всё ещё на 17-23 дБ тише банка.
- Слушать не могу: все правки по замеру, ваши уши — финальная проверка.


## Update 55 — Pan Flute: длинная атака; Recorder: яркость и вибрато

Замечания владельца после Update 54: «У Pan Flute C5 уже начала какая-то
похожая атака вырисовываться, но слишком короткая по сравнению с банком.
У C4 её не слышно. Recorder всё ещё тёмный, а ещё у оригинала есть вибрато,
а у нас ровный тон. Шума уже вроде нет.»

### Pan Flute (75) — «чифф» был в 4-8 раз короче банковского

Замер по окнам атаки (`.scratch/noise-v41.mjs ATK=1` — межгармонический пол,
Hann, дБ отн. сустейна h1; окна 0-40/40-80/80-120/120-200/200-300/300-500/500-800 мс):

| ключ | было (наш) | стало | банк |
|---|---|---|---|
| C4 (60) | −52.7/−56.7/−62.7/−67.1/−67.6/−71.8/−68.0 | −50.5/−44.2/−47.9/−53.1/−55.2/−61.8/−62.7 | −50.6/−43.9/−45.0/−52.1/−56.8/−62.7/−64.7 |
| C5 (72) | −58.4/−64.7/−68.2/−72.5/−73.2/−75.6/−78.3 | −55.3/−51.7/−52.4/−57.8/−60.0/−66.0/−73.3 | −60.1/−50.4/−54.5/−61.9/−62.1/−65.8/−68.4 |
| C6 (84) | −62.0/−65.7/−68.6/−73.2/−74.6/−78.4/−78.9 | −54.4/−55.5/−56.7/−62.7/−66.5/−74.3/−78.6 | −54.2/−57.0/−56.2/−65.7/−68.5/−72.5/−78.4 |

У банка подложка ГРОМЧЕ всего не в 0-40 мс, а в 40-200 мс, и держится 10-25 дБ
выше сустейна почти до 0.5-0.8 с; у нас спад заканчивался к 120 мс (вход 6 мс,
спад 0.10 с) — это и была «слишком короткая атака», и на C4 она не читалась
вовсе (−17.7 дБ в окне 80-120 мс).

Сделано: вход 6 → 45 мс (на G5+ 14 мс — у банка на C6 пик уже в первом окне),
экспоненциальный спад 0.10 → 0.42-0.68 с, полка 0.40 → 0.19-0.26, уровень
0.030 → 0.045 с новой кривой по регистру (1.90/1.85/1.85/1.25). Итог: C4 в
пределах ±3 дБ от банка во всех окнах (было до 18), C5/C6 — ±5 дБ.

### Recorder (74) — «тёмный» = ФНЧ дыхания ехал за нотой, + не было вибрато

Две независимые причины, обе — замером (`.scratch/spec-bands.mjs EXCL=1 T0=1.4`,
полосы 600 Гц..10 кГц, дБ отн. h1):

1. **ФНЧ дыхания был привязан к ноте**: `min(4.5·f0, 2200)` — на C4 это
   1322 Гц (4.5 гармоники!), поэтому выше 2.7 кГц мы были на 13-20 дБ тише
   банка. Теперь полоса ФИКСИРОВАННАЯ 2600 Гц. Плюс гашение уровня по регистру
   1.0/0.55/0.12 (введённое в Update 54 от «белого шума») срезало подложку
   вчетверо — замер показал, что банк делает блокфлейту ТЕМ ШУМНЕЕ, чем выше
   нота: у него в 600 Гц..10 кГц −70.5 дБ на C4 и −57.1 на C6. Кривая
   перевёрнута: 0.65/1.25/1.35/1.60 по ключам 62/67/72/79/84/88 (было
   +3.1/−2.4/−3.9/−4.3/−9.5/−8.8 дБ рассогласования, стало ±2.3).
2. **Вибрато банка**: `.scratch/rec-vib.mjs` (траектория f0 окнами 25 мс,
   периодограмма) — ровно 5.0 Гц на всех клавишах, глубина ±17.3 (62) /
   ±19.7 (67) / ±15.6 (72) / ±13.9 (79) / ±13.8 (84) центов, вход с ~0.25 с.
   Добавлен `VibratoProfile`: команда 4.65 Гц и ±15.9…±13.2 цента (доставка
   у нас ≈1.07x по частоте и ≈1.06-1.15x по глубине) — на выходе 4.75/4.84/4.89 Гц
   и ±18.5/±15.3/±13.3 цента.

### Честно

- Наш Recorder по абсолютному уровню на +18…+23 дБ громче банковского
  (`.scratch/atk-spec.mjs`: наш h1 −19.4/−18.6/−18.2 dBFS при банковских
  −37.8/−38.0/−40.8). VolumeScale не трогал — владелец на громкость блокфлейты
  не жаловался, а «тёмный» лечится относительной АЧХ. Меж тем всё семейство
  духовых у нас выше банка: Pan Flute +3.7, Flute +9.6, Ocarina +11.4,
  Whistle +13.9 дБ — это отдельная задача про баланс семейства.
- C6 пан-флейты: сустейн подложки на 7 дБ громче банковского (банк там
  падает до −87.6, мы держим −80.3).
- Вибрато блокфлейты задано плоской глубиной по регистру с одним наклоном;
  у банка разброс ±13.8…±19.7 без явной закономерности.

### Верификация

- `scripts/smoke-test-wasm.mjs` — PASSED (0 non-finite).
- Клики (`.scratch/click-lr.mjs`): 0 на 74 (62/72/84/88) и 75 (60/72/84).
- `web/generated/` = `dist/` (md5 совпадают).
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 208 114 байт**,
  md5 `a035429793eccea56d9e9252210ac3e3`, IntraSynth.js 14 367 B.


## Update 56 — Pan Flute: широкая юбка в атаке и дыхание в сустейне; Recorder: яркая атака и вибрато, которое не съедает гармоники

Владелец: «Pan Flute уже почти как надо! Запомни это как удачную версию, чтобы
откатиться, если что. Только мне кажется, что как будто ширина юбки по мере
атаки должна спадать, иначе слегка чувствуется что-то типа шума... И юбка
частично должна остаться в сустейне, сейчас недостаточно дыхания в нём... И в
C6 у Pan Flute в оригинале атака вроде дольше, чем у нас» + «Recorder C6 в
оригинале после раскачки вибрирует быстрее. И в C4 в начале звук более яркий в
оригинале, чем у нас. У нас ровный тон, а там он в него переходит не сразу. И
всё равно у нас темнее».

### Замеры (новые пробники)

- `.scratch/skirt-time.mjs` — юбка дыхания ВО ВРЕМЕНИ: по каждой гармонике в
  окнах 0-60…1200-1800 мс считает пик (P), «верхнюю юбку» (US = среднее по
  полосе 1.1-1.4·k·f0, ограниченной 0.93·(k+1)·f0), её ШИРИНУ (W = US−P) и пол
  между гармониками (MID) — всё в дБ отн. h1 сустейна, L-канал банка.
  Сводка поправок — `.scratch/skirt-fit.mjs`.
- `.scratch/atk-harms.mjs` — профиль гармоник h1..h16 в коротких окнах
  (0-30/30-60/60-120/120-250/250-500/800-1200 мс), наши/банк.
- `.scratch/fine-scan.mjs` — тонкий скан (2 Гц) с опцией `WASM_JS=...` для
  сравнения с сохранённым снимком.

### Pan Flute (GM 75): оказалось, дело не в гребёнке

Замер впервые разделил то, что раньше мерили одним числом (дБ отн. сустейна h1,
среднее по k=1..4; банк юбка/пол):

| окно, C4 | 0-60 | 60-140 | 260-450 | 450-800 | 1200-1800 |
|---|---|---|---|---|---|
| банк юбка / пол | −34/−30 | −36/−42 | −45/−53 | −48/−57 | −54/−64 |
| было (v55) юбка / пол | −34/−41 | −36/−37 | −45/−48 | −48/−54 | −54/−57 |

У банка пол между гармониками СТОИТ на юбке в первых 60 мс (−30 против −34) и
уходит под неё к сустейну на 10-20 дБ — то есть «ширина юбки спадает по ходу
атаки», как владелец и слышал. У нас этого не было: гребёнка задана одна на всю
ноту.

Что пробовалось и что осталось:

1. **Каскад гребёнки** (`combPasses = 2`, движковый параметр добавлен): пол
   между гармониками падает на 19 дБ (у банка между гармониками −80…−100 дБ,
   `.scratch/fine-scan.mjs`), но вместе с ним на 10 дБ падает ФЛАНГ юбки —
   уровня пришлось бы поднимать обратно, выигрыша нет. Откатил на один проход;
   механизм оставлен параметром.
2. **Перестраиваемая гребёнка** (g(t) от широкого к узкому) — тоже пробовал:
   обратная связь с гребёнкой не может опустить пол НИЖЕ собственной полки,
   то есть ширину во времени она не даёт. Откатил, в движке не осталось.
3. **Два слоя дыхания** — то, что осталось: узкий (гребёнка 0.85, как в v55) и
   широкий (0.35, почти без гребёнки) с быстрым спадом. Сумма даёт «широкий
   вход → узкий сустейн». Уровни слоёв и полка в сустейне подобраны замером за
   5 итераций (v56…v56e).
4. **Задержка гребёнки была ЦЕЛОЙ** (`round(sr/f0)` = 84 вместо 84.28 на C5):
   гребёнка уезжала с гармоник на 5-7 центов, и юбки h2..h4 садились рядом со
   своей гармоникой. В движке теперь дробная задержка (линейная интерполяция
   линии задержки) — юбки стоят ровно на гармониках.

Итог (Δ «наши − банк», среднее по k=1..4, юбка):

| ключ | 0-60 | 60-140 | 260-450 | 450-800 | 1200-1800 |
|---|---|---|---|---|---|
| C4 | +2.8 | −5.5 | +0.5 | −2.3 | 0.0 |
| C5 | +0.3 | +5.0 | −0.8 | −2.5 | +1.8 |
| C6 | −0.8 | +0.8 | −0.3 | 0.0 | +4.0 |

Плюс по просьбам: спад C6 замедлен (0.50 → 0.95 с: у банка юбка держится до
800 мс), полка сустейна выше (0.26 → 0.30, 0.42 → 0.25 после подгонки),
дыхание в сустейне стало слышно (в v55 фланг на 450-800 мс был на 9-13 дБ ниже
банка).

### Recorder (GM 74): «темнее» делала... наша же вибрато

Ключевая находка получена диагностической сборкой: с ВЫКЛЮЧЕННЫМ вибрато наши
гармоники совпадают с банком до 0.5 дБ (ключи 62/72, `.scratch/oc-rec-tuneL.mjs`),
а с включённым — на 8-16 дБ ниже (h4 −15.6, h5 −13.9, h6 −8.9). Причина:
FM-вибрато размазывает партиалы в боковые полосы, и пик гармоники в спектре
падает. У банка вибрато записано в самой петле семпла, поэтому партиалы
остаются острыми и яркость не страдает.

- глубина вибрато 15.9-13.2 цента → **5.2-4.3 цента** (в 2.5-3 раза меньше),
  частота растёт к верху (5.0 → 5.8 Гц на выходе) — по просьбе «на C6 быстрее».
  Остаточное отставание h5..h8 — 2.5-11 дБ (замер с вибрато on).
- **яркая атака** (блум-слой h2..h8, гейт 50 мс, τ 90 мс) — по замеру банка:
  в окне 30-60 мс у него h2 стоит на 2 дБ ВЫШЕ h1, а h4 всего на 14 дБ ниже; у
  нас было h2 на 11 дБ, h4 на 20 дБ ниже. Масштаб вспышки подобран замером
  (первая попытка дала всего +1 дБ — блум нормируется иначе, чем вейвтаблица).
- вход тона 75 → 105 мс линейно заменён профилем: плоский ноль (22 мс), быстрый
  подъём до 0.5 за 35 мс, экспоненциальное добирание за 90 мс — по замеру у
  банка первые 30 мс почти тишина (−41 дБ отн. сустейна), а к 100 мс перелёт.

Относительно своего h1 в окне 30-60 мс: у нас h2 −4, h3 −14, h4 −13 против
банковских +2/−16/−14 — яркость атаки вышла на уровень банка (было −11/−19/−20).

### Честно

- Клип-проба (`.scratch/click-lr.mjs`) на блокфлейте показывает 17/57 «клика»
  в первых 50 мс (ключи 62/72) — это яркая атака-блум; `max|2nd|/rms` при этом
  0.055/0.193, то есть НИЖЕ, чем у одобренной пан-флейты (0.07-0.19), и все
  отсчёты в окне 0.241-0.248 с (вспышка), а не периодические щелчки.
- Блокфлейта по-прежнему на 16-18 дБ громче банковского рендера в абсолюте
  (общее свойство семейства, отдельного прохода не делал).
- Пан-флейта: межгармонический пол у нас всё ещё на 10-20 дБ выше банковского
  (у банка между гармониками бывают провалы до −100 дБ, у нас ровнее) — это
  предел гребёнки на периоде ноты; следующий шаг — банк резонаторов по
  гармоникам, а не гребёнка.
- Слушать не могу: всё сделано по замеру, уши владельца — финальная проверка.

### Верификация

- `scripts/smoke-test-wasm.mjs` — PASSED (0 non-finite).
- Клики: 0 на 75 (60/72/74/79/84), 79 (62/80), 78 (74); на 74 — см. «Честно».
- `web/generated/` = `dist/` (md5 совпадают).
- Точка отката к одобренной владельцем версии:
  `.scratch/known-good/20260912-PanFlute-v55/` (снимок `InstrumentLibrary.cpp` +
  `IntraSynth.wasm` md5 `a035429793eccea56d9e9252210ac3e3`).
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 209 376 байт**,
  md5 `03114769d0118dc779d3388a4bf880eb`, IntraSynth.js 14 367 B.

### Файлы

- `intrasynth/src/Intra/Synth/Synth.h` — `combPasses` (каскад гребёнки) и
  дробная задержка гребёнки в `NoiseSampler`.
- `intrasynth/src/Intra/Synth/InstrumentLibrary.cpp` — пан-флейта (два слоя
  дыхания, опорные точки C4/C5/C6, спад/полка) и блокфлейта (вибрато, блум,
  профиль входа тона).
- Пробники: `skirt-time.mjs`, `skirt-fit.mjs`, `atk-harms.mjs`, `fine-scan.mjs`.


## Update 57 — Блокфлейта: яркая атака приходит в окно 30-60 мс (время, а не уровень)

Слушатель после Update 56: «Recorder C6 в оригинале после раскачки вибрирует
быстрее. И в C4 в начале звук более яркий в оригинале, чем у нас. У нас ровный
тон, а там он в него переходит не сразу. И всё равно у нас темнее».
Ocarina и Pan Flute не трогал (одобрены).

### Время яркой атаки

Замер `.scratch/atk-harms.mjs` по окнам (дБ отн. h1 В ТОМ ЖЕ окне) показал, что
дело не в уровне, а во ВРЕМЕНИ вспышки:

| ключ 62, окно | наша h2/h1 (v56e) | банк |
|---|---|---|
| 0-30 мс | +11 | −9 |
| 30-60 мс | −4 | **+2** |
| 60-120 мс | −9 | −7 |

То есть у банка яркость приходит ПОСЛЕ старта (в 30-60 мс его h2 стоит на 2 дБ
ВЫШЕ h1), а у нас вспышка вспыхивала за 12 мс — яркость попадала в самый первый
момент, где слух читает её как часть удара, а окно 30-60 мс оставалось тёмным.
Отсюда и «ровный тон», и «в оригинале ярче в начале».

Первая правка (v57: рост вспышки 0.048 с, τ 0.20 с при масштабе слоя 0.16)
попадание дала, но проба на щелчки `.scratch/click-lr.mjs` показала жёсткий
призвук: **292/295/10 «кликов»** и max|2nd|/rms **0.476** на C6 (у одобренной
пан-флейты 0.187-0.214, до правки было 17/57/0 и 0.323). Принят v57b:
рост 0.036 с, τ 0.16 с, масштаб слоя 0.16 → **0.11**.

Итог v57b (ключ 62): 0-30 мс h2/h1 +2 (было +11), 30-60 мс −5, 60-120 мс −8;
клики **37/93/0**, max|2nd|/rms **0.053/0.184/0.311** — то есть не хуже, чем до
правки, при сдвинутом на 30 мс пике.

Относительно СВОЕГО сустейна наша атака теперь ярче: h2 +15 дБ, h4 +22 дБ,
h8 +33 дБ (в окне 30-60 мс против 800-1200 мс) — «переход в тон не сразу»
измеряется, а не только слышится.

### Что осталось честно открытым

- Наш 0-30 мс всё ещё на ~10 дБ ярче банковского относительно h1: у банка первые
  30 мс — почти один h1 (его h2 там −50 дБ, h3/h4 −55), у нас h2 −26. Убрать это
  полностью нечем: у `BloomSampler` нет задержки вспышки (есть только рост), а
  убирать рост в ноль нельзя — тогда пик уедет в начало ноты.
- Сустейн h5..h8 у нас по-прежнему на 7-9 дБ темнее банка (ключ 62). Диагностика
  Update 56 показала: с ВЫКЛЮЧЕННЫМ вибрато гармоники совпадают до 0.5 дБ, с
  включённым — FM-вибрато размазывает партиалы (J0(β), β = k·Δf/f0·f0/f_m: для
  h5 ≈ 1.9 дБ, h8 ≈ 4.9 дБ на ключе 62). Остаток (3-4 дБ) — разрешение пробы:
  вибрато смещает гармонику на ±6.8 Гц при бине 2.5 Гц. Принципиальное лечение —
  не FM поверх рендера, а вибрато, «запечённое» в вейвтаблицу (как в петле
  семпла банка); это движковая правка, сделаю по отдельному согласованию.
- Абсолютный уровень: блокфлейта на +17…+21 дБ громче сухого рендера банка
  (h1 −19.5/−18.7 dBFS против −36.5/−39.5 на ключах 62/84). Это общее свойство
  семейства (Pan Flute +3.4, Ocarina +11.4, Whistle +13.9). Владелец жаловался
  на громкость только у окарины, поэтому здесь не трогал.

### Верификация

- `scripts/smoke-test-wasm.mjs` — PASSED (0 non-finite).
- Клики: 75 (60/72/84) — 0; 74 (62/72/84) — 37/93/0 (см. выше); 79 (62/80) — 0.
- Пан-флейта (Update 56) не изменилась: `.scratch/skirt-fit.mjs` на свежем
  замере `.scratch/skirt-time-v57.txt` даёт те же ±5 дБ (ключи 60/72/84).
- `web/generated/` = `dist/` (md5 совпадают).
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 209 376 байт**,
  md5 `9b9d9ef1d11dc170d0f234930aced0dc`, IntraSynth.js 14 367 B.

### Файлы

- `intrasynth/src/Intra/Synth/InstrumentLibrary.cpp` — только блок Recorder
  (вспышка блума: рост/спад/масштаб).
- Пробники: `tune-check.mjs` (точная высота тона: наши ±0.4 цента от номинала —
  «f0 510 Гц» в `rec-vib.mjs` на ключе 72 оказался артефактом поиска пика,
  вибрато-трасса считает центры от него и показывает +44 цента).


## Update 58 — Блокфлейта: возврат от блума к данным банка (затухание 6.2 дБ/с и поздний вход); Pan Flute: пик атаки и лёгкое вибрато сустейна

Владелец: «Recorder вообще сломался, не похож абсолютно стал»; «У Pan Flute атака
как будто какой-то слишком простой формы… В сустейне не хватает лёгкого вибрато
оригинала»; вопрос про запекание вибрато в вейвтаблицу.

### Блокфлейта: что именно сломалось
В Update 56 я добавил отдельный блум-слой (h2..h8) со своей огибающей и паузу
22 мс перед тоном. Замер атаки (`.scratch/atk-harms.mjs`) показал, что блум
сломал тембр атаки: в окне 0-30 мс у нас h2 стояла на **4 дБ ниже h1** (жирный
«бум»), а у банка в том же окне h2 на **34 дБ ниже h1** — у банка атака почти
чистый тон с подъёмом h4..h6. Откат: блум-слой убран целиком, вход тона — снова
линейный (v55), глубина вибрато возвращена.

Важный урок про замер: «FM-вибрато съедает партиалы» (Update 56, основание для
среза глубины) было **артефактом зонда** — узкий зонд мерил только несущую линию
гармоники, а банк меряется тем же зондом и держит свои партиалы. Замер
`.scratch/rec-vib.mjs` (клавиши 62..96): банк **4.96..4.99 Гц и ±14.2..14.9
цента ровно на всех клавишах** — предположение «на C6 вибрато быстрее» не
подтвердилось, частота и глубина теперь постоянные.

### Блокфлейта: главная находка — затухание
Новый профиль огибающей (`.scratch/bank-env.mjs`, RMS по окнам 0..6 с, дБ отн.
уровня в окне 2.0-2.5 с) показал: банк на **всех** клавишах 62/72/84 даёт подъём
за ~50-100 мс и далее РАВНОМЕРНОЕ затухание **3.1 дБ за 0.5 с (6.2 дБ/с)** вплоть
до 6 с (−16.4 дБ), а у нас была ровная полка 0.0 дБ от 75 мс до конца. Это
объясняет и «в C4 в начале звук более яркий, там он в него переходит не сразу»,
и «всё равно у нас темнее»: у банка вся нота — атака + спад.

- `ExpCoeff` 0.16 → **0.71** (неперы/с: ExponentAttenuator считает
  `exp(−coeff/sampleRate)`, значит 6.167 дБ/с = 0.71).
- Вход — регистровый профиль по сетке 10 мс (`.scratch/bank-env-fine.mjs`):
  у банка на C4 первые 10 мс −41 дБ отн. уровня 2 с (почти тишина), подъём
  начинается после ~30 мс; на C6 вход сразу (−18.4 дБ в первом окне).

### Блокфлейта: результат (замер bank-env / bank-env-fine, банк Titanic, L-канал)
| клавиша | 0-10 мс | 20-30 мс | 40-50 мс | 60-120 мс | 200 мс…6 с |
|---|---|---|---|---|---|
| 62 (C4) | +2.6 | −6.8 | −1.6 | +2.0…+4.7 | ±0.3 |
| 72 (C5) | −1.8 | −3.5 | 0.0 | −0.6…−0.4 | ±0.2 |
| 84 (C6) | +0.1 | +3.9 | +0.5 | −0.1 | ±0.2 |

Атака по гармоникам (`atk-harms`, окна 0-30…800-1200 мс): Δ ≤ **5.2 дБ** на всех
клавишах (было +18 дБ в окне 0-30 мс). Сустейн-гармоники (`rec-vib`): ±8 дБ
на клавишах 82/86/90, но ключ 84 — **−6…−11 дБ** при ключах 82/86 в ±4: это
собственный разброс семпла банка внутри зоны A#5 (одна таблица зоны его не
различит), уровень зоны не трогал. Вибрато: 4.80..4.90 Гц и ±14.8…16.3 цента
против банковских 4.97 и ±14.6…14.9.

### Pan Flute
- Атака: у банка есть ПИК, у нас его не было. Замер `bank-env`: key 60 банк
  +3.9 дБ в 200-300 мс, key 72 +4.2 в 150-200 мс, key 84 **+8.7 в 50-75 мс**;
  у нас на C6 в тех же окнах было +0.6/+1.9. Слой W («широкий вход»), слой
  дыхания и полку не трогал; пересобраны опоры овершута h1
  (`overA1`/`overRise`/`overTau`) и вход зон D5/G5 в огибающей.
  Итог C6: 0-25 мс +2.9, 25-50 −1.4, 50-75 −3.0, 75-100 −2.7, 100-200 ±0.8,
  200-500 −1.4…−3.8 (было −7.4…−8.1 в 25-100 мс).
- Вибрато сустейна добавлено: **5.6 Гц, ±12.6 (C4) / ±6.3 (C5) / ±3.5 (C6)
  цента**, задержка 0.35 с, вход 0.30 с (банк: ±12.6/4.2/2.2 цента при 5.5-8.5 Гц).
  Глубина ограничена: блум-слой h1 стоит на f0 и не вибрирует, поэтому глубокое
  вибрато дало бы амплитудную модуляцию от фазового разъезда с телом (ровно та
  жалоба была в Update 40). Разброс RMS сустейна (100 мс): 0.51/0.37/0.57 дБ
  против банковских 0.25/0.20/0.83 — модуляции сверх банка нет.

### Честно
- C5 пан-флейты на +4…+7 дБ громче банка в первых 75 мс (это слой W — «широкая
  юбка в атаке», о которой просил владелец), и её овершут в 200-700 мс ещё на
  2-3 дБ ниже банка. C4 пан-флейты в ±3.2 дБ по всем окнам.
- Ширина юбки по ходу атаки спадает не «задержкой гребёнки», а суммой двух
  слоёв дыхания (W — почти без гребёнки и с самым коротким спадом; N — с
  гребёнкой 0.85 в сустейне). Механизм `combPasses` в движке сохранён, но
  каскад не дал выигрыша (см. Update 56d).
- Слушать не могу: правки только по замеру, уши владельца — финальная проверка.

### Верификация
- `scripts/smoke-test-wasm.mjs` — **PASSED** (0 non-finite);
- клики: **0** на 74 (62/72/84), 75 (60/72/84), 79 (62/80);
- `web/generated/` = `dist/` (md5 совпадают). Снимки для отката:
  `.scratch/known-good/20260912-PanFlute-v56f/` и
  `.scratch/known-good/20260912-Recorder-v56broken/`.

**WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 209 102 байта**,
md5 `9114273de09b70535f0052c66aa45f0c`, IntraSynth.js 14 367 B.

## Update 59 — Вибрато пан-флейты идёт по ВСЕМ слоям ноты (общий LFO в движке)

Владелец: «Что-то я вообще не слышу вибрато Pan Flute. И вообще что-то не то с
тембром, какое-то гудение на C4, как будто насос работает, но с юбкой»; вопросы
про процедурный SF2-подобный кэш семплов, степень двойки у IFFT и «острые
партиалы» («это не решается суммой звуков с вибрато и без?»).

### Диагноз: вибрато было у ОДНОГО слоя
Замер (новый пробник `.scratch/vib-track.mjs`: полосовой фильтр вокруг h1,
мгновенная частота по нулям в окнах 12 мс + АМ ТОЙ ЖЕ полосы; и
`.scratch/pump-check.mjs`: AMDF по всему сигналу, АМ огибающей и острота линий):

| ключ | наш вибрато (rms/пик) | банк | АМ полосы h1 (rms/размах) | банк |
|---|---|---|---|---|
| 60 (C4) | 3.5 / 9.4 ц, 5.6 Гц | 2.9 / 8.2 ц | 0.27 / 1.85 дБ | 0.40 / 2.60 дБ |
| 72 (C5) | 3.2 / 9.8 ц, 5.4 Гц | 2.2 / 8.3 ц | 0.20 / 1.18 дБ | 0.38 / 2.11 дБ |
| 84 (C6) | 2.3 / 5.8 ц, 5.4 Гц | 2.1 / 4.6 ц | 0.31 / 2.25 дБ | 0.66 / 3.58 дБ |

Глубина вибрато у нас НЕ меньше банка — значит «не слышу» было не про глубину.
Причина архитектурная: вибрато было только у вейвтейбла (`VibratoProfile`), а
слои дыхания (NoiseSampler) и блума/овершута (BloomSampler) играли РОВНУЮ ноту:

* гребёнка дыхания «вшита» в таблицу (задержка = период ноты), и таблица читалась
  ровно 1 сэмпл на выходной — линии юбок стояли ЖЁСТКО на номинальных кратных;
* тело же уезжало на ±0.23 % (±9 ц на C4) → жёсткие линии бились с уходящим
  тоном (до 1.4 Гц на h1 и до 6 Гц на h5 — это и есть «насос» в тембре), а сам
  тон «стоял на месте» по высоте, потому что жёсткие линии держали центр.

### Что сделано (Update 59)
- **Общий LFO ноты** `VibratoLfo` (WaveTableSampler.h): та же математика, что у
  `WaveTableSampler` (SineRange: амплитуда = глубина, фаза 0, шаг 2πf/sr, gate
  Delay/Ramp). Слой, стартующий вместе с телом и получающий по одному `Next()`
  на выходной сэмпл, остаётся с ним В ФАЗЕ (все слои одной ноты рендерятся одной
  и той же длиной блока в `fillStereo`/ADSR-пути).
- **NoiseSampler**: позиция чтения стала дробной, скорость = 1 + LFO → гребёнка
  юбок едет по частоте вместе с гармониками. Без вибрато скорость ровно 1,
  дробная часть 0 — старые инструменты рендерятся побитово как раньше.
- **BloomSampler**: коэффициенты рекурсии k = 2cos(step) модулируются тем же LFO
  (k(δ) ≈ k − 2sin(step)·step·δ, ошибка первого порядка ≤ сотых цента при
  δ ≤ 0.005). Без вибрато — прежняя быстрая ветка.
- **Pan Flute**: параметры вынесены в `PanFluteVibrato(freq)`; тот же профиль
  получают оба слоя дыхания (N и W), обе вспышки (h3/h4/h5) и овершут h1.
  Глубина/частота/задержка не менялись (5.6 Гц, ±4.0…±2.0 цента, 0.35 с + вход
  0.30 с) — поменялась только согласованность.

### Результат замера
|  | было (v58) | стало (v59) |
|---|---|---|
| вибрато всего сигнала, C4 | rms 2.6 ц, пик 5.9 | rms 2.9 ц, пик 6.1 |
| АМ полосы h1, C4 | rms 0.27 дБ, размах 1.85 | rms 0.25 дБ, размах 1.54 |
| АМ огибающей, C4 | rms 0.37 дБ | rms 0.36 дБ |

Теперь «дышит» вся нота: юбки и блум двигаются вместе с телом, относительного
биения жёстких линий с тоном больше нет.

### Верификация
- `scripts/smoke-test-wasm.mjs` — **PASSED** (0 non-finite).
- `.scratch/click-lr.mjs` — **0 кликов** на 75:60/72/74/79/84.
- `web/generated/` = `dist/` (md5 совпадают).
- Точка отката этой версии: `.scratch/known-good/20260912-PanFlute-v59/`.
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 211 309 байт**,
  md5 `d0133568833d866d1ae8e77134eac27c`, IntraSynth.js 14 367 B (+2.2 КБ к v58
  на код общего LFO и модуляцию двух слоёв).

### Честно открыто
- Слушать не могу: правки только по замеру. Если «гудение на C4» останется, то
  единственное отличие C4 от одобренной владельцем v55 (кроме вибрато) — слой W
  (Update 56, уровень 0.20 на C4, живёт только в атаке) и полка дыхания в
  сустейне 0.26 → 0.30; их и надо будет трогать следующими.
- Процедурный «SF2-семпл» (атака + петля, генерируется при первом использовании)
  разобран в отдельной записи решения: `docs/decisions/active/20260912-ProceduralSampleCache.md`.

## Update 60 — Блокфлейта: тело осветлено и старт по зонам, шум вниз; пан-флейта: тело ярче по замеру; 43/115: один профиль вибрато на все слои

Владелец: «Recorder какой-то шумный, куда-то вообще не туда ушёл. Pan Flute
дыхание звучит идеально, но основной тембр какой-то тёмный, тихий и
невразумительный, его бы усилить как-то и добавить яркости, но в соответствии с
замерами банка. Кстати, Flute 115 и 43 вроде похожи, но при этом звучат с
какой-то неприятной вибрирующей расстройкой. Проверь глубину вибрато или может
ещё какие-то причины расстройки есть не в нём?»

### Диагностика: замер ТЕЛА отдельно от дыхания

Прибор один и тот же у обеих жалоб: `atk-harms.mjs` меряет ПИК гармоники, а у
наших слоёв дыхания гребёнка стоит ровно на k·f0, поэтому её линии попадают в
«гармоники» зонда. Собраны две диагностические сборки (`patch-diag-pfbody.cjs`,
`patch-diag-recbody.cjs`: уровни шумовых слоёв = 0), чтобы измерить тело
(вейвтейбл + блум) без дыхания:

- `.scratch/pf-body-only-v59.txt` — пан-флейта (75);
- `.scratch/rec-body-only-7keys.txt` — блокфлейта (74), все 7 зон.

### Пан-флейта (75): тело было на 4-12 дБ темнее банка

Окно 800-1200 мс, дБ отн. h1, Δ = банк − наш:

| ключ | h2 | h3 | h4 | h5 | h6 | h7 | h8 | h9 | h10 | h11 | h12 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 60 (A3) | 0 | +1 | 0 | +2 | +5 | +4 | +5 | +7 | +9 | +10 | +12 |
| 72 (D4) | 0 | +1 | +4 | +5 | +7 | +9 | +9 | +12 | +10 | +10 | +7 |
| 84 (G4) | +2 | +4 | +4 | +6 | +7 | +8 | +5 | +3 | −2 | +3 | +2 |

Зоны A3/D4/G4lo/G4hi пересчитаны на эти Δ (значения ×10^(Δ/20), обрезка по
32768). Уровни дыхания, вибрато и огибающая НЕ трогались (владелец: «дыхание
звучит идеально»).

Честно: в СОВМЕСТНОМ замере (тело + дыхание) этой правки почти не видно — в
бинах h6..h12 энергия определяется гребёнкой дыхания (например C4 h8: тело
−63 дБ, а вместе −38), поэтому тело там на 25 дБ тише слоя, который уже слышен.
Видно только на нижних гармониках, где тело доминирует (C4: h5 −21 → −19 дБ,
h7 −32 → −30 — ровно применённые +2/+4). Верхние Δ применились к телу точно
(таблицы), но слышимость правки — за ухом владельца.

### Блокфлейта (74): тело тоже тёмное, а межгармонический пол — на +4..+8 дБ выше банка

Тело (окно 800-1200 мс, Δ = банк − наш по 7 зонам, h2..h12):

| зона (клавиша) | h2 | h3 | h4 | h5 | h6 | h7 | h8 | h9 | h10 | h11 | h12 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| D3 (62) | −1 | +2 | +6 | +11 | +11 | +11 | +13 | +19 | +21 | +20 | +22 |
| A4 (67) | +4 | +9 | +11 | +13 | +15 | +16 | +15 | +14 | +16 | +15 | +16 |
| B4 (70) | +3 | +10 | +17 | +10 | +7 | +8 | +13 | +26 | +26 | +23 | +23 |
| C#5 (72) | +4 | +16 | +15 | +7 | +6 | +10 | +26 | +15 | +6 | +11 | +4 |
| D5 (75) | +6 | +27 | +8 | +5 | +8 | +17 | +14 | +8 | +3 | +3 | −1 |
| A5 (79) | +13 | +7 | +5 | +11 | +12 | +6 | +11 | +16 | +18 | +11 | +15 |
| A#5 (84) | +4 | +5 | +11 | +10 | +11 | +17 | +15 | +22 | +15 | +10 | +4 |

То есть таблицы (снятые когда-то с ПЕТЕЛЬ семплов) темнее РЕНДЕРА банка на
5-26 дБ — вот откуда «тёмный», и на этом фоне шумовой пол доминировал: пол в
полосе 400+ Гц (EXCL, `.scratch/rec-spec-bands-v59.txt`) был на +7.8/+6.7/+4.7/
+6.3/+4.3 дБ (клавиши 62/67/72/79/84) выше банка, максимум +16…+17.

Сделано (все 7 таблиц пересчитаны на Δ; измеренные h13..h16 — по Δ h12):

- дыхание 0.018 → **0.0095**, кривая по регистру переведена в явные три узла
  {0.60, 1.05, 1.00} (прежние {0.65,1.25,1.35,**1.60**} вызывались с n=3, то
  есть 4-й узел был мёртвым — убран), гребёнка **0.55 → 0.68** (энергия липнет к
  гармоникам, пол уходит ниже);
- **старт — по зонам семплов** (как таблица), `zV0/zT0`: замер банка h1 в окне
  0-30 мс у D3 −41, A4 −16, B4 −19, C#5 −28, D5 −3, A5 −5, A#5 −2 дБ отн.
  сустейна. Прежний плавный профиль по u запаздывал на A4/B4/D5/A5 на 6-16 дБ.

Итог: сустейн-гармоники (тело + дыхание) — в ±2 дБ от банка на 62/67/72 (84:
выше h8 на 4-9 дБ темнее), окна атаки — в ±6 дБ (были до 16), пол 400+ Гц —
средний Δабс **+2.5 / 0.0 / −3.2 дБ** (было +7.8 / +4.7 / +4.3), максимум
+11/+10/+8 (было +16/+14/+15). Абсолютный h1 остался +6.2…+6.3 дБ над банком —
это общий уровень семейства (открытый пункт, отдельного прохода не делал).

### Flute 115 vs 43: глубина вибрато расходилась в 1.6×

Замер `.scratch/vib-track.mjs` (полоса h1, окна 12 мс): вибрато 43 rms
**5.0/7.1/11.4** цента (C4/C5/C6), 115 — **7.8/11.1/18.4** (1.6×), банк (73) —
7.2/6.6/10.1. Причина: срез глубины Update 42 (0.6×) применили к FluteClean, а
FluteHybrid остался на старых значениях (копипаст разошёлся — ровно случай
правила «unify similar code paths»).

Сделано: общий `FluteTitanicVibrato(freq)` (профиль FluteClean) — у ОБЕИХ флейт
в вейвтейбле и, как у пан-флейты (Update 59), в слоях дыхания и блума; у
блокфлейты профиль вынесен в `RecorderVibrato(freq)` и тоже раздан дыханию.
После правки 43 и 115 дают ОДИНАКОВОЕ вибрато rms 5.0/7.1/11.4 (115: было
11.1 → стало 7.1 на C5), синхронное по всем слоям — биения жёстких линий с
уходящим тоном больше нет по построению.

### Верификация

- `scripts/smoke-test-wasm.mjs` — **PASSED** (0 non-finite).
- `.scratch/click-lr.mjs` — **0 кликов** на 74 (62/67/75/84), 75 (60/72/84),
  43 (60/84), 115 (60/84).
- `web/generated/` = `dist/` (md5 совпадают).
- Точка отката: `.scratch/known-good/20260912-FluteFamily-v60/`.
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 211 310 байт**,
  md5 `5a4e1582e166f98fcb6fa4b1d63d5322`, IntraSynth.js 14 367 B (+1 байт к v59
  на хелперы вибрато и зонную таблицу старта).

### Честно открыто

- Слушать не могу: все правки — по замеру; уши владельца финальная проверка.
- Пан-флейта: правка тела почти не видна в совместном пробнике (дыхание
  доминирует в тех же бинах). Если «тёмный и тихий» останется, следующий шаг —
  поднять саму полку дыхания в сустейне (владелец просил его не трогать) или
  уровень тела (сейчас h1 ровно на банке).
- Блокфлейта: ключ 67 (A4) всё ещё −5.7 дБ в окнах 0-30/30-60 мс; абсолютный
  уровень +6 дБ над банком (семейство).
- Блокфлейта выше h8 на C6 на 4-9 дБ темнее банка (h13-h16 взяты по Δ h12).

### Файлы

- `intrasynth/src/Intra/Synth/InstrumentLibrary.cpp` — тела PanFlute (4 зоны) и
  Recorder (7 зон), дыхание 74, огибающая 74 по зонам, хелперы
  `FluteTitanicVibrato`/`RecorderVibrato` и общий LFO у 43/115/74.
- Патчи: `.scratch/patch-update60.cjs`, `patch-update60b-recenv.cjs`,
  `patch-update60c-recenv2.cjs`; диагностика `patch-diag-pfbody.cjs`,
  `patch-diag-recbody.cjs`.

## Update 61 — вибрато стало слышимым; яркость блокфлейты и флейт 43/115 поднята к банку

Владелец: «Всё равно наша Pan Flute глухая по сравнению с оригиналом, а вибрато
не слышно. У Recorder шум пропал, но вибрато как будто по частоте в 2 раза
уступает оригиналу. И тоже глухой. И то же самое с вибрато флейты 43/115, оно
слишком редкое, незаметное. А расстройка всё равно чувствуется. Кстати,
расстройка чувствуется не на примерах C4, C5, C6, а на промежуточных нотах,
например, A3. Может какая-то кривая интерполяция?»

### Новый прибор: `.scratch/hnr-audit.mjs`

Старые зонды брали ПИК гармоники в узкой полосе, а у наших слоёв дыхания
гребёнка стоит ровно на k·f0 — её линии попадали в «гармоники», и по такому
замеру нельзя было отличить «тембр темнее» от «шума больше». Новый пробник
даёт по одной клавише сразу: уровни гармоник h1..hK в полосах ±40 Гц,
межгармонический пол 200 Гц-9 кГц, **HNR** (гарм./шум), **центроид** по
номеру гармоники (мера яркости) и глубина/частота FM полосы h1 (окно 0.8-4.2 с,
чтобы хватало разрешения по частоте модуляции). Полосы считаются ОДИНАКОВО
для нас и для банка, поэтому сравнение честное.

### Вибрато: глубина (замер `hnr-audit`, rms в центах, полоса h1)

| инструмент | было (v60) | банк | стало (v61) |
|---|---|---|---|
| Pan Flute 75:60 | 3.5 | 2.9 | **7.8** (когерентно 5.87 Гц, mag 7.9) |
| Pan Flute 75:72 | 3.2 | 2.2 | **6.3** |
| Recorder 74:60 | 7.7 | 8.5 | **11.2** (4.89 Гц) |
| Recorder 74:72 | 8.8 | 8.3 | **12.3** |
| Flute 43 43:60 | 5.0 | 23.1 | **14.1** (3.92 Гц) |
| Flute 43 43:72 | 7.1 | 21.6 | **16.1** (6.36 Гц) |
| Flute 43 43:57 | 7.0 | 24.2 | **14.1** (3.92 Гц) |

Честно о замере: у пан-флейты когерентного вибрато у банка НЕТ вовсе
(rms 2.2-3.1 при mag 0.5-1.1 — это шероховатость), а у блокфлейты наша
глубина и частота (4.89 Гц) уже совпадали с банком. То есть «в 2 раза
уступает» замером не подтверждается ни по глубине, ни по частоте. Правки
сделаны по слуху владельца, а не по прибору, и это записано здесь честно:
глубина поднята к банковской (флейты — 0.75 от банковского замера, чтобы
«неприятная вибрирующая расстройка» не вернулась), у пан-флейты и блокфлейты
— выше банковского замера (иначе, как показали v56-v60, вибрато на слух не
читается вообще). Частота пан-флейты 5.6 → 6.0 Гц, флейт 4.1 → 4.3…6.0 Гц
(растёт с регистром, как у банка 4.4 → 7.3), блокфлейты 4.78 Гц оставлена.

### Откат: задержка пан-флейты возвращена к v60 (0.35/0.30 с)

Пробный ранний вход (0.22/0.22 с) ронял уровень атаки: замер
`.scratch/click-zoom.mjs` (средний rms по 1-мс окнам) на 75:72 давал
0.22-0.32 с −31.2 дБ против −28.5 у v60, 0.32-0.46 с −27.7 против −24.7, а
полосовой зонд `.scratch/atk-bands.mjs` — −6.6 дБ от банка в окне 120-250 мс
(было −1.9). Задержка возвращена; вибрато пан-флейты теперь начинается на
0.35 с, то есть в СУСТЕЙНЕ (владелец просил именно сустейн).

### Яркость: только там, где замер её подтверждает

| инструмент | центроид было → стало (банк) | полосовая сумма сустейна, Δ к банку |
|---|---|---|
| Recorder 74:60 | 3.70 → **4.53** (4.41) | −0.1 → **+0.2 дБ** |
| Recorder 74:72 | 2.83 → **3.16** (3.16) | — |
| Flute 43 43:60 | 6.57 → 6.52 (7.43) | −4.4 → **−3.2 дБ** |
| Flute 43 43:72 | 3.16 → **3.63** (3.40) | — |
| Pan Flute 75:60 | 4.61 → 4.51 (4.89) | — |

Сделано: h4..h12 тела ×1.413 (+3 дБ) у блокфлейты (7 зон) и флейт 43/115
(8 зон FluteClean, общая таблица). HNR блокфлейты при этом сдвинулся К банку
(74:60 2.3 → 0.7 при банковских 1.0; 74:72 0.2 → −0.9 при −1.4).

**Пан-флейту НЕ трогал** — и это результат замера, а не пропуск: её полосы
±40 Гц на 60-80 % заняты гребёнкой дыхания (тело доминирует только на
h3/h5/h7), поэтому ×1.334 по h4..h12 сдвинул центроид всего на 0.02
(4.61 → 4.59 при банковском 4.89) — правка не читается, — а уровень тела
+1 дБ («усилить основной тембр») поднял сустейн на 0.8 дБ и РОНЯЛ атаку на
2.7 дБ: тело в атаке складывается с блум-овершутом в противофазе. Оба
пробных изменения откачены (`.scratch/patch-update61c`, `61d`).

### Расстройка на промежуточных нотах: замером не воспроизводится

- Наш h1 в строе на ВСЕХ клавишах: средний сдвиг −0.3…+0.2 цента
  (55/57/58/59/60/62/63/64/65/67/69/71/72 у 75/74/43).
- Банк, наоборот, расстроен по-своему: 43 — +10.3…+11.4 цента на всех
  клавишах, 74 — −4.4…−4.7 (ключи ≤65) и +14.8 (67/69), 75 — +2.7…+2.8.
- Биений в нашем рендере нет: АМ полосы h1 (0.8-4.2 с) rms 0.04-0.09 дБ у
  43 и 75 (у банка 4.5-5.9 дБ на 43 — это его собственная расстройка).
- Интерполяцию проверил отдельно: `MixZoneSets` переписывает FreqMultiplier
  как `i+1`, но все профили семьи построены через `Harms16`/явные
  HarmonicDesc с целыми кратностями, поэтому подмена безвредна; амплитуда и
  ширина интерполируются линейно. У блокфлейты и пан-флейты выбор зоны вообще
  жёсткий (без интерполяции), интерполяция есть только у 43/115 и у G4-зоны
  пан-флейты. Баг не найден.

### Верификация

- `scripts/smoke-test-wasm.mjs` — **PASSED** (0 non-finite).
- `.scratch/click-lr.mjs` — 0 кликов на 75 (60/72/84), 43 (60/72), 115 (60/72)
  и 74 (60/62/67/84). На 74:72 порог срабатывает на вспышке «чиффа»
  (337/468 отсчётов в 0.26-0.27 с) — проверено `.scratch/click-zoom.mjs`:
  профиль второй разности гладкий и вырос ровно ×1.4 (тот же +3 дБ по h4..h12),
  изолированных выбросов нет; узкополосный порог просто относительный.
- Атака не просела: средний rms 0.22-0.32/0.32-0.46 с — 74:72 −25.4/−24.4 →
  −26.0/−25.0, 43:60 −32.2/−26.5 → −32.6/−26.9, пан-флейта 75:72 совпала с v60.
- `web/generated/` = `dist/` (md5 совпадают).
- Точка отката: `.scratch/known-good/20260913-WindVibrato-v60/`, снимок v60 —
  `.scratch/IL-v60.cpp`; эталоны сборок для сравнения — `.scratch/ref-v60/`,
  `.scratch/ref-v61/`.
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 211 565 байт**,
  md5 `760f278f64dfce4471a60a8b466a1235`, IntraSynth.js 14 367 B (+255 байт к
  v60 — только константы профилей и вибрато).

### Честно открыто

- Слушать не могу: глубины вибрато выставлены по банковскому замеру и словам
  владельца; если «вибрирующая расстройка» вернётся, следующий шаг — 0.6×
  от текущих значений у 43/115.
- Пан-флейта: замер совместного спектра сходится с банком в ±2 дБ, поэтому
  «глухость» остаётся необъяснённой прибором. Единственное измеримое отличие —
  ширина линий: у банка вокруг каждой гармоники горб ±12-15 % её частоты
  (шумовая юбка), у нас энергия сидит тонкими линиями на тех же бинах
  (гребёнка дыхания). Следующий шаг, если жалоба останется, — заменить
  гребёнку на широкую юбку, но это меняет характер дыхания, которое владелец
  назвал идеальным, поэтому без его согласия не делаю.
- Пан-флейта и блокфлейта остаются на +6 дБ выше банка по абсолютному уровню
  (общий уровень семейства, отдельного прохода не делал) — поднять тело
  «по просьбе усилить» не удалось из-за противофазы с блумом в атаке.
- Флейта 43 у банка на +11 центов выше A440 на всех клавишах; наш тон ровно в
  A440. Если владелец сравнивает с банком на слух, это и есть «расстройка» —
  но она в банке, а не у нас.

### Файлы

- `intrasynth/src/Intra/Synth/InstrumentLibrary.cpp` — три профиля вибрато
  (`PanFluteVibrato`, `FluteTitanicVibrato`, `RecorderVibrato`), 8 массивов
  `fluteClean*` и 7 профилей блокфлейты (h4..h12 ×1.413).
- Патчи: `.scratch/patch-update61.cjs`, `61b-pfsplash`, `61c-pfrevert`,
  `61d-pfvol`; зонды `.scratch/hnr-audit.mjs`, `atk-bands.mjs`,
  `click-zoom.mjs`, `click-lr-any.mjs`.

## Update 62 — вибрато блокфлейты приходит ПОСЛЕ раскачки (точка отката v61 сохранена)

Владелец: «У Recorder теперь вибрато сустейна совпадает, но оно такое должно
быть не сразу, а после раскачки. В остальном он отличный, надо сохранить как
удачную версию. Флейты потом послушаю подробнее».

Итог: блокфлейта объявлена удачной версией и снята в точку отката ДО правки
(глубину и частоту вибрато Update 61 не трогали — тронута только задержка
появления). Флейты 43/115 в этом апдейте не трогались: владелец слушает их
отдельно.

### Новый прибор: `.scratch/vib-onset.mjs`

Прежние зонды мерили вибрато одним окном 0.8-4.2 с и по определению не могли
показать, КОГДА оно появляется. Новый пробник строит траекторию f0 по полосе h1
(окно 12 мс, шаг 2 мс) от note-on и считает rms траектории в центах блоками со
скользящим шагом — у нас и у банка одинаково. Блоки короче периода вибрато
(0.21 с при 4.78 Гц) дают «биение» rms, поэтому замер идёт блоками 0.26 с.

### Замер: раскачка у банка — 0.35-0.4 с, полная глубина к 0.6-0.7 с

rms FM полосы h1 по блокам 0.26 с (шаг 0.13 с), секунды от note-on:

| | 0.13-0.39 | 0.26-0.52 | 0.39-0.65 | 0.52-0.78 | полка |
|---|---|---|---|---|---|
| банк 74:62 | 2.6 | 4.0 | 6.4 | 8.8 | 6.4-10.7 |
| наш v61 | 9.6 | 11.7 | 11.0 | 10.6 | 10.6-11.8 |
| наш v62 | **2.8** | **4.8** | **8.2** | **10.3** | 10.3-11.9 |
| банк 74:72 | 2.2 | 2.5 | 6.0 | 8.0 | 7.6-8.4 |
| наш v61 | 10.5 | 11.9 | 11.4 | 11.9 | 11.4-12.6 |
| наш v62 | **3.0** | **4.8** | **8.7** | **11.6** | 11.4-12.5 |
| банк 74:84 | 1.4 | 1.7 | 6.1 | 8.4 | 7.5-8.4 |
| наш v61 | 11.4 | 12.4 | 12.4 | 13.1 | 12.1-13.4 |
| наш v62 | **3.2** | **4.9** | **10.0** | **13.0** | 12.2-13.1 |

Пол измерения у этого зонда 1.4-3 цента rms, то есть у банка до 0.35-0.4 с
вибрато просто НЕТ — оно начинается после раскачки и растёт до полного за
0.25-0.3 с. У v61 из-за Delay 0.15 + Ramp 0.12 оно было полным уже к 0.27 с
(отсюда «не сразу, а после раскачки»).

### Правка

`RecorderVibrato` (`InstrumentLibrary.cpp`): `Delay 0.15 → 0.30`, `Ramp 0.12 →
0.30` — полная глубина к 0.60 с, как у банка на всех трёх проверенных клавишах.
Глубина (`Value`) и частота (4.78 Гц, Update 61) не тронуты: владелец принял их
как совпадающие. Профиль по-прежнему ОДИН на тело и дыхание (Update 60),
поэтому юбки уезжают вместе с тоном и рассогласования нет.

### Верификация

- `vib-onset.mjs` — таблица выше: профиль появления вибрато совпал с банком
  (в пределах пола измерения) на 62/72/84.
- `smoke-test-wasm.mjs` — **PASSED** (0 non-finite).
- `click-lr-any.mjs` (74:60/62/72): 0 кликов на 60 и 62; на 72 остаётся прежний
  «чифф» (337/468 → **317/369**, то есть только меньше), он был и в v61 и
  разобран в Update 61 по `click-zoom`. Шум возврата IIR на R упал
  (60: 0.060 → 0.031, 62: 0.098 → 0.050) — модуляция теперь стартует позже.
- `click-zoom.mjs` 74:62 (1-мс окна 0.22-0.42 с) — огибающая атаки совпала с
  v61 (±0.3 дБ), новых выбросов нет.
- `atk-bands.mjs` — сустейн-гармоники те же, что в v61 (74:62 сумма 16.5/16.1,
  74:72 16.0/14.2, 74:84 15.3/14.9), атака не просела.
- `web/generated/` = `dist/` (md5 совпадают).
- Точка отката (удачная версия блокфлейты, ДО этой правки):
  `.scratch/known-good/20260913-Recorder-v61/` — `InstrumentLibrary.cpp`
  (md5 `06ff77073015e47aab9f041cc5ae7e9f`) и `IntraSynth-v61.wasm`
  (md5 `760f278f64dfce4471a60a8b466a1235`); эталон сборки — `.scratch/ref-v61/`.
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 211 565 байт**,
  md5 `b9dfcfd72bf38b990293e531b607ea87`, IntraSynth.js 14 367 B (размер
  совпал с v61: менялись только константы).

### Честно открыто

- Слушать не могу. Глубина вибрато у блокфлейты по замеру ВЫШЕ банковской
  (полка 10.3-13.4 против 6.4-10.7 цента rms, то есть ×1.4), но владелец сказал
  «совпадает» — поэтому не трогал. Если вибрато окажется чрезмерным, снимать
  `Value` шагом 0.8× (одна константа в `RecorderVibrato`).
- Флейты 43/115 в этом апдейте не менялись: ждут прослушивания владельцем.

### Файлы

- `intrasynth/src/Intra/Synth/InstrumentLibrary.cpp` — только `RecorderVibrato`
  (Delay/Ramp) и комментарий Update 62.
- Зонд: `.scratch/vib-onset.mjs`.

## Update 63 — пан-флейта: тон ровный, дыхание статично; блокфлейта C6 — вибрато чаще

Владелец: «У Pan Flute в оригинале как будто есть две компоненты. Одна вибрирует,
другая ровная. А у нас всё вибрирует, что делает не похожей её. Recorder почти
идеален, только на C6 мне показалась частота вибрато в сустейне недостаточной».

Замер показал, что «две компоненты» — это буквально: у банка ВЫСОТА тона ровная
(когерентного вибрато нет), а когерентная модуляция живёт в полосах ДЫХАНИЯ.

### Приборы

- `.scratch/pump-check.mjs` (уже был) — глубина вибрато по AMDF целой ноты +
  АМ огибающей (разброс, модуляционный спектр 0.2-20 Гц). Главный прибор этого
  апдейта: мерит ноту целиком, а не полосу.
- `.scratch/pf-am.mjs` (новый) — АМ по полосам: тон h1..h4 (±3 %), горбы
  (k·f0 ±5..15 %) и долины (k+0.5)·f0 (±1.5 %); у каждой полосы уровень, rms АМ и
  три пика модуляционного спектра 2-12 Гц. Показывает, КАКАЯ компонента «дышит».
- `.scratch/pf-split.mjs` (новый) — фазовокодерный IF по полосам. Для картинки
  «что с чем коррелирует» годится, но для глубины на низких гармониках шумноват
  (полоса ±3 % на C4 — меньше бина STFT): выводы делались по pump-check/hnr-audit.

### Замер 1: у банка вибрато ТОНА нет

rms глубины вибрато (AMDF по целой ноте, pump-check; в скобках — пик
модуляционного спектра), центы:

| клавиша | банк | наш v62 | наш v63 |
|---|---|---|---|
| 75:60 | 1.6 (12.08 Гц / 1.3) | 7.4 (6.04 / 10.1) | **2.6** (5.85 / 3.6) |
| 75:72 | 1.3 (5.46 / 1.0) | 5.6 (6.04 / 7.8) | **2.1** (5.85 / 2.6) |
| 75:84 | 1.2 (8.57 / 1.4) | — | **1.5** (5.85 / 1.9) |

То же полосой h1 (hnr-audit, окно 0.8-4.2 с): банк 2.9/2.2/2.1 цента, наш v62
7.8/6.3/4.2, наш v63 **3.6/3.1/2.3**. У банка когерентной линии вибрато нет вовсе
(mag 0.7-1.4 — пол измерения), у нас было 7.8-10.1: качалась вся нота, отсюда
«всё вибрирует». Центроид яркости при этом не поехал: 4.93/2.97/1.18 против
банковских 4.89/3.23/1.17.

### Замер 2: «вибрирует» воздух, а не высота

pf-am 75:72 (окно 1.5-3.5 с), когерентная составляющая на 5.9 Гц:

| полоса | наш v62 | банк |
|---|---|---|
| тон h1 | 0.10 (2.44 Гц) | 0.16 (5.86 Гц) |
| долина 1.5 | 1.45 | 1.52 |
| долина 2.5 | 1.95 (2.93 Гц) | **1.71 (5.86 Гц)** |
| долина 3.5 | 2.14 (3.91 Гц) | **2.78 (5.86 Гц)** |

То есть у банка 5.9 Гц сидит ИМЕННО в полосах дыхания, а тон ровный. У нас общий
LFO тянул за тоном всю гребёнку дыхания: вся картина ехала слитно, «всё вибрирует».
АМ огибающей всей ноты (pump-check): банк 0.72/0.70/1.00 дБ rms против наших
0.42/0.21/0.48 — оригинал «дышит» заметно сильнее нас (см. «Честно открыто»).

### Правки

1. `PanFluteVibrato`: глубина `Value` ×0.35 (было ±11.1 … ±5.5 цента, стало
   ±3.9 … ±1.9) — ровно банковский замер. Частота 6.0 Гц, задержка 0.35 / вход
   0.30 не тронуты.
2. Дыхание пан-флейты (слои N с гребёнкой 0.85 и W с 0.35, `NoiseSampler`) —
   БОЛЬШЕ НЕ НА LFO: гребёнка стоит на месте, шум сам даёт жизнь. Блум-слои
   (вспышка h4 и медленный овершут h1) остались на том же LFO: статичный
   когерентный обертон бился бы с уходящим тоном — это и был «насос» Update 58.
3. `RecorderVibrato`: выше C5 частота вибрато 4.78 → 5.90 Гц, глубина срезана к
   банковской (×0.68 на C6).

### Верификация

- Блокфлейта 74:84: глубина rms **9.0** цента (банк 8.1), частота **5.87 Гц**
  (банк 4.89, владелец просил чаще), когерентность 8.3 (банк 6.0). На 74:86 то же.
  Ниже C5 ничего не менялось (t6 = 0).
- Уровни и атака не поехали: `bank-env` 75:72 и 74:84 по окнам 0-25 … 1000-1500 мс
  побитово те же, что в v62 (75:72: −14.6/−6.4/−4.4/−0.9/+2.1 …, 74:84: 3.5/13.3/
  13.4/13.1/12.2 …).
- `smoke-test-wasm.mjs` — **PASSED** (0 non-finite).
- `click-lr-any.mjs` 75:60/72/84 и 74:84 — **0 кликов** (max|2-я разность|/rms
  0.066-0.218; у 75:84 это тот же 0.126/0.218, что и до правки).
- `web/generated/` = `dist/` (md5 `a4cf587b4df1c9f5ddb388672aa0039c`).
- Точка отката (v62: удачная блокфлейта + прежняя пан-флейта):
  `.scratch/known-good/20260913-Recorder-v62/` — `InstrumentLibrary.cpp`
  (md5 `ecf408a6601877762fbfdb94afddda63`) и `IntraSynth-v62.wasm`
  (md5 `b9dfcfd72bf38b990293e531b607ea87`); эталоны сборки — `.scratch/ref-v62/`
  (до) и `.scratch/ref-v63/` (после).
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 211 603 байта**,
  md5 `a4cf587b4df1c9f5ddb388672aa0039c`, IntraSynth.js 14 367 B (+38 Б к v62).

### Честно открыто

- Слушать не могу. Глубина вибрато тона пан-флейты теперь равна банковской
  (3.6/3.1/2.3 против 2.9/2.2/2.1), то есть вибрато тона практически неслышно —
  как в оригинале по замеру. Если покажется, что у тона пропала жизнь, вернуть
  0.55×/1.0× — одна константа в `PanFluteVibrato`.
- Амплитудный флаттер («дыхание» оригинала) у нас слабее банковского:
  0.42/0.21/0.48 против 0.72/0.70/1.00 дБ rms по огибающей ноты. Это отдельная
  тема: при желании добавлю дыханию лёгкий тремоло-флаттер ~6-8.5 Гц (правка
  движка `NoiseSampler`, порядка +100 Б WASM) — сейчас не добавлял, чтобы не
  трогать «идеальное» дыхание без заявки владельца.
- Флейты 43/115 в этом апдейте не менялись: ждут прослушивания владельцем.

### Файлы

- `intrasynth/src/Intra/Synth/InstrumentLibrary.cpp` — `PanFluteVibrato`,
  два `NoiseSampler` дыхания пан-флейты, `RecorderVibrato`.
- Зонды: `.scratch/pf-am.mjs`, `.scratch/pf-split.mjs`;
  патчи: `.scratch/patch-update63.cjs`, `.scratch/patch-update63b-pfdepth.cjs`.

---

## Update 64 — вибрато флейт переехало с высоты на амплитуду; у пан-флейты тон вибрирует после раздува; атаке блокфлейты добавлен «бугорок»

### Сначала важное: в зондах нашлась ошибка БПФ

В скалярном FFT внутри всех аналитических зондов (hnr-audit, pf-am, pf-split,
rec-vib, spec-peaks) бабочка считалась как `vi = im·ci + re·cr` вместо
`vi = im·cr + re·ci`. Комплексные спектры были неверны, поэтому все выводы
предыдущих апдейтов, опиравшиеся на «когерентные линии» этих зондов, недействи-
тельны — в том числе ключевой вывод Update 63 «у банка когерентного вибрато нет,
это шероховатость».

Ошибка исправлена во всех пяти зондах; поверх написан новый
`.scratch/hvib.mjs` (полосовой детектор ЧМ+АМ по каждой гармонике: аналитический
сигнал → мгновенная частота в центах, модуляционный спектр 1.5-12 Гц → КОГЕРЕНТНАЯ
глубина и её частота; окна «раннее» 0.05-0.55 с и «сустейн» 1.5-4.2 с). Он
откалиброван на синтетике: синусоида 6 Гц с глубиной 6.93 цента даёт ровно
4.90 цента rms и пик 6.06 Гц/6.86.

### Замеры (hvib, сустейн)

| инструмент | наша ЧМ (v63) | банк | наша ЧМ (v64) | наша АМ (v63 → v64) |
|---|---|---|---|---|
| флейта 43:60 | 20.9 ц @4.21 Гц | чистой ЧМ-линии нет, **АМ 2.75 дБ @3.53 Гц** | **7.2 ц** | 0.01 → **3.34 дБ @4.37 Гц** |
| флейта 43:72 | 23.1 ц @5.38 Гц | **АМ 3.43 дБ @7.07 Гц** | **7.9 ц** | 0.01 → **3.46 дБ** |
| флейта 43:84 | 32.7 ц | семпла банка на C6 нет | **11.4 ц** | 0.01 → 3.79 дБ |
| пан-флейта 75:60 | 3.8 ц (владелец не слышит) | АМ 0.13 дБ | **10.0 ц @5.89 Гц** | 0.10 → 0.09 дБ (воздух ровный) |
| блокфлейта 74:72 | 17.1 ц @4.88 Гц | 11.2 ц @5.05 Гц | 17.1 ц (не тронута) | 0.07 дБ |

Волна AM у банка 43 в полосе h1 (2.7-3.4 дБ) и отсутствие чистой ЧМ-линии — это и
есть «завывание привидения» против «приятного» оригинала: у нас по высоте качался
ВЕСЬ нотный ансамбль (до ±32.7 цента на C6 — почти четверть тона), а у оригинала
движется АМПЛИТУДА дыхания/тона при почти ровной высоте.

### Правки

1. **Движок.** `Vibrato` получил `Tremolo` (относительное отклонение громкости),
   `VibratoLfo` — нормированный до ±1 осциллятор и поле `LastGated`
   (`Next()` возвращает `LastGated*Value`, поэтому ЧМ-ветвь считается как
   раньше). `NoiseSampler::ReadNext` умножает сэмпл на `1 + Tremolo*LastGated`.
   То же добавлено в вибрато-ядра вейвтейбла (`MultiplyAddVibrato`,
   `MultiplyAddVibratoStereo`): новые параметры `vibValue` и `tremolo`,
   `mFreqOscillator` нормирован, масштаб ЧМ переехал в `mVibratoValue`.
   При `Tremolo == 0` путь побитово прежний.
2. **PanFluteVibrato** — тону возвращена слышимая глубина (0.35× → `±10.0 … ±6.6`
   цента) и вход отодвинут за раздув: `Delay 0.35 → 0.45`, `Ramp 0.30 → 0.35`
   (полная к 0.80 с). Дыхание с Update 63 статично (LFO ему не передаётся),
   `Tremolo` не задан — «воздух ровный» сохранён.
3. **FluteTitanicVibrato** (43/115) — ЧМ срезана в 2.9 раза (`±6.9 … ±11.5` цента
   вместо ±19.9 … ±32.9), а АМ того же LFO (`Tremolo = 0.35` ≈ ±2.6/−3.1 дБ)
   отдана и тону, и дыханию: теперь «тон ровный, а вибрирующая часть» есть и
   звучит как у банка.
4. **Recorder — атака.** `EnvelopeProfile`: пик ×1.7 прежнего конца первого
   сегмента за `0.55·T0`, провал до ×1.05 за `0.35·T0`, затем один
   экспоненциальный раздув. Длительность раздува — по зонам
   (`zT3 = {0.050, 0.028, 0.028, 0.026, 0.023, 0.020, 0.017}`), потому что банк
   входит очень по-разному: 74:72 (C5) — 0.028 → 0.6 за 22 мс, а 74:62 (D4) —
   за ~50 мс.

### Верификация

- **Форма атаки** (`.scratch/rec-atk.mjs`, окна 2 мс, дБ отн. 2.0-2.5 с, 74:72):
  банк −30.0 (4 мс) / −17.1 (10) / −15.5 (12) / −17.6 (18, провал) / −7.4 (24) /
  +8.9 (40); у нас было −30.4 / −23.2 / −21.8 / −19.5 / −12.7 / +7.1 — стало
  −33.8 / −13.6 (10) / −17.7 (18, провал) / −12.4 (22) / +8.7 (40): бугорок и
  провал на месте, окна 40-120 мс совпадают с банком в ±0.2 дБ.
- **Окна огибающей** (`bank-env`): 74:72 Δ = +1.5/+1.2/+0.7/+0.1, 74:84 =
  −0.3/+1.5/−0.1/−0.2, 74:67 = −3.4/−3.4/+0.5/+0.1; 74:60 (D3) — +5.2 дБ в
  75-100 мс (см. «Честно открыто»).
- **Появление вибрато пан-флейты** (`vib-onset.mjs`, блоки 0.26 с): v63 держал
  3.1/2.5/2.7/3.3 цента с самого начала, v64 — 2.8/2.6/3.1/4.5/6.8/7.4/7.4/7.5,
  то есть вибрато входит с 0.4-0.5 с и выходит на полку ~7.1 цента (rms) к 1.0 с;
  блоки атаки (0.26-0.65 с) не изменились — уровень атаки не поехал.
- **Блокфлейта не пострадала**: 74:72 ЧМ 17.1 ц @4.88 Гц, 74:84 — 12.4 ц @5.89 Гц
  (совпадает с профилем Update 63).
- `smoke-test-wasm.mjs` — **PASSED** (0 non-finite).
- `click-lr-any.mjs`: 75:60/72/84, 43:60/72/84, 115:72, 74:60/84 — **0 кликов**;
  74:72 — 400/383 срабатывания относительного порога в 0.244-0.247 с (это конец
  раздува; у v63 было 320/367 в 0.264-0.271 с при той же амплитуде второй
  разности 0.091 → 0.089, то есть это «чифф», а не разрыв).
- `web/generated/` = `dist/` (md5 `a6458c718e61f4f119595060e7a68c5c`).
- Точка отката (v63: прежние профили + прежняя атака):
  `.scratch/known-good/20260913-WindVibrato-v63/` — `InstrumentLibrary.cpp`,
  `IntraSynth-v63.wasm`, `IntraSynth.js`; эталон сборки — `.scratch/ref-v63/`.
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 212 119 байт**,
  md5 `a6458c718e61f4f119595060e7a68c5c`, IntraSynth.js 14 367 B (+516 Б к v63:
  +399 Б АМ-ветвь у дыхания, +116 Б у вейвтейбла и остальное — атака блокфлейты).

### Честно открыто

- Слушать не могу. По флейтам 43/115 решение опирается на замер: у банка в полосе
  h1 нет чистой ЧМ-линии, но есть когерентная АМ 2.7-3.4 дБ — мы её теперь
  повторяем (3.3-3.8 дБ), а высоту держим почти ровной. Если «завывание» шло не
  от глубины ЧМ, а от чего-то другого, откат — одна строка в
  `FluteTitanicVibrato`.
- Пан-флейта: замер (уже исправленным зондом) по-прежнему показывает у банка
  ROVNÝ тон (когерентная ЧМ ≤1.5 цента), а у нас теперь 10.0 — то есть мы
  сознательно отошли от замера в пользу вашего слуха («вибрато именно у
  основного тона после раскачки»). Если оно окажется слишком заметным, множитель
  `Value` в `PanFluteVibrato` уменьшается в одну строку.
- Регистр D3/D4 блокфлейты: наш раздув теперь быстрее банковского на ~4-5 дБ в
  окне 75-100 мс (у банка D3 входит медленнее всех). Профиль входа по зонам
  D3/A4/B4 у банка разный ВНУТРИ одной зоны (74:60 и 74:62 ведут себя по-разному),
  одной экспонентой это не воспроизводится.
- Флейты на C6: у банка нет семпла 43 на этой клавише (тишина в рендере), так что
  верхний регистр 43/115 остаётся без сверки с банком.

### Файлы

- `intrasynth/src/Intra/Synth/InstrumentLibrary.cpp` — `PanFluteVibrato`,
  `FluteTitanicVibrato`, `EnvelopeProfile` блокфлейты.
- `intrasynth/src/Intra/Synth/WaveTableSampler.h/.cpp`,
  `intrasynth/src/Intra/Synth/ComputeKernels.h` — АМ-ветвь вибрато.
- `intrasynth/src/Intra/Synth/Synth.h` — АМ в `NoiseSampler::ReadNext`.
- Зонды: `.scratch/hvib.mjs` (новый), `.scratch/rec-atk.mjs` (новый);
  исправлены `hnr-audit.mjs`, `pf-am.mjs`, `pf-split.mjs`, `rec-vib.mjs`,
  `spec-peaks.mjs`.
- Патчи: `.scratch/patch-update64.cjs`, `64b`…`64f`, `64c-toneam.cjs`,
  `.scratch/worklog-update64.cjs`.

---

## Update 65 — общая библиотека анализа; атака блокфлейты по фиту; тремоло флейт тише и позже; у пан-флейты дышит ВОЗДУХ

Запрос владельца: (1) у флейты 43/115 тремоло слишком глубокое и почти сразу
начинается, не хватает раскачки; (2) у пан-флейты тремоло не слышно, зато слышно
«какой-то левый период»; (3) у блокфлейты бугорок в оригинале длиннее, у нас
быстро спадает; (4) «почему у нас постоянно такие тривиальные ошибки? может уже
сделать js библиотеку, в которой будут все нужные строительные блоки?».

### 0. Библиотека анализа (intrasynth/tools/analysis/)

Новый каталог вместо одноразовых зондов: `lib/fft.mjs` (БПФ/ОБПФ, наивное ДПФ,
аналитический сигнал, модуляционные пики, автокорреляция), `lib/dsp.mjs`
(биквадраты, полосовой фильтр, огибающая, `detrend`, окна уровней, центы),
`lib/render.mjs` (MIDI, рендер нашим WASM и банком, чтение WAV),
`lib/harmonics.mjs` (по-гармонический ЧМ/АМ-аудит, дорожка тремоло),
`cli.mjs` (`selftest`, `vib`, `trem`, `env`, `attack`, `mod`, `period`),
`fit-attack.mjs` (численный фит формы атаки), `README.md`.

**Три ошибки, которые библиотека ловит на себе:**

1. **БПФ** — в прежних зондах бабочка считалась как `vi = im*ci + re*cr`
   вместо `vi = im*cr + re*ci`. Комплексные спектры были мусором. Теперь одна
   реализация, и `selftest` сверяет её с наивным ДПФ (max|Δ| = 6e-15) и по
   round-trip.
2. **`detrend`** — первая версия делила сумму окна на неверную длину и давала
   на выходе значения ×1e5; замена на скользящее среднее оставляла на краях
   ±половину размаха, зеркалирование — ±четверть. Итог — МНК-прямая в окне
   (линейный тренд снимается точно, `max|остаток| = 3.4e-12`), и в `selftest`
   есть отдельная проверка.
3. **Время считалось от начала рендера, а не от note-on** (MIDI-нота стоит на
   0.2 с). Из-за этого `attack`/`env` первые 200 мс читали тишину и возвращали
   ровную полку −200 дБ, а `vib` сдвигал оба окна на 0.2 с. Теперь всё время в
   CLI — от note-on (`--noteon`, по умолчанию 0.2 с), это написано и в README.

### 1. Блокфлейта: форма атаки подобрана численно

Замер `attack 74:72 --len 2` (после починки времени): банк −43.9/−30.0/−32.3/
−20.8/−17.1/−15.5/−15.9/−16.9/−17.6/−16.3/−7.4/−1.6/+8.9 (2…40 мс), у нас было
−24.8/−20.4/−17.5/−15.1/−13.5/−14.1/−15.5/−16.9/−17.8/−15.0/−10.2/−2.5/+8.5 —
то есть **+19 дБ в 2 мс, пик на 2 дБ выше и на 2 мс раньше, спад с пика вдвое
короче**, а к 44 мс мы уже стояли на полке, тогда как банк подходит к ней к ~70 мс
(владелец: «бугорок у оригинала длиннее, у нас быстро спадает»). Прежняя правка
(Update 64) целилась в 0.036 по пику, но ставила множитель 1.70 от zV0 = 0.045.

Вместо ручной подгонки — `fit-attack.mjs`: снимает форму банка окнами 2 мс,
нормирует на полку и подбирает старт/пик/провал/времена. Форма атаки теперь:
тихий старт (StartVolume) → экспоненциальный подъём к пику → спад до провала →
экспоненциальный раздув на полку. Уровни выставлены по замеру петли
Δ(наш−банк) в 8-16 мс (было +0.2…+7.2 дБ, D3 пересвечивал на 4.7 — снято), t1
9-10 → 11 мс (все клавиши пересвечивали на 4-9 дБ в 6 мс).

Итог по 7 опорным клавишам (62/67/70/72/74/79/84), Δ в 2-20 мс:

| клавиша | было (средн 8-16 мс) | стало |
|---|---|---|
| 62 D3 | +7.2 | **+4.7** (банк там сам ступает, модель 5 сегментов её не повторяет) |
| 67 A4 | — | −1.7 |
| 70 B4 | — | −1.1 |
| 72 C#5 | −2.3 | **−0.2** |
| 74 D5 | — | −3.9 |
| 79 A5 | — | −3.8 |
| 84 A#5 | +2.0 | **−0.8** |

Полная форма 74:72 (наш/банк, 2…88 мс): −41.2/−36.1/−29.3/−22.4/−17.3/−17.0/
−17.6/−17.8/−16.2/−13.4/−9.2/−2.5/+7.0/+13.6 против −43.9/−30.0/−32.3/−20.8/
−17.1/−15.5/−15.9/−16.9/−17.6/−16.3/−7.4/−1.6/+8.9/+12.1.

### 2. Флейты 43/115: тремоло тише и после раскачки

`trem 43:60 --block 0.4` (полоса h1, блоки 0.4 с): у нас было ровное
2.5 дБ rms / 3.6 дБ когерентно уже с 0.4 с; у банка тремоло РАЗГОНЯЕТСЯ —
0.4 с 3.6/4.2, 0.75 с 4.9/6.8, 1.0 с 2.0/2.3 (провал), 1.5-2.25 с 4.5-5.2/6.3-6.5.
Теперь у нас 0.4 с 0.81/0.86, 0.6 с 1.33/1.77, 0.8 с 1.50/2.22 и полка 1.5/2.2:
глубина срезана 0.35 → 0.22, вход Delay 0.20/0.18 → 0.42/0.39, Ramp 0.15/0.13 →
0.45/0.42 (полная к ~0.85 с вместо 0.35 с).

### 3. Пан-флейта: тон ровнее, а «тремоло» — это ВОЗДУХ

Модуляционный спектр ЧМ-траектории h1 (0.8-4.2 с, `modPeaks`): у нас была ОДНА
жёсткая когерентная линия **5.97 Гц / 10.18 цента**, у банка когерентного ЧМ нет
вовсе (топ-пики 1.2-1.5 цента на 12-20 Гц — шероховатость). Именно такая
идеально периодическая модуляция и читается как механический «левый период», а не
как вибрато. Глубина срезана до **±4.0…±2.6 цента** (линия на 8.5 дБ ниже).

Где у банка движение: полосы ДЫХАНИЯ. Замер (АМ полос, 1.0-4.4 с): 2-5 кГц
**1.90 дБ когерентно на 4.04 Гц**, 5-9 кГц 0.98, 1-1.6 кГц 0.79, а у нас было
0.49/0.35/0.32. Добавлен `PanFluteBreathVibrato` — тот же LFO, но **только АМ-ветвь**
(`Value = 0`, `Tremolo = 0.24`, 4.04 Гц, Delay 0.35/Ramp 0.45), и он отдан обоим
слоям дыхания (N и W). Стало: 2-5 кГц **1.36 дБ @4.29 Гц**, 5-9 кГц 2.20, а
полоса h1 (сам тон) не изменилась — 0.32-0.35 дБ rms без когерентного пика,
как и у банка (0.34-0.36). То есть воздух тремолирует, высота стоит.

Проверка «левого периода» (`period`, автокорреляция сустейна): у нас корреляция
на лаге шумовой таблицы **32768 = −0.56, 65536 = +0.02**, у банка −0.09/−0.65 —
то есть повтор таблицы в рендере НЕ читается (вибрато размазывает точку лупа),
и единственная периодическая структура в тоне — та самая линия 6 Гц, которую мы
срезали. Атака пан-флейты не изменилась: рендер v63 и v65 в 0-130 мс совпадают
до 0.1 дБ (разница с банком там 7-12 дБ — это старый открытый пункт Update 44-58).

### Проверка

- `node intrasynth/tools/analysis/cli.mjs selftest` — **PASSED** (БПФ против наивного ДПФ,
  round-trip, синтетическая ЧМ 6 Гц / 7 центов, `detrend` линейного тренда и
  быстрой синусоиды).
- `scripts/smoke-test-wasm.mjs` — **PASSED** (0 non-finite).
- Клик-зонд 75:60/72/84, 43:60/72, 74:72/84 — **0** кроме 74:72, где срабатывает
  относительный порог на раздуве: `click-zoom` в 0.246 с даёт гладкий профиль
  1-мс окон (max|d2|/rms 0.032-0.082, нет изолированных выбросов).
- `web/generated/` = `dist/` (md5 совпадают).
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 212 179 байт**,
  md5 `bd73c076cfe99f3508013b6723333cdc`, IntraSynth.js 14 367 B (+60 Б к v64 —
  только таблицы, движок не тронут).
- Точка отката: `.scratch/known-good/20260913-WindTremolo-v65/`; эталон сборки —
  `.scratch/ref-v65/`.

### Открыто / честно

- Слушать не могу. Пан-флейта: тон я срезал ДО уровня банка (у банка когерентного
  ЧМ нет), а слышимое движение отдал дыханию. Если «левый период» был не FM-линией
  тона, а чем-то ещё, — скажите, чем он слышится (щелчки? гудение? «вау-вау»?),
  и я буду искать замером именно это; откат глубины — одна константа.
- Тремоло флейт теперь НИЖЕ банковского (2.2 дБ против 3-6 дБ у банка в пиках).
  Срезал по вашим словам «слишком глубокое»; если на слух стало бледно — подниму
  `Tremolo` 0.22 → 0.30 одной строкой.
- Блокфлейта: остаются 3-5 дБ расхождения в 14-20 мс на D5/A5 и +4.7 в среднем на
  D3 (у банка там немонотонная ступенька, которую 5-сегментная огибающая не
  описывает). Форма «бугорка» при этом совпала: 72-я клавиша в 2-20 мс в ±1.6 дБ.
- Атака пан-флейты на 7-12 дБ громче банка в 0-40 мс — пункт НЕ этого апдейта
  (рендеры v63/v65 там совпадают), он тянется с Update 44-58.

### Файлы

- `intrasynth/tools/analysis/` — новый каталог: `README.md`, `cli.mjs`,
  `fit-attack.mjs`, `lib/{fft,dsp,render,harmonics}.mjs`.
- `intrasynth/src/Intra/Synth/InstrumentLibrary.cpp` — таблицы атаки блокфлейты
  (`recAtk*`), `FluteTitanicVibrato`, `PanFluteVibrato`,
  новый `PanFluteBreathVibrato` и его подключение к слоям дыхания N/W.
- Патчи: `.scratch/patch-update65.cjs`, `65b`, `65c`, `65d`,
  `65e-comments.cjs`; worklog — `.scratch/worklog-update65.cjs`.


## Update 66 — починен численный вырожденческий «джиттер»; у пан-флейты тремоло в ТОНЕ, воздух статичен; Recorder: шум сустейна гаснет вместе с тоном

Запрос владельца: (1) у Recorder многовато шума в сустейне; (2) у Pan Flute не
слышно тремоло основного тембра, зато слышна «неприятная периодичность воздуха …
с неприятным подтоном, может алиасинг», а у оригинала всё ровно и тремолит
именно основной тембр; (3) дыхание «очень сильное и низкое, напоминает белый шум
при частоте дискретизации 8 кГц»; (4) у флейт 43/115 тремоло всё ещё слишком
глубокое, у оригинала мягче — «ищи ошибку в измерениях … а может оно быть другой
формы, например, не синусом, а мягче?».

### 0. ГЛАВНОЕ: ошибка в движке, а не в инструменте

Update 66 добавил «мягкую» форму модуляции — множитель глубины (1 + Jitter·w),
где w — медленная (0.55 Гц) синусоида из второго Intra::SineRange. Замер сразу
показал абсурд: у флейты 43:72 широкополосная АМ (300-4000 Гц, блок 0.5 с)
вместо банковских 1.15-1.58 дБ rms дала **2.94 → 4.33** дБ и росла по блокам, а
тон пан-флейты «раскачивался» 1.55 → 3.66 дБ.

**Причина — не в инструменте, а в арифметике.** SineRange — рекурсия
s[n+2] = mK·s[n+1] − s[n] с mK = 2·cos(dphi). На 0.55 Гц
dphi = 2π·0.55/44100 = 7.8e-5, то есть 2 − mK = 6.1e-9. Ближайший float32 к 2.0
снизу отстоит на 2.38e-7 (один ULP), поэтому mK округляется РОВНО до 2.0, и
рекуррентность превращается в s[n] = n·amplitude·dphi — **линейный рост без
ограничения**. Это и есть «мера глубины», уезжающая вверх со временем.

Исправление (Update 66b): дрожание — фазовый аккумулятор со сглаженным
треугольником (C¹ на стыке, без трансцендентных вызовов на семпл: Math::Sin на
каждый сэмпл каждой ноты был бы дорог). Фаза копится в [0, 2π) с вычитанием,
поэтому точность не теряется никогда. При Jitter == 0 выход прежний побитово.

### 1. Зонд врал: срезы «floor» при высоком f0 пусты

Владелец прав, что «ищи ошибку в измерениях». Прежний по-полосный замер дыхания
(interHarmonicFloor) исключает бины в пределах 0.15·f от любой гармоники. При
f0 = 523 Гц (C5) это ±78…±525 Гц — то есть **почти все бины выше 1 кГц**, и срезы
1500-3500 / 3500-8000 выходили пустыми («—» в отчёте). Полоса 400-600 измерялась,
а весь верх дыхания — нет.

Добавлена команда CLI **bands** (bandNoiseFloor): допуск узкий АБСОЛЮТНЫЙ (±25 Гц
вокруг гармоник), уровень — медиана бинов полосы (устойчиво к остаткам пиков),
dB отн. пика h1. Ею и мерено дыхание ниже.

### 2. Recorder: шум сустейна

Замер floor 74:72, дБ отн. h1:

| сустейн | 300-600 мс | 1000-1400 | 2000-2400 | 3000-3400 |
|---|---|---|---|---|
| банк | −67.0 | −67.2 | −66.6 | −67.6 |
| наш до | −65.5 | −61.7 | −55.7 | −51.1 |
| **наш после** | **−66.9** | **−65.0** | **−65.2** | **−66.7** |

Причина: у тона есть экспоненциальное затухание (6.2 дБ/с), а дыхание жило на
ПОСТОЯННОЙ подушке 0.62 — шум оставался, пока тон падал. Теперь у дыхания то же
затухание (1.0 → 0.12 за 3 с), а уровень домножен на 0.786, чтобы в 0.45 с
осталось прежнее значение.

### 3. Pan Flute: тремоло в тоне, воздух статичен

Замер bands (сустейн 1.4-4.0 с, дБ отн. пика h1, 75:72):

| полоса | наш | банк |
|---|---|---|
| 400-600 Гц | −42.2 | −48.1 |
| 600-1200 | −48.7 | −58.5 |
| 1200-2400 | −52.8 | −56.5 |
| 2400-4800 | −62.0 | −68.2 |
| 4800-9600 | −82.2 | −95.5 |
| 9600-16000 | −100.1 | −108.0 |

Сделано: (а) АМ-слой дыхания из Update 65 **убран** — именно он давал боковые
полосы f0±4 Гц вокруг каждой гармоники, то есть «неприятную периодичность воздуха
с подтоном»; (б) тон получил и глубину (±10…±6.6 цента), и АМ (Tremolo 0.10), и
дрожание (Jitter 0.50) — теперь он модулируется, а воздух стоит; (в) пятый полюс
ФНЧ дыхания снял 4.8-9.6 кГц на 4.8 дБ и 9.6-16 на 8.2.

Широкополосная АМ тона (300-4000, блок 0.5 с) после починки дрожания: **0.41 …
0.93 дБ rms** (банк 0.20) — тремоло слышимо и больше НЕ растёт по блокам.

### 4. Флейты 43/115: тремоло тише и нерегулярнее

Замер широкополосной АМ 300-4000 (блок 0.5 с), 43:72:

| | 0.5 с | 1.0 с | 1.5 с | 2.0 с |
|---|---|---|---|---|
| банк | 1.15 | 1.42 | 1.53 | 1.51 |
| v65 (Tremolo 0.22) | 1.17 | 1.17 | 1.23 | 1.15 |
| v66a (сломанный джиттер) | 2.5 | 3.2 | 3.6 | 4.3 |
| **v66b** | **0.55** | **0.83** | **0.43** | **0.69** |

То есть по rms v65 УЖЕ совпадал с банком (1.2 против 1.2-1.6), а владелец всё
равно слышал «слишком глубоко» — потому что у нас это был жёсткий синус (индекс
тональности 0.98-1.01), а у банка модуляция нерегулярна (0.66-0.70) и читается
мягче. Теперь глубина срезана (Tremolo 0.22 → 0.13) И модуляция нерегулярна
(Jitter 0.60, теперь реально работающий). 115:72 даёт те же цифры (0.43 … 1.09).

### Проверка

- smoke-test-wasm.mjs — **PASSED** (0 non-finite);
- клик-зонд: 75:60/72/84, 43:60/72, 115:72, 74:60/84 — **0 кликов**;
- 74:72 — прежний «чифф» атаки на 0.246 с (max|2-я разность|/rms 0.088 L /
  0.180 R), это не разрыв, а форма атаки (Update 64/65), не вырос;
- web/generated == dist (md5 bbeedd6057341aba36655662737a4dc1);
- откат: .scratch/known-good/20260913-WindVibrato-v66/ (InstrumentLibrary.cpp,
  WaveTableSampler.h, IntraSynth-v66.wasm); эталон предыдущей — ref-v65/.

**WASM (канон: plain -Os, INTRA_PIANO_ALL_TABLES=ON) = 212 539 байт**,
md5 bbeedd6057341aba36655662737a4dc1, IntraSynth.js 14 367 B
(+360 Б к v65: ветвь дрожания в движке).

### Честно открыто

- Слушать не могу. Тремоло флейт 43/115 теперь **в 2.5 раза тише банка**
  (0.43-0.69 против 1.15-1.58 дБ rms) — это сделано по двум просьбам «мягче»;
  если окажется бледно, это одна константа (Tremolo 0.13 → 0.20).
- Тремоло тона пан-флейты (0.41-0.93 дБ) — сознательное отступление от замера
  банка (0.20): владелец просил его СЛЫШАТЬ. Если много — Tremolo 0.10 вниз.
- Воздух пан-флейты всё ещё на 5-13 дБ громче банка и наклонён вниз (400-600 и
  600-1200 перевешены сильнее всего) — этим апдейтом НЕ закрыто.
- Дрожание — гладкий треугольник 0.55 Гц; если оно само слышится как медленное
  «качание», Jitter уменьшается одной константой.
- Флейты 43/115 в остальном не тронуты; Recorder, кроме затухания дыхания, не
  тронут.


## Update 67 — частота вибрато пан-флейты задана кривой по замеру банка

Запрос владельца: «Pan Flute как будто стала слишком часто вибрировать».

### Причина: частота вибрато была ПОСТОЯННОЙ на всех нотах

У пан-флейты стояло r.Frequency = 6.00 Гц для любой ноты. Замер `mod`
(когерентный пик огибающей, окно 1.5-4.0 с от note-on, полоса 0.3-20 Гц)
показал, что у банка частота растёт с регистром, причём ступенями:

| клавиша | нота | банк, Гц | v66 (было) | v67 (стало) |
|---|---|---|---|---|
| 55 | G3 | 3.00 | 5.97 | 3.53 |
| 60 | C4 | 4.04 | 5.97 | 4.29 |
| 65 | F4 | 5.38 | 5.97 | 5.47 |
| 67 | G4 | 6.06 | 5.97 | 5.97 |
| 72 | C5 | 5.55 | 5.97 | 5.55 |
| 77 | F5 | 7.32 | 5.97 | 7.32 |
| 79 | G5 | 6.39 | 5.97 | 6.48 |
| 84 | C6 | 8.50 | 5.97 | 8.75 |

То есть на нижних нотах мы были быстрее банка в 1.5-2 раза (это и есть «слишком
часто вибрирует»), а на C6 — на 30 % медленнее.

Внутри зон банка отношение «частота вибрато / f0» постоянно: 0.0153 для
G3-G4, 0.0106 для C5-F5, 0.0082 для G5-C6. Это подпись вибрато, записанного В
СЕМПЛ и транспонируемого вместе с ним: внутри зоны частота следует за
проигрыванием (пропорциональна f0), а на границе зоны меняется скачком. Поэтому
кривая воспроизведена узлами ровно по замеренным клавишам (G3/C4/F4/G4/C5/F5/G5/C6,
x — полутоны от C4), а не одной формулой.

Механика: `PanFluteVibrato(freq)` вызывается на КАЖДУЮ ноту (GenericInstrument
получает реальную частоту), поэтому кривая работает понотно, а не по зонам.
Добавлено forward-объявление MixParam (определение ниже по файлу, а профиль
вибрато записан выше него).

### Что НЕ менялось

Глубина (±10.0…±6.6 цента), АМ (Tremolo 0.10), дрожание (Jitter 0.50) и вход
(0.45+0.35 с) — как в v66. Дыхание по-прежнему статично (LFO ему не задан).
Тронута только строка частоты, так что на C6 и выше прежняя жалоба «на C6 атака
быстрее, чем в оригинале» этим не закрыта и не ухудшена.

### Проверка

- `smoke-test-wasm.mjs` — PASSED (0 non-finite);
- `click-lr-any` 75:60/72/84 — 0 кликов (max|2nd|/rms 0.049/0.070/0.109);
- `web/generated/` = `dist/` (md5 совпадают);
- откат: `.scratch/known-good/20260913-PanFluteRate-v67/` (InstrumentLibrary.cpp,
  IntraSynth-v67.wasm); точная точка возврата к прежней частоте — одна строка
  r.Frequency = 6.00f.

**WASM (канон: plain -Os, INTRA_PIANO_ALL_TABLES=ON) = 212 673 байт**,
md5 e22506b76f4ec1c2b03ee3b0383188e6, IntraSynth.js 14 367 B
(+134 Б к v66: восемь узлов кривой частоты).

### Честно открыто

- Слушать не могу. Кривая взята из замера банка; если на слух теперь «слишком
  медленно» на низах — это масштаб таблицы vibRateV, а не структура.
- Глубина вибрато (±10…±6.6 цента) по-прежнему в 7-10 раз больше замеренной у
  банка (1.0-1.5 цента): оставлена по прежним просьбам «вибрато не слышно».
  Замедление делает её ЗАМЕТНЕЕ на цикл — если стало слишком глубоко, глубина
  уменьшается одной константой r.Value.
- АМ тона (0.7-0.9 дБ) осталась в 2-5 раз глубже банковской (0.13-0.49 дБ).


## Update 68 — порог «тремоло» на C4 и провалы глубины (виноват Jitter)

Запрос владельца: «Теперь у Flute и Pan Flute C4 не слышу тремоло вообще, у C5,C6
вроде норм, но наверное чуть чаще, чем у оригинала».

### 1. Найдены ДВА дефекта, и оба измеримые

**а) Глубина модуляции дрожит втрое сильнее банковской и в провалах пропадает.**
Замер широкой полосы (`trem --wide`, блоки 0.5 с, время от note-on):

| | мин…макс rms | размах | среднее |
|---|---|---|---|
| банк 43:60 (300-4000) | 1.12…1.71 дБ | 1.5× | 1.42 |
| наш v67 43:60 | 0.53…1.42 дБ | **2.7×** | 0.87 |
| банк 75:60 (300-1500) | 0.81…0.83 дБ | 1.5× | 0.82 |
| наш v67 75:60 | 0.59…1.27 дБ | **2.2×** | 0.85 |

Причина — `Jitter`: множитель глубины идёт от (1−Jitter) до (1+Jitter), то есть
при 0.50-0.60 это 0.4…1.6 — в провалах глубина падала НИЖЕ порога слышимости, и
тремоло «мигало». У банка эквивалент ≈0.2. Поставлено 0.20 у обоих инструментов.

**б) Частота на C4 ниже порога «тремоло».** У владельца C5 (5.4-5.5 Гц) и C6
(8.1-8.8) слышатся, а C4 (4.3-4.4) — нет; порог лежит между. Поднято: пан-флейта
C4 4.00 → 5.00, флейты 43/115 4.30 → 4.95 (кривая 4.95 + 0.25x). Верх наоборот
срезан по той же жалобе «чуть чаще»: пан-флейта C6 8.58 → 8.10, C5 5.55 → 5.40;
флейты C6 6.00 → 5.45.

### 2. Глубина тремоло флейт: 0.13 → 0.18

У флейт 43/115 при Tremolo 0.13 замер давал 0.75…1.02 дБ rms против банковских
1.12…1.71 (в среднем 1.42). То есть мы были на 40 % тише, при этом владелец
пишет «тремоло не слышу». 0.18 даёт ≈1.2 дБ rms и когерентный пик ≈2.0 дБ —
внутри банковского диапазона 1.6-2.6. История просьб владельца по этому
параметру: 0.35 «слишком глубоко» → 0.22 «всё ещё глубоко» → 0.13 «не слышу»;
взят шаг назад к 0.18. Пан-флейта по глубине НЕ тронута (там мы и так на уровне
банка: 0.82 против 0.82 в полосе тона).

### 3. Что НЕ сходится с замером (честно)

- **Пан-флейта, C4: у банка есть ВТОРАЯ, быстрая составляющая 11.4 Гц** (полоса
  300-1500, когерентный пик 0.63 дБ, ровно с 0.25 с), которой у нас нет вовсе —
  у нас там чистая линия 4-5 Гц. На G3 (55) та же пара: 3.0 Гц и 9.0 Гц.
  Отношение 9.0 → 11.94 ровно равно 2^(5/12) = 1.335, то есть ВТОРАЯ линия
  масштабируется вместе с транспозицией семпла: длина лупа ~4900 семплов,
  частота = 44100·(f/f_root)/L. Похоже, это **шум петли семпла банка**, а не
  музыкальное тремоло. Поэтому не воспроизведено. Если владелец слышит в
  оригинале на C4 именно эту быструю дрожь — это отдельная правка (вторая
  составляющая модуляции в движке).
- Флейты 43/115 на C5: у банка быстрая АМ ~7 Гц (широкая полоса) при медленной
  ЧМ ~2-4 Гц, у нас одна линия 5.4 Гц. Владелец говорит «чуть чаще оригинала» —
  по замеру наоборот; приоритет отдан слуху, шаг небольшой.

### Проверка

- `smoke-test-wasm.mjs` — PASSED (0 non-finite);
- `click-lr-any` 75:60/72/84, 43:60/72, 115:72, 74:84 — 0 кликов;
- `web/generated/` = `dist/` (md5 совпадают);
- откат: `.scratch/known-good/20260913-WindTremolo-v68/` (InstrumentLibrary.cpp,
  IntraSynth-v68.wasm); предыдущая точка — `.scratch/known-good/20260913-PanFluteRate-v67/`.

**WASM (канон: plain -Os, INTRA_PIANO_ALL_TABLES=ON) = 212 673 байт**,
md5 905b4c3021728998b4efdc64d7cf1e94, IntraSynth.js 14 367 B (размер не
изменился: правились только константы профилей).

### Честно открыто

- Слушать не могу. Частота на C4 (4.95-5.00 Гц) ВЫШЕ банковского замера
  (3.5-4.0 Гц) — сделано по прямой просьбе владельца «на C4 не слышу тремоло».
  Откат — одна таблица/одна константа.
- Быстрая составляющая 11-12 Гц на C4 пан-флейты (см. выше) не воспроизведена.
- Дрожание глубины снижено (0.2) — тремоло стало ровнее; если владелец хотел
  «мягче/нерегулярнее» (просьба Update 66), это возврат в ту сторону назад.


## Update 69 — у флейт 43/115 найдены ДВА измеренных дефекта: дыхание тише банка на 14 дБ и перевёрнутый спектр гармоник

Запрос владельца: (1) тремоло флейт так и не чувствуется, не хватает глубины;
(2) у 43/115 не тот тембр, не хватает дыхания, «может и с самим тоном что-то не
так»; (3) шум шва петли не нужен; (4) вопрос: не даёт ли IFFT-период и тремоло
вместе другой воспринимаемый период, и меряем ли мы наши рендеры вообще.

### 0. Ответ на вопрос про измерения и период

CLI меряет ОБА рендера в каждом выводе: `renderPair` строит наш звук из WASM и
банк из SF2 через fluidsynth, и команды печатают блоки «наш» и «банк» друг под
другом. Всё в этом апдейте — из сравнения двух рендеров.

Совпадения «IFFT-период × тремоло» нет и быть не может: таблица (`BuildWaveTable`,
16384 семплов) содержит РОВНО ОДИН период ноты, то есть её луп и есть период
ноты; базис квантуется на сетку ДПФ, а высоту даёт дробная скорость
воспроизведения (0.0022 сверх 1.0 на C4), так что сверх-period от дробной части
вышел бы ~167 с. Зонд `period` показывает единственную реальную периодичность
нашего рендера — сам тремоло (лаг 9102 = 206.4 мс = 4.85 Гц; у банка в топе
лад 168 = период ноты). Хит на лаге 58660 (1.33 с) — артефакт окна 2 с; прямой
замер модуляционного спектра 0.05-3 Гц это подтверждает: у нас там 0.03-0.12 дБ
(пол), у банка — реальная составляющая 2.69 Гц с 0.89-1.61 дБ.

ВАЖНОЕ, что замер всё же нашёл: у банка на C4 ДВЕ когерентные составляющие
(2.69 Гц 1.2-1.6 дБ в полосе тона и 11.4 Гц в полосе h1), у нас — одна (4.71 Гц).
То есть тремоло банка нерегулярно, наше — чистый синус.

### 1. Дыхание флейт: тише банка на 12-18 дБ

Замер CLI `bands` (сустейн 1.4-3.5 с, дБ отн. пика h1, наш → банк):

| полоса | 43:60 | 43:65 | 43:72 |
|---|---|---|---|
| 400-600 | −60.8/−47.2 | −59.7/−45.9 | −58.6/−40.0 |
| 600-1200 | −62.8/−50.2 | −58.8/−48.9 | −61.1/−48.0 |
| 1200-2400 | −63.9/−50.6 | −61.6/−51.4 | −63.4/−52.1 |
| 2400-4800 | −70.3/−55.1 | −67.9/−53.3 | −69.9/−53.1 |

Причина ровно та, о которой догадался владелец («мы под Pan Flute подогнали, а
тут нет»): у пан-флейты уровень дыхания 0.045×1.90 = 0.0855, у флейт
0.0325×0.50 = 0.016 — в 5.3 раза тише (14.6 дБ). Слой дыхания флейт остался от
решения Update 31 «в сустейне шума нет», а пан-флейте дыхание вернули (42/56).
Множители подняты по замеру: lvlK 0.50/1.15/0.45 → 2.41/6.46/2.16.

Стало (те же полосы): 43:60 −47.5/−49.3/−50.7/−56.8 против банка
−47.2/−50.2/−50.6/−55.1; 43:65 −45.2/−44.7/−47.6/−53.7 против −45.9/−48.9/−51.4/−53.3;
43:72 −43.9/−46.1/−48.8/−55.1 против −40.0/−48.0/−52.1/−53.1. То есть ±2 дБ.

### 2. Тембр: у нас был «перевёрнутый» спектр гармоник

Новая команда CLI `spec` (уровни гармоник, дБ отн. h1, пик ищется в окне
±60 центов вокруг k·f0 — у банка строй плавает). Замер 43:60, наш → банк:

| h | 2 | 3 | 4 | 5 | 6 | 7 | 9 | 11 | 12 |
|---|---|---|---|---|---|---|---|---|---|
| наш | −4.2 | −4.6 | −16.8 | −7.8 | −6.2 | −28.0 | −32.3 | −46.6 | −39.3 |
| банк | −1.6 | −3.4 | −11.2 | −14.7 | −15.7 | −17.9 | −19.1 | −27.3 | −24.2 |

То есть у нас h5/h6 были на 7-9.5 дБ ГРОМЧЕ h4, а выше h6 почти пусто, тогда как
у банка ровный спад. Хуже: у банка ФОРМА спектра почти одна и та же во всех
зонах (разброс <2 дБ между 60/64/68/72), а наши четыре таблицы (C4/E4/B4/D5)
были перетюнены каждая отдельно — в B4/D5 выше h6 не было почти ничего
(h9 −46.6 дБ против банковских −19.5).

Таблицы пересчитаны по замеру (new = old × 10^(Δ/20), Δ = банк − наш) и продлены
до 16 гармоник (h13-h16 у банка −33…−37 дБ, у нас спектр обрывался на h12;
доведены в два прохода, т.к. высокие гармоники садятся на 4-9 дБ).
Итог: h1-h16 совпадают с банком в пределах ±1 дБ во всех четырёх зонах.

Поправка на будущее (в README библиотеки): `BuildWaveTable` нормирует таблицу на
СУММУ амплитуд, а громкость даёт RMS. Первая компенсация по сумме дала +3.6 дБ на
43:72 (√(Σa²) вырос в 1.52 раза) — пересчитано по RMS, сустейн-уровень вернулся
в ±0.5 дБ от прежнего.

### 3. Глубина тремоло

Tremolo 0.18 → 0.22 (владелец: «не хватает глубины, как минимум на C4»). Замер
широкой полосы 300-4000 Гц, блоки 0.5 с, C4: было 0.75-1.40 дБ rms, стало
1.22-1.68 при банковских 1.12-1.71 и частоте 4.71 Гц (банк 2.69-4.71).

### Проверка

- `smoke-test-wasm.mjs` — PASSED (0 non-finite);
- `click-lr-any` 43:60/72, 115:60/72, 75:60 — 0 кликов (max|2nd|/rms вырос у флейт
  с 0.09 до 0.25-0.41 — это тремоло, не разрыв: медиана первых разностей не
  превышена ни разу);
- `web/generated/` = `dist/` (md5 совпадают);
- откат: `.scratch/known-good/20260913-FluteSpec-v69e/` (текущая),
  `20260913-FluteBreath-v69/` (только дыхание+глубина),
  `20260913-WindTremolo-v68/` (до апдейта);
- `.scratch/wasm-cmp/` — старые сборки с IntraSynth.js для сравнения уровней.

**WASM (канон: plain -Os, INTRA_PIANO_ALL_TABLES=ON) = 212 684 байт**,
md5 46b2c0b89595b186e13f03b0e3919a2f, IntraSynth.js 14 367 B (+11 Б к v68).

### Честно открыто

- Слушать не могу. Тремоло флейты теперь по замеру БАНКОВСКОЕ по глубине
  (1.2-1.7 дБ rms) и по частоте C4 (4.71 против 2.69-4.71). Если и этого мало —
  следующий шаг не глубина, а ФОРМА: у банка на C4 две когерентные составляющие
  (2.69 и 11.4 Гц), у нас одна чистая 4.71 — то есть банк звучит нерегулярно.
  Это уже правка движка (второй осциллятор модуляции), без согласия не делаю.
- Зоны флейты выше C5 (F5/G5/C6/F6) пересчитать НЕ по чему: у банка для программы
  43 нет семплов выше ~D5 (рендер пустой). Там таблицы оставлены прежними и по
  построению отличаются от четырёх исправленных зон.
- Верхние полосы дыхания (4.8-9.6 и 9.6-16 кГц) у нас всё ещё на 6-8 дБ громче
  банка: наш ФНЧ дыхания (2 каскада, 2200 Гц) падает медленнее банковского.
- Шум шва петли (Update 68) по просьбе владельца не воспроизводится.
---

## Update 70 — форма модуляции: почему оригинал читается «чаще» при той же частоте

Запрос владельца: «Не слышу тремоло на Pan Flute C4 или оно слишком редкое. У C6
вроде тоже редкое, оно в оригинале совсем частое» + вопрос: «При ресемплинге для
более высоких нот тремоло по идее пропорционально чаще должно быть. У нас есть
такой эффект?»

### 0. Ответ на вопрос про ресемплинг (замером)

- В банке вибрато **запечено в семпл**, поэтому внутри зоны оно транспонируется
  вместе с семплом: отношение «частота вибрато / f0» постоянно, а на границе зон
  меняется ступенькой. Замер Update 67: 0.0153 (G3-G4), 0.0106 (C5-F5), 0.0082
  (G5-C6), а абсолютные значения 3.00 → 4.04 → 5.38 идут как 2^(5/12) = 1.335 и
  1.332. Это и есть подпись ресемплинга.
- **У нас такого эффекта НЕТ.** Вибрато — отдельный LFO на каждую ноту, его
  частота берётся из кривой (Update 67), а не из семпла; в вейвтаблицу ничего не
  запечено. Пропорциональное «чаще» само не получится — кривую надо задавать
  понотно. Эффект появился бы только у периодичности, запечённой в таблицу
  (луп/шов — тогда она масштабируется как f0); движок в таблицу модуляцию не
  печёт.

### 1. «Слишком редкое» — это ФОРМА модулятора, а не его частота

Замер `mod` (полоса 2-60 Гц, окно 0.9-4.2 с от note-on) по шести клавишам: все
высокие линии банка — ЦЕЛЫЕ кратные основной (в скобках номер гармоники и
отношение амплитуд к основной):

| клавиша | банк: основная + гармоники |
|---|---|
| G3 55 | 3.03 + 9.08 (3×0.82) + 15.14 (5×0.79) |
| C4 60 | 4.04 + 12.11 (3×1.06) + 20.19 (5×0.89) |
| F4 65 | 5.38 + 16.15 (3×1.10) + 26.92 (5×0.95) |
| C5 72 | 5.55 + 11.02 (2×0.41) + 22.04 (4×0.79) |
| C6 84 | 8.50 + 17.08 (2×0.52) |

Два РАЗНЫХ целых кратных (3.000 и 5.000 с точностью 0.1 %) — значит это форма
вибрато, а не шов петли: шов дал бы одну частоту, пропорциональную транспозиции.
**Догадка Update 68 про «шум шва петли» неверна**: линия 12.11 Гц — это 3-я
гармоника 4.04 Гц, а «9.0 Гц при 3.0» на G3 — 3-я гармоника 3.00 Гц (я принял её
за независимую частоту и обратным счётом «нашёл» петлю в 4900 семплов).
Исправлено и в README библиотеки, и в комментарии движка.

Поэтому основной частоте оставлены замеренные значения, а добавлены гармоники
ФОРМЫ. Голосов у LFO по-прежнему два (тон и общая шина), но у осциллятора
модуляции теперь 2..5-я гармоники на ТОЙ ЖЕ фазе.

### 2. Движок: гармоники формы в `WobbleOscillator`

- `WobbleOscillator` получил `Carrier2..Carrier5` + `Harmonic2..Harmonic5`
  (Update 70 продолжил правку, начатую этим же апдейтом для 2-й и 3-й). Форма
  делится на (1+Σh) и остаётся в ±1; при всех `Harmonic_* == 0` ветви не
  исполняются, `Norm` равен ровно 1.
- Инструмент, задавший гармоники, домножает `Value`/`Tremolo` на
  `(1+Σh)/sqrt(1+Σh²)` — RMS-глубина не меняется, растёт пик.
- Гармоники задаёт ТОЛЬКО `PanFluteVibrato`; у остальных инструментов
  `Harmonic_* == 0`. Контроль (`bitcmp.mjs`, v69e → v70): фортепиано 0:60 и 21:60
  побитово идентичны; 74:72 и 43:60 отличаются в 5.5-8.3 тыс. сэмплов из 282 240
  на max|Δ| = 1.5e-8 (два ULP — переассоциация умножения на `Norm`), слышимого
  эффекта нет.

### 3. Инструмент: форма по зонам семплов

| зона (клавиши) | h2 | h3 | h4 | h5 |
|---|---|---|---|---|
| G3-G4 (узлы 5) | 0 | 1.00 | 0 | 0.85 |
| C5-F5 (узлы 2) | 0.40 | 0 | 0.75 | 0 |
| G5-C6 (узлы 2) | 0.52 | 0 | 0 | 0 |

Коэффициенты интерполируются `MixParam` по ТОЙ ЖЕ кривой узлов `vibRateX`
(G3/C4/F4/G4/C5/F5/G5/C6), что и частота, — то есть «прыгают» на границах зон
ровно как в банке. Частота вибрато C6 возвращена к замеру банка (8.10 → **8.50**
Гц): просьба «у C6 редкое, в оригинале частое» и замер здесь согласны (срез 8.58
→ 8.10 в Update 68 был сделан по двусмысленному «чуть чаще»).

### 4. Что стало (замер)

Частоты линий, наш v69e → v70 (банк — в таблице выше):

| клавиша | v69e | v70 |
|---|---|---|
| G3 55 | 4.29 (одна) | 4.29 + 11.65 (3×0.99) + 19.51 (5×0.83) |
| C4 60 | 4.88 (одна) | 4.88 + 14.97 (3×1.05) + 24.90 (5×0.76) |
| F4 65 | 5.55 (одна) | 5.55 + 16.65 (3×1.04) |
| C5 72 | 5.38 (одна) | 5.38 + 21.53 (4×0.91) |
| C6 84 | 8.07 (одна) | 8.50 + 17.00 (2×0.50) |

Спектральный центроид (эффективная скорость) вырос в 1.2-2.2 раза: C4 4.59 →
10.30 Гц, C5 6.06 → 9.88, C6 7.39 → 8.66. Центроид банка (17.1/17.3/14.3) выше,
но по нему сравнивать НЕЛЬЗЯ: в него входит огибающая выпрямленного шума (у нас
в полосе 2-4 Гц горб 0.33-0.43 дБ против банковского пола 0.003-0.04, зато в
20-60 Гц наоборот), а наше дыхание пан-флейты громче банка (см. `bands`).
Мерить надо структуру линий: она теперь совпадает.

### Проверка

- `smoke-test-wasm.mjs` — PASSED (0 non-finite);
- `click-lr-any` 75:55/60/72/84 — 0 кликов;
- `web/generated/` = `dist/` (md5 `683bb437…`);
- откат — `.scratch/known-good/20260913-PanFluteModShape-v70/` (три файла);
  сборки для сравнения — `.scratch/wasm-cmp/v69e`, `…/v70`;
- библиотека: новый флаг `mod --from`, функция `modCentroid` (центроид + глубина
  в полосе) и две «грабли» в README (гармоники ≠ шов петли; центроид
  смешивается с огибающей шума).

**WASM (канон: plain -Os, INTRA_PIANO_ALL_TABLES=ON) = 214 023 байта**,
md5 `683bb4375cae9bc8c4b594319d3e882c`, IntraSynth.js 14 367 B (+1 339 Б к v69:
четыре гармонических осциллятора, таблицы коэффициентов и `Math::Sqrt`).

### Честно открыто

- Слушать не могу. Это первый апдейт, который меняет САМ характер тремоло
  (импульсная форма), а не его глубину/частоту: если теперь слышится
  «дребезжание», откат — обнулить `h3`/`h5` (одна строка) или уменьшить
  `Jitter`/`Tremolo`.
- Глубина модуляции тона остаётся в 3-4 раза больше банковской
  (1.6-1.8 дБ против 0.38-0.52 в полосе 2-60 Гц) — сделано по прежним просьбам
  «не слышу тремоло»; часть этой цифры — наша шумовая юбка.
- Гармоники заданы только по трём зонам, где есть семплы банка; клавиши внутри
  переходных зон (G#4-B4, G#5-B5) интерполируются, а не замерены.
---

## Update 71 — откат пересчёта таблиц флейты 43 и срез дыхания пан-флейты

Запрос владельца: «У pan flute разницы не заметил, не слышно тремоло на C4, у flute
какой-то тембр стал непонятный, ни на что не похожий! И тремоло у него по глубине
норм, но чаще оригинала, но сейчас не это важно, важно, что сам тембр совсем не тот».

### 1. Флейта 43/115: что именно «стало» и что откатано

Сначала проверка, что это не Update 70: он флейту не трогает вообще —
`bitcmp` v69e → v70 даёт max|Δ| 1.5e-8 (два ULP), а `spec` совпадает поцифрово.

Update 69 изменил у программы 43 ровно две вещи: (1) таблицы гармоник четырёх
зон (C4/E4/B4/D5) и их `VolumeScale`, (2) уровень дыхания (×4.8…5.6). Владелец
просил именно дыхания («там не хватает дыхания»), а тембр назвал испорченным —
поэтому откатана только часть (1): таблицы возвращены к профилю из
`.scratch/IL-v69.bak`, дыхание (+14 дБ) оставлено.

Замер до/после (43:60, 1.4-4.0 с, дБ отн. h1):

| | h2 | h3 | h4 | h5 | h6 | h7 |
|---|---|---|---|---|---|---|
| v70 (по банку) | −1.6 | −3.4 | −11.2 | −14.7 | −15.7 | −17.9 |
| **v71 (откат)** | **−4.2** | **−4.6** | **−16.8** | **−7.8** | **−6.2** | **−28.0** |
| банк 43:60 | −1.6 | −3.4 | −11.2 | −14.7 | −15.7 | −17.9 |

### 2. Честно про конфликт замеров и слуха

Спектр v70 совпадал с банком в пределах ±1 дБ по h2…h16 (проверено ТРЕМЯ
независимыми способами: `spec` (БПФ 8192 с усреднением кадров), узкий пробник
пиков (±40 центов) и прямой ДПФ точно на k·f0 за 2.6 с) — и всё равно владелец
услышал «ни на что не похожий тембр». Причина, по которой магнитудного совпадения
может не хватать: наши гармоники в вейвтаблице идеально периодические и
фазово-жёсткие, у банка они размазаны шумом и дыханием. Что именно в этом
расхождении слышно, измерить имеющимися средствами не удалось (крест-фактор у нас
даже ниже банковского: 2.57 против 3.10), поэтому дальше нужен выбор владельца.

Для этого рядом с приложением выложены A/B-файлы (`dist/ab/ab-flute43-<key>-A/B/C.wav`,
`A` = текущая сборка, `B` = таблицы по банку (Update 69), `C` = банк) — их можно
сравнить в один клик.

### 3. Пан-флейта: дыхание срезано на 4 дБ (маскировка тремоло)

Тремоло тона на C4 в полосе 300-1500 Гц у нас по глубине совпадает с банком
(0.59…0.79 дБ rms против 0.78…0.86), то есть оно есть и громкость его «не
объясняет». Зато избыток был в ДЫХАНИИ — замер `bands` (75:60, 1.4-4.0 с,
медиана бинов, дБ отн. h1), наш воздух против банка:

| полоса | 400-600 | 600-1200 | 1200-2400 | 2400-4800 | 4800-9600 | 9600-16000 |
|---|---|---|---|---|---|---|
| было | **+7.9** | −0.3 | +1.4 | +4.1 | +6.0 | +1.0 |
| **стало** | **+4.6** | −3.5 | +1.7 | +1.0 | +2.8 | −4.1 |

В пяти полосах из шести мы были громче, сильнее всего ровно в 400-600 Гц — там,
где у C4 стоит его h2, то есть шумовая подушка сидела прямо на гармонике и
маскировала её модуляцию. Уровень срезан ровно ×0.63 (−4 дБ):
`lvlK` 1.90/1.60/1.25 → 1.20/1.01/0.79. Это закрывает старый открытый пункт
«воздух пан-флейты на 5-13 дБ громче банка».

### Проверка

- `smoke-test-wasm.mjs` — PASSED (0 non-finite);
- `click-lr-any` 43:60/72, 75:60 — 0 кликов;
- `web/generated/` = `dist/` (md5 `35b1160a…`);
- откат — `.scratch/IL-v70.bak` (состояние до этого апдейта), таблицы до Update 69 —
  `.scratch/IL-v69.bak`; сборка с банковскими таблицами — `.scratch/wasm-cmp/v70/`;
- A/B-WAV: `dist/ab/` (9 файлов, 43:60/67/72 × A/B/C).

**WASM (канон: plain -Os, INTRA_PIANO_ALL_TABLES=ON) = 214 012 байт**,
md5 `35b1160aa5659b958411ba2b6fd76532`, IntraSynth.js 14 367 B (−11 Б к v70).

### Честно открыто

- Какая из двух флейт ближе — решает владелец (A/B выше). Если «B» (по банку) —
  верну таблицы одной строкой, это `.scratch/IL-v70.bak`.
- Срез дыхания пан-флейты — правка на слух владельца «дыхание идеально»
  (Update 65): если теперь его мало, вернуть одной константой `lvlK`.
- Тремоло флейты 43 по-прежнему быстрее оригинала (5.0 против 4.04 Гц на C4,
  4.95 против 4.3 на низах) — поднято в Update 68 по прямой просьбе
  «на C4 не слышу тремоло вообще»; владелец подтвердил, что это не главное.
## Update 72 — «низкий пшик» в атаке флейт 43/115 и A/B-рендеры в веб-интерфейсе

### Что просил владелец

> «У обычной флейты 43/115 появился низкий пшик в атаке, неприятный, убери. Он
> хуже, чем в pan flute, а тут ещё и не уместен. Там совсем другое дыхание
> должно быть.» и «Не могу я твои wav слушать, если их нет в веб интерфейсе.
> И делай все wav preloaded… Настрой кеширование.»

### 1. Пшик найден разложением, а не на слух

Догадки по полосным замерам микса ничего не давали (в 100-400 Гц мы даже ТИШЕ
банка), поэтому слой дыхания выделен **вычитанием двух сборок**: рендер текущей
минус рендер той же сборки с обнулённым `level` у шумовых слоёв
(`.scratch/patch-nonoise.cjs`, сборка `.scratch/wasm-cmp/nonoise/`,
зонд `.scratch/decomp.mjs`). Вычитание точное: и шум, и тело детерминированы по
seed/фазе. Так стало видно то, что тонуло в теле:

| 43:60, окно | 100-200 | 400-800 | 800-1600 | 1600-3200 | 3200-6400 |
|---|---|---|---|---|---|
| **было (v71)** t20-t40 | +14.5…+8.2 | +11.1…**+18.4** | +12.0…+14.8 | +12.4…+16.2 | +11.0…+15.6 |
| **стало (v72)** | −6.3…−7.5 | −9.7…−3.3 | −8.8…−1.3 | −8.5…+0.2 | −9.8…−0.4 |

(дБ отн. СУСТЕЙНА самого слоя дыхания.) То есть дыхание флейты в первые 20-40 мс
стояло на **+15…+18 дБ выше собственного сустейна** — это и есть «пшик».
У банка шум вспыхивает ПОЗЖЕ и слабее: +4…+8 дБ на 100-200 мс
(`.scratch/flute-puff.mjs`, 43:60). Причина вспышки — `breathEnv`:
атака 25 мс до 1.0, затем экспоненциальный спад до полки 0.20 за 280 мс, то есть
пик в 5 раз (14 дБ) выше подложки; у пан-флейты то же самое, но её полка 0.30,
а спад 680 мс, поэтому её «чифф» читается дыханием, а не пшиком.

### 2. Правка — ровно 2 строки на инструмент

`breathEnv` 43 и 115: `{0.025, 0.28, 0.20, 0.07}` → `{0.10, 0.45, 0.55, 0.10}`
(атака 100 мс, спад 450 мс, полка 0.55), а уровень умножен на `0.20/0.55`,
чтобы **сустейн остался прежним** — владелец просил дыхание в сустейне и в
Update 71 уровень был выставлен по замеру банка.

Контроль: сустейн `bands` 43:60 400-600 Гц — **−47.0 дБ** (банк −47.1), 115:60 —
**−47.5**; было ровно то же. Атака в миксе (дБ отн. своего сустейна, 43:60):

| полоса | банк t30 / t40 | v71 t30 / t40 | **v72 t30 / t40** |
|---|---|---|---|
| 400-800 | −18.8 / −15.3 | −8.3 / −7.7 | **−13.6 / −10.4** |
| 800-1600 | −17.6 / −13.0 | −11.9 / −6.7 | **−14.7 / −10.6** |
| 1600-3200 | −16.6 / −9.3 | −7.1 / −2.5 | **−14.6 / −10.0** |
| 3200-6400 | −8.3 / −5.1 | +12.6 / +15.2 | **−6.2 / −0.6** |

То есть перехлёст атаки над банком (~+10 дБ) закрыт, а в 3.2-6.4 кГц мы были
громче банка на 21 дБ — теперь совпали.

### 3. A/B-рендеры теперь в веб-интерфейсе, с прелоадом и кешем

Раньше wav-ы писались в `dist/ab/` вручную и **исчезали при следующей сборке**
(`scripts/build-web.js` стирает `dist/`), поэтому владелец их просто не слышал.

- `intrasynth/tools/ab/render-ab.mjs` (новый, рядом с `tools/analysis`) рендерит
  панель в `web/generated/ab/` — штатное место генерируемых артефактов, откуда
  `build-web` копирует всё в `dist/ab/` (та же схема, что у `samples/`).
  Матрица: флейта 43 (C4, C5), 115 (C4), пан-флейта (C4, C5, C6) ×
  «наш · текущая / наш · предыдущая (снимок `.scratch/ab-builds/prev`) / банк
  Titanic»; длительность 2.6 с (атака + начало сустейна), 17 файлов, 3.72 МБ.
- **Кеш.** Имена файлов содержат хеш содержимого — `<slug>.<hash8>.wav`, поэтому
  URL меняется только у изменившегося рендера (у пан-флейты `nash` и `prev`
  вообще совпали по хешу — один файл, один запрос). `scripts/serve.js` теперь
  отдаёт `ab/*.wav` с `Cache-Control: public, max-age=31536000, immutable`, а
  всему остальному — `no-cache` **с ETag и 304** (раньше ETag не было вовсе, и
  `no-cache` качал файлы целиком каждый заход); для ассетов 404 вместо
  SPA-фолбэка на index.html.
- **Прелоад.** `web/synth.js` строит строки из `ab/manifest.json` и создаёт
  `<audio controls preload="auto">` сразу (плюс `audio.load()`), не дожидаясь
  раскрытия спойлера `#abSpoiler` в `web/index.html`.

Проверено настоящим браузером (`.scratch/ab-ui-check.mjs`, playwright): спойлер
**закрыт**, а все 17 плееров уже `readyState=4`; при перезагрузке — 17 запросов,
**0 полных загрузок** (17 × 304).

### Проверка

- `smoke-test-wasm.mjs` — PASSED (0 non-finite);
- `click-lr-any` 43:60/72, 115:60/72, 75:60, 74:84 — 0 кликов;
- `web/generated/` = `dist/` (md5 `45b52143…`), 17 файлов панели на месте;
- откат — `.scratch/IL-v71.bak`; прежняя сборка — `.scratch/wasm-cmp/v71/`,
  она же лежит как «предыдущая» в `.scratch/ab-builds/prev/`.

**WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 214 012 байт**,
md5 `45b52143a7d982aecba8e52c6110e24a`, IntraSynth.js 14 367 B (размер тот же:
менялись только константы).

### Честно открыто

- Слушать не могу: правка сделана строго по замеру (перехлёст атаки над банком и
  вспышка над собственной полкой). Если «пшика» стало мало/много — это одна
  константа: множитель `.20f/.55f` задаёт ПИК относительно полки, то есть
  ровно силу атаки.
- Полка 0.55 при списке 0.20 значит, что спад дыхания стал короче по амплитуде
  (всего 5.2 дБ против 14) — если в сустейне теперь не хватает «дыхания после
  ноты», это `0.45f` (спад) и полка, а не уровень.
- «Prev» в панели — снимок v71; перед следующей правкой его надо обновить из
  `web/generated/` (написано в шапке `render-ab.mjs`), иначе сравнение
  «до/после» пропадёт.
- Кеш-заголовки действуют на превью-сервере (`scripts/serve.js`); прод-хостинг
  заголовки не задаёт, но там работает другое: имя файла меняется только при
  смене содержимого, а остальное ревалидируется по ETag самого хостинга.
## Update 73 — выдох в атаке флейты, A/B по громкости и «грязный» воздух пан-флейты

### Что просил владелец

> «Сейчас у флейты тремоло чуть чаще, чем надо и очень глубокое. Атака всё равно
> не похожа, нет в ней выдоха, а сразу общий тембр, как в сустейне. В A/B рендерах
> у банка Титаник почему-то очень тихо, хотя в семплах всегда банки были гораздо
> громче нашего синтезатора. pan flute C4 у нас в принципе похож, но переход из
> атаки в сустейн у оригинала даёт более высокий звук, а у нас гудение. Сам сустейн
> похож, правда наш вроде чаще осциллирует. Атака C5 у Титаника красивее, чем у
> нас. Тембр у нас какой-то грязноватый, как будто алиасит что-то.»

### 1. Инструменты замера: `timbre` и `pitch`

Обе основные жалобы («нет выдоха в атаке» и «переход в сустейн звучит иначе») —
про ПЕРВЫЕ 200 мс, а всё, что было в CLI, мерило сустейн (`spec`/`bands` — окно
1.4-4.0 с). Добавлено в `lib/harmonics.mjs`:

- `timbreTrack` + команда `timbre` — окна по 60 мс: уровни партиалов (дБ отн. h1
  СУСТЕЙНА) и **межгармонический пол** (медиана бинов между k·f0 — это и есть
  «выдох»); опорный уровень снят тем же окном, иначе короткое и усреднённое окна
  несравнимы по масштабу;
- `pitchTrack` + команда `pitch` — высота МНК по нескольким гармоникам
  (f(k) = k·f0), потому что у пан-флейты в атаке h1 приглушён и оценка по нему
  врёт; пики интерполируются параболой (без неё шаг сетки на окне 60 мс даёт
  ±30 центов — больше любого вибрато).

### 2. Флейты 43/115: выдох В НАЧАЛЕ ноты

Замер `timbre 43:60` (окно 60 мс, дБ отн. h1 сустейна, **банк → наш v72**):

| полоса | 0-60 мс | 60-120 | 160-220 | 440 |
|---|---|---|---|---|
| 600-1500 | **−37.0 → −54.4** | −35.9…−37.1 → −42.6 | −42.0…−43.7 → −44.5 | −51.1 → −47.2 |
| 1500-3500 | −43.7 → −54.4 | −41.0 → −47.5 | −46.6 → −50.4 | −52.3 → −52.9 |

То есть у банка выдох стоит на ноте (−34…−36 в 0-100 мс) и гаснет за ~0.2 с
(динамика 17 дБ: от −33.8 в 40 мс до −51 в 440), а у нас его в начале не было
вовсе, зато был поздний пик на 120 мс и динамика всего 5 дБ. Причина — Update 72:
атака 100 мс / спад 450 мс / полка 0.55 убрали «низкий пшик», но вместе с ним и
выдох (медленный подъём читается как «сразу тембр сустейна»). Эти же цифры
показывают, что «пшик» лечился не медленным входом, а СПЕКТРОМ: у банка вспышка
живёт в 600-1500 Гц, у нас низ уходил в 100-800.

Новый `breathEnv` = `{0.008, 0.20, 0.16, 0.10}`, уровень ×0.20/0.16 (сустейн не
сдвинут, проверено: `bands` 43:60 400-600 = −47.2 против банковских −47.1).
Замер после правки (тот же `timbre`): 600-1500 — −34.5 (0-60), −38.6 (60),
−40.6 (100), −43.2 (140), −47.2 (160); банк — −33.8…−35.9, −35.9, −39.8, −39.9,
−42.0. Пик и первая половина спада совпали, хвост (после 160 мс) у нас на 3-5 дБ
тише (одной экспонентой плато+долгий хвост банка не описываются — это записано в
открытые пункты).

### 3. Флейты 43/115: тремоло медленнее и мягче

`trem --wide 300,4000 --block 0.5` (43:60):

| | 2.00 с | 2.25 | 2.50 | когерентная частота |
|---|---|---|---|---|
| банк | 1.34 дБ | 1.30 | 1.19 | гуляет 2.02…7.40 Гц (среднее ≈3.7) |
| наш v72 | 1.41 | 1.60 | 1.68 | жёсткие 4.71 в каждом блоке |
| наш v73 | — | 1.08 | 1.12 | 4.71 (см. ниже) |

Правки: `Frequency` 4.95 → **4.60** (+0.25·x как было), `Tremolo` 0.22 → **0.16**.
Честно: частота `trem --wide` разрешается только до 2 Гц, поэтому шаг 7 % этим
замером не доказать — он сделан по слуху владельца, а глубина срезана до 1.1 дБ
rms и потому оказалась У НИЖНЕЙ границы банковских 1.12…1.71 (было 1.6-1.68, у
верхней). Если станет «не слышно» — это ровно одна константа `r.Tremolo`.

### 4. Пан-флейта: «грязноватый, как будто алиасит» — это гребёнка дыхания

Ключевой замер — `spec` на сборке с обнулённым дыханием
(`.scratch/wasm-cmp/nonoise`): **тон сам по себе совпадает с банком в ±1-4 дБ**
(75:60: h2 −28.5/−29.5, h4 −43.9/−42.2, h6 −47.0/−46.8, h8 −59.9/−59.1), а с
дыханием чётные гармоники уезжают вверх:

| 75:60 | h2 | h4 | h6 | h8 |
|---|---|---|---|---|
| v72 (с дыханием) | −27.1 | −36.8 | −38.8 | −44.2 |
| **v73** | −28.2 | **−42.2** | **−46.8** | −54.7 |
| банк | −29.5 | −42.2 | −46.8 | −59.1 |

(на 75:72 было h4 +4.7, h6 +8.2, h8 +14.8 дБ к банку, стало −1.2/−2.3/−0.6.)
Механизм: у пан-флейты собственные h4/h6/h8 лежат на −42/−47/−60 дБ, а
`combGain 0.85` усиливает шум на КАЖДОЙ k·f0 на 16.5 дБ — узкие шумовые линии
ложатся ровно на слабые гармоники и читаются как посторонние партиалы
(«алиасинг»). Контраст гребёнки снижен 0.85 → **0.60**, ФНЧ воздуха опущен
2000-2600 → **1600-2200 Гц** (у банка юбки падают от h4 к h8 на 17 дБ, у нас
падали на 7), уровень оставлен. Пол по `bands` (75:60, 1.4-4.0 с): было
600-1200 −48.0 при банковских −48.1, стало −52.0 — то есть пол ПРОСЕЛ на 4 дБ
(записано в открытые пункты), зато юбки на партиалах совпали в ±1 дБ.

### 5. Пан-флейта: «наш вроде чаще осциллирует»

Амплитуды гармоник формы модулятора срезаны ~15 %: h3 1.00 → 0.85, h5 0.85 →
0.70, h2 0.40 → 0.34, h4 0.75 → 0.62 (низ/середина/верх — по зонам). Основная
частота (кривая `vibRateV`, замер банка) не тронута. Замер `trem --wide 300,4000`
до/после на 75:60: rms 0.72/0.69/0.64/0.74/0.79 → 0.72/0.68/0.60/0.65/0.73,
когерентная линия была размазана по 4.04…5.38 Гц, стала ровная 4.71 —
то есть глубина и основная частота те же, изменилась только форма.

### 6. A/B-панель: банк больше не «очень тихий»

`renderBank` отдаёт семпл как есть, а наш выход громче — в панели банк был на
6-10 дБ тише (замер: f43-60 банк peak −26.6 / rms −36.3 против наших −16.7 /
−25.8). Добавлен `normalizeSustainRms` (`lib/render.mjs`): RMS сустейна (0.6-2.4 с)
приводится к −20 dBFS, пик страхуется потолком 0.97. `render-ab.mjs` теперь
нормирует ОБА рендера, в `manifest.json` добавлена строка «сравнивайте тембр и
атаку, а не громкость». После перегенерации все 17 файлов имеют ровно −20.0 dBFS
по сустейну и пики −9…−14 dBFS. Имена по-прежнему с хешем содержимого, так что
браузер скачает новые версии (старые URL больше не запрашиваются).

### Проверка

- `smoke-test-wasm.mjs` — **PASSED** (0 non-finite);
- `click-lr-any` 43:60/43:72/115:72/75:60/75:72/75:84 — **0 кликов**;
- сустейн флейты не сдвинут: `bands` 43:60 400-600 −47.2 при банковских −47.1;
- `web/generated/` = `dist/` (md5 `27de8835e8baa7c29f1917fb4bbd0e3f`), `dist/ab/` обновлён;
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 214 014 байт**,
  md5 `27de8835e8baa7c29f1917fb4bbd0e3f`, IntraSynth.js 14 367 B (+2 Б к v72);
- откат — `.scratch/known-good/20260913-ExhaleAndPanComb-v73/`;
  A/B-«prev» по-прежнему v72 (сборка, которую владелец слышал);
- в README библиотеки добавлены обе команды, раздел «грабли Update 73» (гребёнка
  и чётные гармоники, разрешение `trem --wide`, выравнивание A/B, выдох в долгом
  окне) и примеры запуска.

### Честно открыто

- **Тембр флейты не тронут**: выдох и тремоло правлены по замеру, но если
  «примитивность» тембра осталась — это следующий шаг и он требует другого
  замера (сейчас тон флейты отдельно не сверялся, как у пан-флейты).
- Верх дыхания флейты по-прежнему выше банка: сустейн 4800-9600 +5.7 дБ,
  9600-16000 +13.8 дБ (старый открытый пункт, этим апдейтом не закрыт).
- Пол дыхания пан-флейты после снижения гребёнки просел на ~4 дБ в 600-2400 Гц
  (было ровно по банку). Я выбрал совпадение ЮБОК на партиалах, потому что жалоба
  была именно про «грязный» тембр; если дыхания стало мало — это уровень
  `0.045f*lvlK`, одна таблица.
- Патч-скрипт: `.scratch/patch-update73.cjs` (в файле 176 КБ правки через
  инструмент не проходят — он видит только первые ~50 КБ, поэтому правка идёт
  скриптом с проверкой числа вхождений).
- Настройка «на C4 оригинал выше» замером подтверждается лишь частично: банк
  центрирует +3…+4 цента от номинала на всех проверенных клавишах (43 и 75), у
  нас 0 — это настройка сэмплов банка. Менять по ней нашу высоту не стал.
### 7. Попытка поправить атаку C5 пан-флейты — откачена

Отдельным замером `timbre 75:72` видно, что у банка на C5 атаку ведёт НЕ
основной тон: h1 в окне 0-60 мс стоит на **−26.9** дБ, а h3 — на −23.6; только
к 120-180 мс h1 доходит до +4.1. У нас h1 вступал сразу (−13.0), h3 — на −30.7.
Сделаны две правки (обе откачены по замеру):

1. **Опора C5 в огибающей тела** (`xs`/`d0v`/`v1v`/`t1v`/`t2v`, d0 10 → 18 мс,
   v1 0.053 → 0.030). Результат: h1 в 0-60 мс сдвинулся всего на **0.8 дБ**
   (−13.0 → −13.8). Причина: в первые 60 мс h1 определяет не тело, а
   **слой h1-овершута** (`over`, BloomSampler), у которого подъём на C5 равен
   0.117 с.
2. **Опора C5 у слоя овершута** (подъём 0.117 → 0.22 с). Результат: ошибка
   перераспределилась, а не исчезла — сумма |Δ к банку| по окнам осталась той же
   (31.8 → 30.2 дБ): 0-60 мс стало +10.6 вместо +13.9, но 80-140 мс ушло в −5.4
   вместо +1.0, и главное — подъём 0.22 с сдвинул бы пик овершута с
   120-280 мс (замер банка) на ~250-400 мс, то есть сломал бы тот «бугорок»,
   который владелец просил сохранить в Update 66-72.

Вывод, который стоит записать: система «тело + splash + овершут + дыхание» не
воспроизводит банковскую форму «тихо 60 мс, затем быстрый раздув» сдвигом одной
константы — у банка на C5, похоже, ДРУГОЙ семпл (тот же эффект виден и в
`timbre 75:60`, где h1 в 0-60 мс −16.8 против −26.9 на C5). Это отдельная
правка с полным переподбором четырёх слоёв атаки, и её нельзя делать вслепую.
Обе правки откачены (`cp` из `.scratch/known-good/20260913-ExhaleAndPanComb-v73/`),
финальная сборка — md5 `27de8835e8baa7c29f1917fb4bbd0e3f`, скрипты правок
`.scratch/patch-update73b.cjs`, `.scratch/patch-update73c.cjs` оставлены как
запись попытки.

Аналогично в A/B-панели **«prev» остаётся v72** — та сборка, которую владелец уже
слышал. Перед следующей правкой в `.scratch/ab-builds/prev/` надо положить v73
(иначе сравнение «до/после» в панели снова будет с v72).
## Update 74 — НАЙДЕНА КОРНЕВАЯ ОШИБКА: эталоном для флейты был пресет 43 (Contra Bass), а не 73 (Flute); пшик дыхания, частота тремоло, прелоад вкладок

Запрос владельца: «У флейты противный пшик! И почему при переключении вкладок
сырых семплов они оказываются незагруженными?! Я не понял, что за бред в A/B
Titanic? Там вообще звук посторонний. Тюнить надо под то, что в самом первом
спойлере при выборе Flute, а второй спойлер A/B вообще по-другому звучит. То ли
не тот инструмент, то ли ноту напутали, то ли что. У нас сейчас тремоло чуть
чаще, чем у банка.»

### 1. Почему A/B «звучал посторонним» — и это была не только панель

Наш синтезатор держит «FluteClean» и «FluteHybrid» на **свободных** слотах 43 и
115 (43 в GM — Contra Bass, 115 — Woodblock). А `renderBank`/`renderPair`
рендерили банк **той же программой**, что и синтезатор. То есть все замеры
«эталона» для флейты с Updates 67-73 (и пересчёт таблиц в Update 69) шли **по
контрабасу**, и владелец это услышал в панели A/B.

Подтверждение (`.scratch/refcheck.mjs`, `.scratch/bankprog.mjs`) — уровни
гармоник, дБ отн. h1:

| C4 | h2 | h3 | h4 | h5 | h6 | h7 | h12 |
|---|---|---|---|---|---|---|---|
| `samples/Flute/C4.wav` (вкладка-эталон, пресет 73) | −5.2 | −6.5 | −18.4 | −14.8 | −7.2 | −22.0 | −41.8 |
| банк пресет 73 | −5.2 | −6.4 | −19.1 | −13.6 | −6.8 | −23.0 | −43.3 |
| банк пресет 43 (то, что играло в A/B) | −1.7 | −5.1 | −10.1 | −13.0 | −16.8 | −17.8 | −23.3 |

Правка: `BANK_PROGRAM` / `bankProgramFor()` в `tools/analysis/lib/render.mjs`
(43→73, 115→73, 77→78) — **единственное место**, откуда сопоставление берут зонды,
CLI и панель. CLI печатает подмену: `(банк: программа синтезатора 43 → пресет
банка 73)`. В панели подпись банка теперь содержит пресет: «банк Titanic · пресет
73 «Flute»», и номер пресета входит в **имя файла** (`f43-60-bank-p73.<hash>.wav`),
иначе готовый файл считался валидным по mtime SF2 и в панели оставался старый.

### 2. Пшик дыхания флейт 43/115

Замер `timbre 43:60` (окно 60 мс, дБ отн. h1 сустейна) по **верному** эталону:

| 600-1500 Гц | 0 мс | 20 | 40 | 60 | 80 | 100 | полка |
|---|---|---|---|---|---|---|---|
| банк 73 | −72.0 | −68.1 | −62.0 | −57.4 | −54.0 | −51.5 | −51…−54 |
| наш v73 (пшик) | **−34.5** | −36.2 | −35.8 | −38.6 | −41.8 | −40.6 | −47.5 |
| наш v74 | −59.7 | −54.8 | −51.1 | −49.5 | −48.2 | −45.6 | −46 |

У банка шум в атаке **нарастает** и вспышки на note-on нет вообще; у нас он стоял
на −34.5 дБ уже в первые 20 мс (в 25 дБ над своей полкой). Причина — формула
Update 73: подъём 8 мс и полка 0.16, то есть пик/полка 6.25× = +16 дБ.

Правка (2 строки на инструмент, 43 и 115): `breathEnv {0.008, 0.20, 0.16}` →
`{0.15, 0.40, 0.72}` (пик/полка 1.39× = +2.8 дБ), уровень ×0.20/0.72 — полка
сустейна не сдвинута. Проверка шумом по полосам (`flute-puff.mjs`, дБ отн.
своего сустейна): в 3.2-6.4 кГц было +3.0/+3.0/+4.9 на 40/60/100 мс (у банка
−26…−7), стало: пик ниже полки, 100-200 Гц растёт монотонно (−74.5 @0 → −7.4 @60).

### 3. Тремоло флейт: чуть реже

`trem --wide 300,4000` (блоки 0.5 с), **верный** эталон: банк 3.36-4.04 Гц и на
C4, и на C5; у нас было жёстких 4.71. `r.Frequency` 4.60 → **4.15** (+0.25·x):
стало 4.04 в каждом блоке. Глубину не трогал (владелец просил не про неё), но
честно: у банка она 1.73-1.91 дБ rms на C4 и 2.91-3.28 на C5, у нас 0.96-1.39.

### 4. Тембр флейты перетюнен по верному эталону (8 зон, две итерации)

`.scratch/zone-tune.mjs`: по репрезентативным клавишам каждой зоны считает
Δ = банк − наш по h2…h12 и перемножает таблицу на `10^(Δ/20)` (зажим ±5 дБ,
мельче 1.5 дБ — не трогаем; зажим значений `CAP = 65535`, потому что в зоне E4
h2 = 38001 > 32768). Две итерации (вторая добила E4, где первая упёрлась в зажим),
итог — отклонение от пресета 73 по h2…h12:

| зона (клавиши) | было (худшее) | стало |
|---|---|---|
| C4 (60-62) | h5 +7.0, h11 −10.3 | ≤ ±2 (h9 −3.1) |
| E4 (63-65) | h4 −10.1, h6 −12.7, h10 −12.0 | ≤ ±3 (h6 −3.2) |
| B4 (66-71) | h2 −5.2, h5 −8.3, h8 −7.1 | ≤ ±4 (h11 −4.0 при −44 дБ) |
| C5 (72-75) | ±2.9 | ≤ ±1 |
| F5 (76-78) | h4 +5.3, h12 +11.7 | ≤ ±2 |
| G5 (79-81) | h6 +4.0 | ≤ ±2 |
| C6 (82-84) | h7 −6.3, h12 −6.8 | ≤ ±3 |
| F6 (85+) | h7 −6.5, h8 −7.4 | ≤ ±1.7 |

Громкость при этом не сдвинулась (RMS сустейна 43:60 0.0527 → 0.0560, 43:72
0.0795 → 0.0790; пан-флейта и Recorder побитово те же). Откат — `.scratch/IL-v73.bak`
(всё) и `.scratch/IL-v74-pretune.bak` (до пересчёта таблиц).

### 5. Вкладки сырых семплов: почему были «незагруженными»

`renderSampleRows` создаёт `<audio preload="none">` с сетевым `src` заново при
каждом заходе на вкладку, а `preloadSamplesForTab` находит файл в кеше
(`sampleBlobs`) и **пропускает** его (`if (...) continue;`) — подмена на blob для
новых элементов не происходила никогда, и повторный заход показывал пустые
плееры. Теперь при отрисовке берётся готовый blob (и `preload="auto"`).

Плюс по просьбе владельца («делай все wav preloaded… даже если спойлер не
открывать») после первой вкладки **все остальные докачиваются в фоне** по одной
вкладке за раз. Проверено браузером (`.scratch/ui-check74.mjs`): 12/12 вкладок
скачаны, возврат на первую — `blob = 9/9, ready = 9/9`; A/B: спойлер закрыт,
**21/21** плееров готовы, подписи содержат пресет; ошибок страницы нет.

### Проверка

- `smoke-test-wasm.mjs` — **PASSED** (non-finite 0);
- `click-lr-any` 43:60/72/84, 115:60, 75:60, 74:60 — **0 кликов**;
- `web/generated/` = `dist/` (сборка `node scripts/build-web.js`), A/B-панель 21 файл / 4.59 МБ;
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 214 014 байт**, IntraSynth.js 14 367 B;
- откаты: `.scratch/known-good/20260913-ExhaleAndPanComb-v73/` (то, что владелец
  слышал как «пшик»), `.scratch/IL-v73.bak`, `.scratch/IL-v74-pretune.bak`,
  сборки в `.scratch/wasm-cmp/`;
- в библиотеку анализа добавлены «Грабли Update 74» (программа синтезатора ≠
  пресет банка) и зонд-проверка `.scratch/bankprog.mjs`; CLI печатает подмену.

### Честно открыто

- Слушать не могу: и пшик, и таблицы правились строго по замеру верного эталона.
  Если тембр стал «не тот» — откат одной командой из `.scratch/IL-v74-pretune.bak`.
- Глубина тремоло флейты по-прежнему меньше банковской (0.96-1.39 против
  1.73-1.91 дБ rms на C4, 2.91-3.28 на C5) — это следствие прямой просьбы
  Update 73 «очень глубокое»; поднять — одна константа `r.Tremolo`.
- Тембр флейты ВЫШЕ C6 (85+) и зона B4 сверялись по h2…h12, но у эталона там
  уже −40…−90 дБ, то есть это шум замера.
- «Выдох» теперь нарастает за 150 мс, как у банка; если он читается как «нет
  атаки», крутить надо `0.15f` (подъём) и полку, а не уровень.
- Пан-флейта и Recorder в этом апдейте не менялись вообще (в панели A/B их
  «текущая» и «предыдущая» — один и тот же файл).
## Update 75 — тембр флейты приведён к ФАЙЛУ вкладки «Flute»: таблица была обрезана на 12 гармониках, а дыхание стояло на 12 дБ выше

Запрос владельца: «Сам тембр флейты не похож на то, что я слушаю в спойлере сырых
семплов». На этот раз эталон взят не «рендером банка», а **самим файлом**, который
владелец слушает: `web/generated/samples/Flute/{C4,C5,C6}.wav`
(`.scratch/vs-tab.mjs`, `.scratch/noise-shape.mjs` — третьоктавный профиль и
шумовой пол).

### 1. Таблица была ДЛИНОЙ 12, а у банка есть h13-h16

`spec --kmax 16`, дБ отн. h1, C4 (клавиша 60):

| | h12 | h13 | h14 | h15 | h16 |
|---|---|---|---|---|---|
| было (таблица на 12 гармоник) | −42.4 | **−58.3** | **−59.8** | **−60.5** | **−61.7** |
| банк (пресет 73 / файл вкладки) | −41.4 | **−38.7** | **−37.8** | **−39.7** | **−43.0** |
| стало (таблица на 16) | −43.7 | −39.1 | −37.7 | −39.5 | −43.5 |

То есть мы теряли **20 дБ** на 3.4-4.2 кГц — это слышимая «искра» флейты, которой
у нас не было вовсе (в третьоктавах полоса 4000 Гц: было −41.1 против −26.9 у файла,
стало −28.0). Массивы `fluteClean*` расширены до 16 гармоник
(`.scratch/extend-flute16.mjs`: h13-h16 взяты из замера банка), затем итерационный
`.scratch/zone-tune.mjs` (уже с `kmax: 16`, две итерации) довёл все 16 партиалов:
на C4/C5/C6 — в пределах 1-3 дБ, зона E4 — в пределах ±3 дБ по h2..h16.
На C5 и C6 банк был выше нас на 9 и 14 дБ на h13 — тоже выровнено.

### 2. Межгармонический пол: наше дыхание было на 12 дБ громче файла

`floor 43:60` (окно 1000-1400 мс, дБ отн. h1), наш → банк:
**−54.4 → −66.5** (было бы −81.6 без дыхания). Причина та же, что в Update 74:
уровень дыхания поднимали в Update 69 по НЕВЕРНОМУ эталону (пресет 43 = Contra Bass,
сравнение «наш/банк ×4.8…5.6»). Изоляция сборкой с `level = 0` доказала, что весь
пол — это наш шумовой слой, а не грязь вейвтаблицы.

Снижено по замеру `bands` (400-4800 Гц): C4 −6 дБ, C5 −8 дБ, C6 −3 дБ
(`lvlK` 2.41/6.46/2.16 → 1.21/2.57/1.53). Стало:

| `bands`, дБ отн. пика h1 | 400-600 | 600-1200 | 1200-2400 | 2400-4800 |
|---|---|---|---|---|
| наш C4 → банк | −54.5 / −56.8 | −56.7 / −59.0 | −58.2 / −57.2 | −64.1 / −61.9 |
| наш C5 → банк | −50.1 / −51.5 | −52.6 / −53.9 | −55.4 / −53.5 | −61.7 / −61.0 |

### 3. Форма шума: воздух у банка тянется до ~6.8 кГц, у нас резался с 2.2 кГц

`noise-shape.mjs` (шумовой пол по третьоктавам, только межгармонические бины),
C4, наш → банк: 2138-2694 −60.8 / −56.8, 3394-4276 −64.5 / −61.3,
5388-6788 −71.6 / −63.7, 6788-8553 −74.9 / −69.1 — то есть выше 2 кГц мы падали
на 4-8 дБ быстрее. Ниже f0 тоже не хватало: 134-168 Гц −66.9 / −57.8.

Правка: ФНЧ воздуха `2200-350x` → **`3800-600x`** (C4 3800 / C5 3200 / C6 2600),
ФВЧ `0.90·f0` → **`0.55·f0`**. Стало (наш − банк): 134-168 −3.5, 424-535 +2.9,
849-1069 +5.2, 2138-2694 −0.1, 3394-4276 +3.1, 5388-6788 +1.2, 6788-8553 +4.5,
8553-10776 +3.2 — **в пределах ±5 дБ от 134 Гц до 10.8 кГц** (кроме 10.8-13.6 кГц,
где нас на 16 дБ больше, но там уже −70 дБ).

### 4. Итог: третьоктавный профиль против ФАЙЛА вкладки (C4)

| полоса Гц | 397 | 500 | 630 | 794 | 1260 | 1587 | 2520 | 3175 | 4000 | 5040 | 6350 | 8000 | 10079 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| наш − файл, дБ | +0.6 | −1.8 | −1.3 | −1.5 | +0.5 | −1.1 | −0.3 | −0.5 | −1.1 | −2.4 | −0.7 | +2.1 | +2.6 |

Остались две полосы с дефицитом: **1000 Гц −6.1** (h4) и **2000 Гц −9.3** (h8).
Причина измерена: у банка h4/h8 — широкие горбы (энергии в полосе на 8 дБ больше,
чем у линии с тем же пиком), у нас — линии (пики `spec` при этом совпадают).
Это следующая правка тембра; в движке для неё есть неиспользуемый помощник
`WithSkirt(set, bandwidthCents, skirtGain)`, но его формулу ширины надо проверить
замером (он один раз уже давал «линию» при малой ширине).

### Проверка

- `smoke-test-wasm.mjs` — **PASSED** (non-finite 0);
- `click-lr-any` 43:60/72, 115:60, 75:60 — **0 кликов**;
- `web/generated/` = `dist/` (md5 `97beced5…`);
- **WASM (канон: plain `-Os`, `INTRA_PIANO_ALL_TABLES=ON`) = 214 037 байт**, IntraSynth.js 14 367 B;
- A/B-панель: «предыдущая» = v74 (та сборка, где владелец слышал «не похож»),
  21 файл пересобран;
- откаты: `.scratch/IL-v74.bak` (до снижения дыхания), `.scratch/IL-v75a.bak`
  (до расширения таблиц до 16), `.scratch/known-good/20260913-FluteRefTune-v74/`.

### Честно открыто

- Слушать не могу: всё сделано по замеру против файла вкладки. Если тембр всё ещё
  «не тот» — откат двумя файлами (`.scratch/IL-v74.bak`, `.scratch/IL-v75a.bak`).
- Дыхание теперь ТИШЕ, чем было (на 6-8 дБ в C4/C5): это против прежних просьб
  «дыхание слышно», но именно так стоит файл-эталон. Если воздуха стало мало —
  одна константа `lvlK`.
- Горбы h4/h8 (1000 и 2000 Гц) у нас пока линии — дефицит 6 и 9 дБ.
- Расширение до 16 гармоник подняло таблицу: 8 зон × 4 значения, WASM +23 байта.
