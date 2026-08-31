---
title: "Piano loudness balance: StereoPan==0 fast-path fix + per-instrument VolumeDb calibration"
status: "active"
created: 2026-08-28
started: 2026-08-28
updated: 2026-08-28
risk_level: medium
related_files:
  - intrasynth/src/Intra/Synth/AdditiveSampler.cpp
  - intrasynth/src/Intra/Synth/AdditiveSampler.h
  - intrasynth/src/Intra/Synth/InstrumentLibrary.cpp
related_tasks:
  - 20260820-PianoAttackTimbre
  - 20260827-HammerGlowShortNoteRelease
---

# Task

User request: calibrate the relative loudness of the 8 additive pianos
("Калибруй, но сохрани точные значения где-то. А то я пока не понял, ок или
нет."). The audit (`20260828` probes in `.scratch/`) had shown the derivative
pianos (Bright/EG/HT/EP1/EP2/Harpsi/Clav) sit 2–15 dB below AcousticPiano with
no calibration reference, and the bass was systematically ~6 dB quieter than
Acoustic at C2 despite the same Scale ratio.

# Root cause (measured, not guessed)

The perceived level of a note depends on which render path it takes through
`NoteSampler::GenerateStereo`:

- **Fast path** (no ADSR envelope — only AcousticPiano): `AdditiveSampler::
  GenerateStereo`. For regions with `StereoPan == 0` (bass regions roots
  25/30/34/38/51) it wrote the **full signal** (×1.0) to both channels.
- **Slow path** (any instrument with an ADSR envelope — all 7 derivative
  pianos): mono render + `panLeft = panRight = 0.5` split → ×0.5 per channel.

The StereoPan feature normalised panned regions to `(L+R)/2 = 0.5` (matching
the mono path, per its own comment), but the `StereoPan == 0` branch was left
writing ×1.0. So at StereoPan==0 keys Acoustic was **exactly 6.02 dB louder**
than the same-note derivatives, and Acoustic itself had a 6 dB level cliff
between bass regions (×1.0) and the first panned regions (×0.62..0.64).

Empirical confirmation (wasm/native A/B, same params, key→ratio vs native =
0.5/gl, gl from `1/(1+10^(StereoPan/20))`):

| key | wasm/native (before fix) | 0.5/gl predicted |
|-----|--------------------------|------------------|
| 36  | 0.500                    | (StereoPan=0 → bug: was 1.0) |
| 48  | 1.010                    | 1.010            |
| 60  | 0.806                    | 0.805            |
| 72  | 1.186                    | 1.186            |
| 84  | 1.036                    | 1.036            |

All panned keys matched the 0.5/gl model exactly; only the StereoPan==0 bass
key deviated — confirming the ×1.0 branch as the bug.

# Changes

## 1. `AdditiveSampler::GenerateStereo` — StereoPan==0 now uses the same
normalised level (×0.5/×0.5 per channel) as every other region/path.

```cpp
const float gl = mStereoPan == 0.0f ? 0.5f : mStereoGainL;
const float gr = mStereoPan == 0.0f ? 0.5f : mStereoGainR;
```

Effect: AcousticPiano bass (roots 25–38, 51) drops exactly 6.02 dB — this is
the "high notes drowned by low ones" the user reported; note-to-note balance
now follows the sample-measured `region.Loudness` instead of the channel
shortcut. Mid/treble Acoustic unchanged (panned regions already at 0.5 norm).

## 2. Per-instrument `VolumeDb` (new 10th param of `AdditivePianoInstrument`)

Applied in the constructor as `mVolume = volume·10^(VolumeDb/20)` — scales the
WHOLE note (attack+sustain+buffers), independent of `Scale` (so the contact
attack normalisation is untouched) and of velocity dynamics (`impactV`, `velF`
still use the raw volume).

Values computed from the post-fix measurement (vel 100, keys 36/48/60/72/84,
target = −avg(peak 0–100 ms, rms 100–400 ms) vs Acoustic):

| instrument            | VolumeDb (dB) |
|-----------------------|---------------|
| AcousticPiano         | 0.0 (reference) |
| BrightAcousticPiano   | +0.7          |
| ElectricGrandPiano    | +6.8          |
| HonkyTonkPiano        | +1.8          |
| ElectricPiano1        | +6.2          |
| ElectricPiano2        | +5.7          |
| Harpsichord           | +7.5          |
| Clavinet              | +9.1          |

# Verification (post-fix + calibration, delta vs Acoustic in dB)

Peak 0–100 ms avg: Bright +0.3, EG +0.4, HT +1.5, EP1 −0.1, EP2 +0.1,
Harpsi +1.4, Clav +2.4. RMS 100–400 ms avg: −0.3, −0.3, −1.4, +0.1, −0.0,
−1.3, −2.3. Residual per-key spread ±2.5 dB (region StereoPan gl-vs-0.5
difference between fast and slow paths — accepted).

RMS 400–1000 ms (late body) still differs by design: EG +2.6, EP1 +3.2, EP2
+5.1 (longer sustain — their character), Harpsi −5.6, Clav −16.6 (fast decay —
inherent, not a gain issue). Same-velocity strike and early body now match
Acoustic.

## Follow-up (same day, after listening): D3-E3 false loudness notch

User noticed D3-E3 (keys 49-52) is quiet and "crooked" on the other pianos.
Diagnosed via `scripts/_tmp-loudmap.js` (per-key early-body loudness for all 8
additive pianos) + `scripts/_tmp-rawlevel.js` (raw SF2-derived samples):

- Region root 51 (E3) carried `Loudness = 0.2127`, ~4-6 dB below neighbours
  root 47 (0.3159) / 54 (0.3711), so all 8 pianos dipped ~5 dB at keys 49-52.
- The source is NOT quieter there: `D3_sample` rms120-300 = -11.8 dBFS vs
  C3 -14.5 / C4 -14.2 (D3 actually ~2.7 dB hotter). So the notch was a
  generator measurement artifact of that one region, not the instrument.
- Cheap fix (no table growth — the WASM is byte-identical in size): raise
  region 51 `Loudness` to 0.43 (adjacent root-47 reproduction × sampled D3/C3
  ratio = 0.3159·10^((-11.8+14.5)/20)). One float in the shared table fixes
  D3-E3 on all 8 instruments simultaneously.

Measured after fix (key 51, rel dB): Acoustic 2.7→8.6, Bright 0.6→6.5, EG
0.9→6.8, HT −0.1→5.8, EP1 1.1→7.0, EP2 0.2→6.1, Harpsi 2.2→8.2, Clav 3.4→9.3
— each now sits on the local 48→54 trend instead of in a dip. Other mid dips
(roots 57/63/69) left untouched — no extra sample evidence yet (per stance: no
fixing to taste, only with analysis).

# Revert

1. StereoPan fix: restore the `if(mStereoPan == 0.0f) { both += v }` branch in
   `AdditiveSampler::GenerateStereo`.
2. Calibration: set all 7 VolumeDb values to 0.0 in `InstrumentLibrary.cpp`
   (the constants are isolated in one place).
3. D3-E3 region-51 Loudness: restore 0.4300 → 0.2127 in `PianoRegions.h`
   (line carries the rationale comment).
4. Rebuild: `sh scripts/build-wasm.sh` (and `sh scripts/build-web.js` to
   refresh dist/ for the probes and player).

# Follow-up: attack-boom off + UI compaction (20260829)

User decided the two-phase attack-«бум» (AttackBoom) was not solving the
D3-E3 brightness/honky-tonk issues, only nudging loudness up, and prefers the
sound without it. Decision recorded publicly so it is not re-added blindly:

- `web/synth.js`: `renderParams.AttackBoom` default `1 → 0` (old value kept in
  a code comment for revert). The C++ feature stays dormant (param fed as 0) —
  no WASM rebuild needed. The «Бум атаки» debug toggle/row and its JS wiring
  were removed.
- Note: with boom off, perceived loudness drops a touch. If bright/honky-tonk
  D3-E3 still read quiet, that is a separate per-region issue (roots 51/57/63/69)
  to revisit only with sample-backed analysis.

## UI (web/index.html + web/synth.js)

- Note-test buttons now play the current selected instrument (Program Change)
  on the melodic channel instead of the hardcoded Acoustic Grand on ch.0.
- Note-test buttons moved directly above the drawn piano; the separate
  «Проверка отдельных нот» panel, its heading, and its status line were removed.
- MIDI enable control shrank to a 30px icon toggle, placed left of the device
  name / «MIDI-устройства недоступны» status.
- «All notes off» UI button removed (internal `allNotesOff()` kept for
  drums/stop keyboard cleanup).
- Hero («Сыграйте…»), footer (Intra/WASM credits), and the local-synthesis /
  WASM / Intra explanations removed; header sub trimmed to sample rate.
- Panels/header/main/notes spacing reduced for a more compact layout.

# Follow-up 2: attack-boom removed at C++ level + D3-E3 spectrum (20260829)

The user decided the boom should be removed entirely, not just disabled via JS.

## Full C++ removal of the attack-«бум» (swell) feature

- `intrasynth/src/Intra/Synth/AdditiveSampler.{h,cpp}`: deleted the whole
  two-phase swell machinery (`mSwell*`, `envA`/`swv`/`gSw`/`dSw`), its hot-loop
  envelope block, the constructor `kBoostDb` table, and `mSwellW` weights.
  `mSwellW` was only used by the swell (the static string-body weights are the
  per-partial `Amp` table), so it is gone too.
- `Types.h`: `RenderParams` reduced to a single `ReverbWet` float; ABI static
  assert now `== 1*sizeof(float)`. JS mirrors this (`paramsPtr` malloc 4 bytes,
  only `ReverbWet` written). `AdditiveSampler::SetRenderParams` override dropped.
- Rebuilt WASM: **166,043 → 164,644 bytes** (−1,399). Probes and player refreshed.

## D3-E3 spectrum analysis (acoustic mild dip; bright / honky-tonk hollow+quiet)

Measured per-partial (harmonic) levels of our synths at keys 50-52 vs the raw
SF2 `D3_sample` (region-51 source), using `probe-d3-spec.js` / `probe-d3-h2.js`.

- **Attack (10-60 ms)**: our h2/h1 ≈ −0.7 dB ≈ sample −1.1 dB → amplitude OK.
- **Body (200-500 ms)**: sample holds h2 at **−4.0 dB** rel h1; ours collapsed to
  **−9.8 dB (Acou), −6.1 (Bright), −5.4 (HT)** → a hollow, weak-octave body —
  the "корявый" timbre, worst on the derivative pianos.
- Root cause in the shared `PianoRegions.h` table: **region 51 (D3-E3) h2 decay
  is an outlier** — D1/D2 ≈ 3.8/s, i.e. ~4-5× h1, and every neighbour region's
  h2 decays at 0.2-2/s. h2 lost its energy through the first ~0.55 s (SegT2).

### Fix (evidence-backed, single region)

`.scratch/PianoRegions.readable.h` region-51 h2, repacked via
`pack-piano-table.js`:
- `Amp 19562 → 25000` (+2.1 dB, toward the SF2 D3 attack h2 ≈ +3 dB rel h1)
- `Decay1 9930 → 3400`, `Decay2 10093 → 3300` (λ 3.8/s → ~1.3/s, h1 pace)
- `Decay3` unchanged. Old values kept in the in-code comment for revert.

Result after rebuild: h2-h1 body = **−4.4…−4.6 dB (Acou)** vs sample −4.0;
Bright −0.9…−1.6, HT +0.6…+0.8 (brighter by intent). Body RMS rose ~1 dB for
Bright/HT (tail much better). Acoustic D3-E3 body now ≈ C4 (small dip gone).

Build caveat: `pack-piano-table.js` overwrites the packed header with a template
that lacks the hand-added `PianoGetPartial()` helper (and the region-block
source-of-truth is the readable). After repacking, re-add `PianoGetPartial`
(packed-row decoder) to `src/Intra/Synth/PianoRegions.h` before building.
Also keep the readible's region-51 `Loudness` at 0.4300 (it was stale at 0.2127)
so repack doesn't revert it.

### Follow-up: A/B baseline build in the UI (user asked to compare vs last commit)

Shipped a UI A/B toggle so the "C#3-E3 is completely broken" report could be
checked against the last commit (`a99c8ec`) instead of guessing.

- `web/generated/IntraSynth.ref.wasm` = genuine HEAD build (168,428 B, has the
  boom feature + old region table), assembled into `dist/` beside the current
  `IntraSynth.wasm` (164,644 B).
- `synth.js`: `?wasm=ref` loads the baseline via the loader's `locateFile`
  override (`IntraSynth.wasm` -> `IntraSynth.ref.wasm`). Both builds share the
  1-float RenderParams ABI, so only the binary is swapped — no param-path
  change needed. `abBuild` is parsed synchronously so the UI highlights right.
- Header: tiny two-button A/B segmented control ("тек."/"head"); clicking
  reloads the page with/without `?wasm=ref`.

### A/B verdict (objective, probe-ab-ci.js + ab-smoke.js)

Same source region table (region 51) feeds all 8 pianos via one AdditiveSampler,
so the note-by-note seam is identical everywhere.

| key | HonkyTonk: CUR body | HT: HEAD(commit) body | Acou: CUR h3 | Acou: HEAD h3 |
|-----|----------------------|------------------------|--------------|---------------|
| 48 (C3)   | -26.6 | -28.4 | +0.7 | +0.7 |
| 49 (C#3)  | -28.4 | -37.6 | -20.3 | -21.1 |
| 50 (D3)   | -28.7 | -37.9 | -24.1 | -24.5 |
| 51 (D#3)  | -28.8 | -38.1 | -22.7 | -23.3 |
| 52 (E3)   | -29.0 | -38.4 | -20.9 | -22.0 |
| 54 (F#3)  | -24.8 | -26.6 | -7.6 | -7.6 |
| 60 (C4)   | -31.1 | -32.9 | -13.6 | -13.6 |

- **The "совершенно сломано C#3-E3" timbre gap (h3 ≈ -20...-24 dB vs C3's +0.7) is
  in the last commit too** — it predates this session's uncommitted changes.
  Region-51 h3..h10 upper partials are near-absent in both builds. Not caused by
  the uncommitted work.
- **The loudness gap WAS this session's change to fix**: at HT C#3-E3 the
  last-commit body is ~-9.2 dB quieter than current (region-51 Loudness 0.2127 +
  weak h2). The user's "не смогли ли незакоммиченные изменения" → NO for timbre,
  but the body fix is real and is what the A/B toggle will demonstrate.

Remaining real bug (next task): region-51's h3..h10 are the hollow "корявый"
seam on C#3-E3; fix by analyzing the SF2 D3 sample's upper-partial RMS at
200-500 ms and bumping region-51 h3..h10 amplitude/decay to match (same method
as the h2 fix here). Sweep C3/C4 controls already exist in the probes.

## Follow-up 2: region-51 h3..h10 "hollow" seam fixed (timbre, C#3-E3)

Cause of the "корявый" timbre seam C3→C#3 was **region-51's upper partials**
(h3..h10) sitting far below the keyboard trend and below its neighbor regions:

| partial (rel h1) | reg47 C3 | reg51 before | reg54 F#3 |
|---|---|---|---|
| h3 | +1.3 dB | **-23.6 dB** | -7.3 dB |
| h4 | -10.7 | -19.7 | -12.0 |
| h5 | -10.7 | -6.7 (anomalous) | -12.0 |
| h7 | -1.3 | -23.9 | -16.8 |
| h8 | -29.8 | -43.4 | -21.2 |

Note: the D3 **sample** itself is dull there too (D3_sample h3=-24.9 via FFT), but
that single sample creates a physically implausible 24 dB h3 drop over half a step
versus both neighbors. Since the whole 8-piano family shares one table/AdditiveSampler,
the seam hit all of them. Fix: raise region-51 h3..h10 toward a monotone declining ramp
(h3=-3, h4=-8, h5=-10, h6=-12, h7=-14, h8=-18, h9=-20, h10=-22) and align their decay
to the h1/h2 pace (D1~3407 D2~3298 D3~2396).

Applied by packing region-51 rows directly into `PianoRegions.h` (Python, working.
bytes->packRow), NOT via pack script (the readable table had been corrupted by an
earlier over-wide regex; pack script skips such rows so it would drop them).

Result: seamless. key-51 loudmap now 6.8 dB (between 48→6.2 and 54→10.3, no notch);
body spectrum h3..h10 is a clean declining ramp matching registration with neighbors.

### A/B toggle & UI persistence (done in same turn)
- Toggle moved OUT of header INTO debug spoiler ("Отладка > Сборка синтезатора", buttons
  текущая/коммит; `?wasm=ref` loads dist/IntraSynth.ref.wasm = HEAD a99c8ec binary).
- UI state (instrument, volume, reverb, pregen, drums, spoiler) persisted to localStorage
  and restored synchronously before boot() so switching builds no longer resets them.

## Follow-up 3: region-51 = interpolation of regions 47 (C3) & 54 (F#3)  [user-directed]

User reported the follow-up-2 manual ramp ("h3=-3,h4=-8,...") made Acoustic worse and
left Honky-tonk broken — D3-E3 still sounded like a different instrument. Suggested
(better) approach: just interpolate C3 and F3.

Implemented per user direction: region 51 partials rebuilt as a per-harmonic blend of
region 47 (root 47/C3) and region 54 (root 54/F#3), weight t=(51-47)/(54-47)=0.571 in
dB-space (amps) and log-space (decays). K, Phase, FreqRatio, and h1 Amp (21247, loudness
0.43) kept from region 51; only Amp/decay replaced.

Table-level effect on region 51 rel-h1 (vs old + neighbors):
  h2: -8.6 (was +1.4 / reg54 -16.5); h3: -3.6 (was -23.6 / reg47 +1.3);
  h4 -11.4, h5 -11.5, h6 -15.7, h7 -10.2, h8 -24.9, h9 -19.6, h10 -14.7.
Body-spectrum measured: key49-52 h3≈-5, h4≈-10, h5≈-11 → continuous between C3 and F#3,
no further seams. Loudness ramps smoothly through the boundary.

NOTES for future maintainers:
- pack-piano-table.js reads .scratch/PianoRegions.readable.h if present, and that file
  still has the OLD (corrupted/mangled) region-51 rows from the follow-up-2 attempt.
  Do NOT run pack-piano-table.js now — it would clobber the hand-packed interp in
  PianoRegions.h from the (stale) readable. The authoritative table is the packed header
  itself. If re-deriving, regenerate readable from packed header first.
- Region 51 h1 kept at 21247 so the interp is additive-blend only for upper partials;
  region Loudness field still 0.4300 (earlier fix, matches neighbors' loudness trend).

## Follow-up 4: reload-free A/B swap + honky-tonk fast-beating fix (2026-08-29)

### A. A/B теперь без перезагрузки страницы
The debug-spoiler build toggle no longer reloads. New `switchWasmBuild(toRef)` in
web/synth.js re-instantiates `IntraMidiSynth` with the other build (locateFile ->
IntraSynth.ref.wasm) and swaps `Module` at runtime under a `swapping` guard that
makes onAudioProcess output silence. Preserves: loaded MIDI file, playback position
(rea-created + seek), selected instrument (program re-sent to rebuilt keyboard
source), volume (WebAudio gain node is module-independent), reverb (re-applied).
Pre-generated buffer dropped (it is a render of the old binary). Verified in node
that two instance of the emscripten Factory() are independent and both render.

### B. Honky-tonk C#3-E3 fast beating - confirmed & reduced
User hypothesis: the broken D3-E3 honky-tonk is very fast beating. Measured with
.proscratch/probe-ht-beat.js (sliding-DFT per-partial AM rate):
- HT region 51 (keys 49-52): h1~2.5Hz h3~8.5 h5~14 h6~18Hz -> ~3x the region-47
  neighbors (0.85-2.5Hz) and far above the SF2 D3 reference (h3 13.6 worst,
  h1/h2 ~1Hz). Acoustic stays smooth (0.85-2.55Hz) across the same keys.
- Root cause: honky-tonk uses the WIDEST detune in the family (DetuneCents=9.0,
  3 voices, rendered as separate detuned lanes - no beat-collapse). The earlier
  region-51 interpolation boosted upper partials, which HT's wide detune turns
  into fast audible beating.
- Fix: Data-driven; beat rate measured to scale linearly with detune (5.5/9 =
  0.61 -> key49 h1 2.55->1.70, h3 8.51->5.11, h5 14.47->8.51). Reduced honky-tonk
  DetuneCents 9.0->5.5 in InstrumentLibrary.cpp. Keeps honky-tonk character, drops
  low-mid beat rates to region-47/SF2 band. Rebuilt wasm, refreshed dist.
- Checked FreqRatio: region-51 packed rows are all near-harmonic sharp (~1.00-1.1,
  from R51_orig); NOT the cause (earlier "4% flat" reading was a byte-offset
  mis-decode - do not trust that; trust apply-interp.py R51_orig).
- Seam after detune change still continuous (probe-seam.js): Acou/HT body spectrum
  smooth across keys 47-52; HT low-mid beats halved.

## Follow-up 5: Honky-tonk C#3-E3 "strong beating" — ROOT CAUSE = region-51 base=7.0 outlier (2026-08-29)

User: "Всё равно сильные биения, и только у этого региона. Расстройка глобальная или
региональная? Не хочу менять остальные регионы. Ищи особенное связанное именно с C#3-E3."

Answer: both. Instrument-wide DetuneCents (global, honky-tonk=9.0) is multiplied by a
PER-REGION `base` spread hipline in the AdditiveSampler constructor:
  effective = DetuneCents * (base / 1.4)
Region 51 (midi 49-52) was the ONLY mid-region with base=7.0 (neighbors 47/54/57 base=0).
For honky-tonk: 9.0 * 7/1,4 = ~45 cents effective → near-total periodic cancellation
of many partials exclusively on C#3-E3 (measured per-partial AM depth h1~30dB, h2-h7
15-53dB vs neighbors ~1.5-4dB). C3→C#3 was a hard beating on/off boundary => "different
instrument". Globally lowering honky-tonk DetuneCents was rejected (user: keep other regions).

Fix (additivesampler.cpp): set region-51 base 7.0 -> 0.0 (match neighbors). C#3-E3 now
sits on the trend:
- probe-ht-depth: HT region 51 h1-h7 AM depth all ~1.5-4dB (was 15-53dB), == region 47/54.
- probe-seam: body spectrum continuous across keys 47-52; HT h2=-6.6 h3=-2.0 h4=-8.5...
- loudmap: keys 48/51/54 ramp smoothly (no notch) for all instruments.
Trade-off: acoustic region 51 also loses its sample-matching ~1.5Hz h2 wobble (now on-trend
with neighbors like C3/F#3, which the user has repeatedly asked for). This removes the last
quantized outlier parameter on region 51; every other region untouched. Rebuilt + dist.

Reverted the earlier experiment (honky-tonk DetuneCents 9.0->5.5) — that was wrong scope.
Probes: .scratch/probe-ht-depth.js (per-partial AM depth), probe-ht-mod.js (broadband
envelope), probe-ht-beat.js (per-partial beat rate), probe-seam.js, scripts/_tmp-loudmap.js.

## Session 2026-08-29 (evening): revert region-51 interpolation for acoustic, per-instrument beat scale

Request: "в acoustic piano откатывай все интерполяции, пусть будет, как в коммите. Но H-T исправился, его надо оставить исправленным. ... Надо измерить реальные значения всех регионов экспериментами ... задавать везде. А для других инструментов, если они ломаются, занулить пока."

### Where the h2 "качка" on C#3-E3 came from (question answered)
Not a hack applied to one region: the unison-spread ladder in AdditiveSampler.cpp is a per-region
profile measured from the SF2 samples on 2026-08-26 (windows 100ms, mono-sum):
  root 43 (G2): h2 ~0.5 Hz -> 4.0c; root 47 (B2): none -> 0; root 51 (E3): h2 ~1.5 Hz -> 7.0c;
  root 54/57: none -> 0; root 60 (C4): ~0.5 Hz weak -> 0.3; C5+: 0.3->1.4c.
base is scaled per instrument: effective spread = DetuneCents x base / 1.4 (spreadHi = AcousticPiano
reference). So region-51 = 7.0c was audible as the h2 wobble on acoustic (1.4 x 7/1.4 = 7c, matches
D3 sample), but honky-tonk (9.0c) got 9 x 7/1.4 = 45c -> destructive AM on C#3-E3 only.

### Re-measurement of the samples (2026-08-29, probe-sample-beat.js / probe-sample-doublet.js)
Ran envelope trough-analysis + high-res spectral doublet detection (0.084 Hz bins) on all 9 SF2
samples (C2, C3, D3, C4, C5, D#5, E5, C6, C7). Result: the samples contain NO clean periodic
string beats in the midrange — partials are stable single peaks; only a slight pitch smear
(~0.1-0.4 Hz) in long windows in bass/mid, and fast deep modulation in the treble (D#5+).
The 2026-08-26 ladder therefore stays as the established calibration (region-51 7.0c = commit
behavior for acoustic); nothing to change from re-measurement.

### Changes
1. PianoRegions.h: region-51 partial rows (214..251) restored to HEAD values (interpolated
   h2=7887/h3=13927/... reverted; now h2=19590, h3=1409 etc., quantized from commit's
   19562/1412/...). Loudness=0.43 (region table) KEPT — user confirmed no loudness dip now.
   All other 506 rows byte-identical (verified by script).
2. AdditiveSampler.{h,cpp}: new per-instrument `beatScale` (default 1.0) multiplies the whole
   region beat ladder (`detuneCents *= (base * beatScale) / spreadHi`); region-51 base restored
   to 7.0 in the shared ladder.
3. InstrumentLibrary.cpp: BeatScale=0.0 for HonkyTonkPiano (kept fixed, as before) AND — after
   measuring per-partial AM depth with the restored ladder (probe-inst-depth.js) — for
   BrightAcousticPiano (3 voices, 0.7c x 7/1.4=3.5c -> h3-h8 AM depth 15-40 dB on C#3-E3,
   same "different instrument" break as honky-tonk), ElectricGrandPiano (2.5c x 5 = 12.5c ->
   h2 seam ~10 dB vs ~2 dB neighbours), ElectricPiano1 (h2 seam ~8 dB). EP2/Harpsichord/Clavinet
   left at 1.0 (EP2 & Harpsi: 1 voice / DetuneCents=0 — unaffected by the ladder; Clavinet:
   short note, 2-voice collapse bounds the depth).
4. Rebuilt WASM (164,692 B), refreshed dist, verified: acoustic region-51 h2 wobble ~13-14 dB
   (commit), HT/Bright/ElGrand/EP1 h1-h6 depth <= ~7 dB everywhere; body spectrum of region-51
   back to commit shape (Acoustic h3 ≈ -23 dB matches D3 sample -23.1); loudmap continuous
   (key 51 sits between 48 and 54 for all 8 pianos).

### Trade-off (documented for the user)
Acoustic C#3-E3 = commit exactly (incl. the "sample" h2 wobble and hollow upper partials).
Honky-tonk/Bright/ElGrand/EP1 lose only the region-specific beat profile; their own DetuneCents
character is untouched. Per-instrument BeatScale values are single numbers in
InstrumentLibrary.cpp — trivial to re-enable per instrument later.

## Follow-up 6: HT lost its character ("sounds like AGP") — restore ladder, tame only D3 (2026-08-30)

User: "Теперь HT звучит как AGP, а тогда у него было своеобразное звучание, которое было норм
везде кроме региона D3 (а C3 и ниже всё равно были как фортепиано)."

### Diagnosis
`beatScale` semantics are `detuneCents *= (base * beatScale) / spreadHi` — it scales the ENTIRE
instrument detune, not just the ladder. Setting Honky-tonk BeatScale=0 therefore zeroed its own
DetuneCents=9.0 too (9 × (base·0)/1.4 = 0), flattening the whole keyboard into plain AGP
(region 43 bass and C5+ treble lost their honky character with the mid).

### Fix
1. InstrumentLibrary.cpp: Honky-tonk BeatScale 0.0 → 1.0 — restores the full region ladder, so
   HT keeps its commit character (bass G2 ~26c, treble C5+ 9c: deep honky beating).
2. AdditiveSampler.cpp: capture `instDetune` before the multiply; for region 51 (midi 49..52)
   ONLY, if `instDetune > spreadHi` (a wide preset), force `base = 0` — so D3-E3 stays calm
   (its earlier approved fix) instead of re-breaking into ~45c destructive beating. Narrow
   acoustic (1.4 == spreadHi) is NOT caught, preserving its commit region-51 h2 wobble.
3. Bright/ElectricGrand/EP1 remain BeatScale=0 (user: "для других инструментов... занулить пока").
   EP2/Harpsi/Clav remain 1.0 (no effect / short notes).

### Verification (AM depth dB, probe-inst-depth.js)
- HT is honky again: key43 h1..h4 17-23 dB, key60 3.8-30, key72 14-35, key84 15-19 —
  clearly not flat AGP.
- HT region 51 (D3-E3) calm: h1..h6 ≤ ~7 dB (h2 ~6.5), not the broken 45c "different instrument".
- Acoustic region 51 keeps commit h2 wobble ≈ 13-14 dB (keys 49-52) — untouched.
- dist/IntraSynth.wasm rebuilt (164,728 B).

### 2026-08-30 follow-up: HT still "sounds like AGP" — D3-E3 was the distinctive part

User: "Не знаю, что ты там изменил, звучит как AGP. Сделай анализ спектра и покрути параметры."

#### Spectral analysis (probe-ht-vs-ago.js: current build vs commit ref wasm, keys 43..84)
- HT cur == HT commit character EXACTLY everywhere except D3-E3 (only approved change). The
  commit's distinctive sound was: bass G2 25.7c (AM 17-23 dB), treble C5+ 9c (AM 15-37 dB),
  C4 1.9c (AM 15-30 dB on h2-h6), mid C3/G3 flat (base=0 in the ladder — same in commit),
  and D3-E3 at 45c (broken: h1-AM 31.8 dB, h1beat 3.8 Hz, h1 peak −36 dB = −11.6 dB dip).
- "Sounds like AGP" = D3-E3 had been zeroed (h1-AM 1.5 dB, flatter than AGP's own 7c wobble),
  and mid C3-G3 is flat in both commit and current (ladder base=0) — so without the D3-E3
  wobble the whole tested range read as AGP.

#### Fix (one line, no new table)
AdditiveSampler.cpp: for wide presets (instDetune > spreadHi) region 51 base: `0.0f` → `spreadHi`.
HT D3-E3 now gets exactly its own detune (9c, same as its accepted treble) instead of 45c or 0:
- D3: h1-AM 18.6 dB (vs 31.8 broken / 1.5 flat), h1beat 1.90 Hz (slow chorus, not flutter),
  h1 peak −25.4 dB (vs −36.0 broken — no loudness dip).
- Commit character preserved everywhere else (G2 25.7c, C4 1.9c, C5+ 9c, mid flat).
- Acoustic and BeatScale=0 presets (Bright/ElGrand/EP1) untouched (same code path as before).
- dist/IntraSynth.wasm rebuilt (164,728 B).
- Trade-off: D3-E3 RMS dips ~4 dB vs neighbors (loudmap key 51: 4.0 vs 48: 7.9 / 54: 9.6) —
  inherent to a honky wobble; the commit's peak dip was 11.6 dB. Knob: region-51 base for
  wide presets (spreadHi = 9c; lower toward 0 = calmer, higher = more honky).
