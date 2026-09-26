---
name: project-intra-fitting-and-measurement
description: Use for any IntraSynth sound work — fitting an instrument to the bank, fixing timbre/attack/noise/period/beating defects, or deciding what to measure. Catalogues the fitting approaches, the script that drives each one (including the uniform report intrasynth/tools/analysis/fit-report.mjs), the acceptance loop, and the traps that already cost updates. Includes the owner's per-class white-noise rule and the "wide harmonics beat noise" conclusion.
---

# Intra Fitting And Measurement

Project skill for turning "sounds wrong" into numbers and back into sound.
The authoritative reference for *reading* measurement output is
`intrasynth/tools/analysis/README.md`; this skill tells you which approach to use,
with which command, and where the traps are. Do not re-derive either.

## 0. Before the first measurement (three checks)

1. `node intrasynth/tools/analysis/cli.mjs selftest` — if it is not PASSED, every
   number you are about to produce is invalid. One FFT, under test, in
   `intrasynth/tools/analysis/lib/fft.mjs`.
2. Time is measured **from NOTE-ON**, not from the start of the file
   (`--noteon`, default 0.2 s). Windows counted from file zero measure silence.
3. The reference for the bank is the **bank preset**, mapped from the synthesizer
   program in `intrasynth/tools/analysis/lib/render.mjs` (`BANK_PROGRAM`). The CLI
   prints the substitution; if it does not appear, the reference is the same number,
   not the preset you heard.

For any A/B claim, render a second build and compare with `--wasm <path>` instead of
comparing numbers across turns.

The uniform report over any instrument is
`node intrasynth/tools/analysis/fit-report.mjs <prog:key ...>` — level, h2..h12
profile, harmonic width in cents, AM depth/speed and the table-loop autocorrelation in
one table (`--wasm`, `--json`, `--no-bank`). Reach for it before writing any probe:
if a metric is missing there, add it to the library (section 5), not to a new script.

## 1. White noise: the owner's rule (by instrument class)

Full record: `intrasynth/docs/tasks/active/20260926-VoiceFamilyArcLog.md` (§12).
The short form, and it is a per-class permission, not a general tool:

| class | noise |
| --- | --- |
| strings (guitars 25-31, violins, pizzicato) | **BANNED** outright: no layer, no bands, and the bank's floor is not a target (owner, three times; `intrasynth/tools/instruments/guitar-modal.json`, guitar worklog). Noise exists only inside the Karplus-Strong excitation |
| flutes / whistles / ocarina (43, 71-79, 115) | **EXTREME CAUTION**: only as tone-correlated filtered "air" (comb on the note period) **and** with band measurements against the Titanic original plus a separate attack check in 0-60 ms windows. Four puff regressions came from shape, not level (Updates 31, 73, 74, 75). Default: do not add |
| voices/choir (50/52/53/54/91/94) | allowed as "air", shaped in hertz from the bank's bands |
| percussion & effects (Gunshot 127, Seashore 122, Applause 126, Helicopter 125, drums) | **REQUIRED** — the noise IS the instrument (`NoiseInstrument`/`NoiseSampler`); only the band and envelope must match the bank |
| everything else | no by default; add only after a measurement shows the bank has it |

The owner's conclusion, which changed a whole line of work: **wide harmonics turned
out better than noise** where "roughness/spark" used to be imitated with a noise layer —
PADsynth skirts (`harmonicWidths`, the `b(k)` formula) produce no attack flash and do
not drift with the key.

- Where noise IS allowed it must be a **strictly filtered table layer**: a noise table
  (default 32768 samples) shaped by the `NoiseSampler` filter chain — one-pole cascade
  cutoff at `cutoffMultiplier × f0`, optional high-pass at `hpMultiplier`, optional
  comb. Unfiltered white noise reads as hiss/пшик and is rejected by ear immediately.
- Shape it like a vowel, **in hertz, not per harmonic number**: the bank's noise floor
  falls with absolute frequency, while a per-`k` slope would make the same preset
  darker or brighter depending on the key
  (`intrasynth/docs/tasks/active/20260926-VoiceFamilyArcLog.md`, §12). Consonant bursts are
  the exception — their spectrum does not move with the key, so they are shaped by `k`.
- Two more ways noise lies to you: a flat shelf up to ~14 kHz (sounds like "шумное
  тело" even at the right level), and a table whose loop period is audible (see 2.3).
- Verify before asking for an audition: `bands` for the shape, `timbre` for the burst
  in the attack, and the subtraction trick below if you suspect the noise layer at all.

```sh
# форма шумовой полки — только АБСОЛЮТНЫМИ полосами (±25 Гц), не «floor»
node intrasynth/tools/analysis/cli.mjs bands 53:60
# вспышка шума в атаке: короткие окна 60 мс от note-on
node intrasynth/tools/analysis/cli.mjs timbre 53:60 --to 0.5 --len 0.06
# изоляция шумового слоя: сборка с level = 0 у шумовых слоёв, затем вычитание рендеров
# (шум и тело детерминированы по seed/фазе, поэтому разность равна ровно слою)
```

## 2. Approach catalog

### 2.1 Harmonic table profile (timbre of a sustained tone)

`spec` with `--kmax` **larger than the profile array** — if our level falls to the
noise floor while the bank still has partials, the array is too short and the "spark"
is missing. Compare band by band, not only peaks.

```sh
node intrasynth/tools/analysis/cli.mjs spec 52:60 --kmax 24
node intrasynth/tools/analysis/cli.mjs floor 52:60        # межгармонический пол (низкие f0)
```

Editing the profile changes **two** things: table normalization (`BuildWaveTable`
normalizes by Σaₖ) and loudness (RMS). Compensate `VolumeScale` by RMS
(`old × rms_new / rms_old`) and re-check the sustain level against the previous build.

### 2.2 Harmonic width b(k)

Use `vib --harm N` for per-harmonic motion and a width probe for line width in cents
at −6 and −20 dB, plus the share of energy inside the line. Rule of thumb from the
choir work: the bank's width is roughly **constant in cents** across h2..h12
(median ≈ 93 for 52:60), while `AddSineHarmonicGauss` derives σ in **hertz from f0**,
so in cents the width falls with harmonic number (≈82 cents at h1 vs ≈7 at h12 for
b = 160). A single `bwHiCents` constant therefore cannot describe the bank; a
`b(k) = c·k` form can.

The measurement now lives in the shared library: `harmonicWidths` in
`intrasynth/tools/analysis/lib/harmonics.mjs` (per-harmonic w-6/w-20 in cents, line
share) — the old `.scratch/u183-width.mjs` probe is superseded by it.

Measured anchor: for 52 ChoirAahs, `b(k) = 52·k` cents reproduces the bank width
(91 vs 93 cents). **Since Update 188 that formula IS the canon for 52 ChoirAahs**, and
the ensemble is gone — the owner's ear overrode the Update 186 argument (see 2.3 and
`intrasynth/docs/tasks/active/20260926-VoiceFamilyArcLog.md`, §12). The width
is per-preset: `VoicePresetSpec::BwSlope` (`0` = the old clamp formula, still used by
50/53/54/91/94).

### 2.3 Ensemble / stacked voices

An ensemble is N detuned copies of the same table (`choirRes, N`). Criterion for
"the movement is choral, not robotic":

```sh
node intrasynth/tools/analysis/cli.mjs mod 52:60 --from 0.9 --band 0.2,15   # линии АМ: должны быть вразнобой
node intrasynth/tools/analysis/cli.mjs period 52:60                        # пик на лаге ровно N семплов = луп таблицы
```

- Uncorrelated AM lines spread over ~6-10 Hz = ensemble. Lines standing on
  `fs / tableLength` (16384 → 2.69 Hz) with a high autocorrelation peak at the table
  period (370 ms) = **table loop**.
- Two symmetric detunings always give ONE dominant beat pair, and its rate is
  **`f0·Δcents/1731`** — proportional to BOTH the detune and the key. That is what the
  owner hears as "strong beating that only gets worse on C5+" (50 SynthStrings,
  Update 188: a single line at 3.11 Hz on C4, 6.90 on C5, 14.05 on C6). Cures,
  in the order they were tried: more voices with an IRREGULAR spacing, a per-voice
  vibrato jitter, and (Update 189) simply NARROWING the spread (±34 → ±18 cents halves
  the rate; ±6 cents removes the key dependence almost completely).
- **Width and movement are separate levers.** The measured w-20 is not only the
  harmonic's own skirt — at low keys it is the width of the valley around it, so an
  ensemble widens it too. When the bank is narrow (50 at C5: 37 cents against our 75),
  narrow the ensemble and give the width back through the TABLE (`BwSlope`), not the
  other way round (Update 189 candidate `u189-str-tight-flat`: 45 vs the bank's 37).
- **The owner's ear is the acceptance criterion, and it is allowed to override a
  measured argument.** Numbers: nine voices, then five, then three, then four were all
  rejected by ear; the pure wide table (no voices at all) was chosen as "pleasant" even
  though it keeps the table-lag periodicity (`r 1.0000`, Update 186). If a measured
  "robot" criterion disagrees with the verdict, record the reversal in
  `intrasynth/docs/decisions/` rather than re-running the experiment.

### 2.3b Table length N as a preset parameter (NOT envelope flattening)

Use when the owner reports a **slow wobble on top of an otherwise good sound**: check
whether the AM lines sit on `fs/tableLength` (16384 → 2.69 Hz, 32768 → 1.35 Hz).

```sh
node intrasynth/tools/analysis/fit-report.mjs 52:60   # столбец «АМ 0.2-15 Гц» + список линий
node intrasynth/tools/analysis/fit-report.mjs 52:60 --wasm .scratch/ab-builds/u190-str16k/IntraSynth.js
```

Recipe (Update 190): `unsigned TableLength` is a field of `VoicePresetSpec`, passed into
`BuildWaveTableCore` by `BuildVoiceTable`. The patcher
`.scratch/u190-patch.mjs --tbl50 16384 --tbl52 32768` moves individual presets. Canon:
32768 for 50/51/52, 16384 for 53/54/91/94.

What doubling N buys and costs (measured, 52:60 and 51):

* the exact table repeat moves 371 → 742 ms and the AM lines sit twice lower, but the
  **depth of the wobble does not drop** (8.52 → 8.34 dB);
* the timbre skews **loud/soft on the skirt**: the wide part of a harmonic is spread
  over `N·bwi` bins with random phases, so its amplitude grows as √N (+3 dB per
  doubling) while the narrow lines and the `amplSum` normalization do not move. On 52
  the level vs the bank moved +2.85 → −0.12 dB (which is why the length change must be
  re-checked against the bank level, not only against the previous build), on 51 the
  profile at C5 went 1.5 → 5.9 dB of error;
* 65536 stays REJECTED (Update 174).

**Envelope flattening (`FlattenTableLoop`, Update 189) is REJECTED BY EAR — do not
retry it.** The measurement liked it (depth 8.52 → 2.47 dB at 52:60) and the owner still
said «делает ещё хуже, такой подход отбраковываем везде». The code is gone. Lesson, in
one line: AM depth is not the defect — if the ear dislikes the cure, look for another
mechanism (see the tiling notes below), do not push the correction harder.

Graphics→audio analogues for hiding a loop (proposal, not yet built): ensemble of the
same table read at slightly different rates = texture bombing (already in canon for
50/51); crossfading TWO tables with incommensurable lengths (16384 + 24576 → combined
period 1.11 s) = Wang tiles / composite tiling, the only one that removes the
periodicity instead of masking it; a baked wide-harmonic floor = detail noise, masking
only (and the owner has already rejected too much of it as «шумно»).

### 2.4 Attack shape and consonant bursts

Between the body attack and the consonant layer, measure both, in 2 ms and 60 ms
windows, and fit numerically rather than by eye:

```sh
node intrasynth/tools/analysis/cli.mjs attack 75:60 --len 2 --to 90
node intrasynth/tools/analysis/cli.mjs timbre 75:60 --to 0.6
node intrasynth/tools/analysis/fit-attack.mjs 74:62 74:72 74:84   # численный фит по банку
```

A "т"-like consonant wants a **burst**, not shaped noise: too short reads as a click,
too hissy reads as English "тш". Compare the burst's own profile with the bank's in
the attack window, not in the sustain.

### 2.5 Breath / air layers (see the white-noise rule above)

Steps that work: measure the bank's noise profile by absolute bands → choose a single
absolute tilt (Hz-based) → render → re-measure shape and early burst → only then fold
it into the A/B panel. Keep the layer's own level tied to the body, and watch the comb
setting: contrast too high lifts even harmonics where the instrument has none (sounds
like aliasing, `spec` against a zero-noise build proves it).

### 2.6 Envelopes, decay and release

```sh
node intrasynth/tools/analysis/cli.mjs env    74:72 --len 25 --to 6000
node intrasynth/tools/analysis/cli.mjs attack 74:72 --len 2  --to 150
```

For piano there is also the joint sustain fitter `intrasynth/tools/analysis/fit-piano.mjs`
(flags and usage are in its header comment; it writes a JSON report and can emit a
separate header for diff/review).

Traps: `SineRange` degenerates below ~4 Hz (rounding `2 − mK` to 2.0 gives linear
growth) — use a phase accumulator for slow envelopes; dB→amplitude conversion is
`20·log10(2) = 6.0206`, and feeding dB straight into a power-of-two exponent
over-attenuates by ~6×; piano decay is fitted per harmonic from one spectral line.

### 2.7 Keyboard regions and value curves

Per-key variation is expressed as regions (anchors) interpolated over the keyboard.
Measure at the anchors, not in the middle, and check monotonicity where the bank is
monotone. Modal instruments (guitars) keep their numbers in JSON
(`intrasynth/tools/instruments/guitar-modal.json`) and are inserted by a generator
(`intrasynth/tools/instruments/guitar-apply.mjs`) which also checks field order — a
silent aggregate-initialization shift is the classic failure mode there.

Interesting probes can also drive the WASM directly (`Factory()` + `SourceCreateLive`
+ `SynthSet...` debug hooks) to sweep dozens of values without rebuilding — but keep
**one** instance per sweep: the helper inside `renderOurs()` creates its own instance
and will not see knobs you set on yours. Always include a control value that must be
audible; if the control does not move, the sweep is blind.

### 2.8 Loudness

`BuildWaveTable` normalization means a profile edit moves the level. Compare sustain
RMS between two builds (`--wasm`), and level-match anything the owner will hear
(`normalizeSustainRms` in the A/B panel; an unleveled "bank is quieter" is a wrong
conclusion, not a finding).

## 3. Acceptance loop

1. Change one variable (numbers stay in `InstrumentLibrary.cpp` or the instrument
   JSON, applied by its generator).
2. `sh scripts/build-wasm.sh`, then `node scripts/build-web.js`, then
   `node scripts/smoke-test-wasm.mjs`.
3. Snapshot the previous pair into `.scratch/ab-builds/<id>/` if you still need the
   "before" ear (the panel replays snapshots, and build-web wipes `dist/`).
4. Measure: candidate vs bank, and candidate vs previous build (`--wasm`), plus the
   specific symptom metric from section 2.
5. `node intrasynth/tools/ab/render-ab.mjs` and label the entries so the owner can
   hear our variant, the rejected variant and the bank.
6. Report the WASM size and md5, and put the numbers in the worklog. Owner verdict is
   the acceptance criterion; rejected variants get marked as rejected in the panel and
   kept only as history.

## 4. Do not measure with these

- A narrow band around a harmonic for tremolo — it converts FM into AM; use
  `trem --wide`.
- `floor` (relative exclusion) above ~1 kHz on a C4-based instrument — the ±0.15·f
  exclusion eats most bins; use `bands`.
- `trem --wide` for the modulation *frequency* — the coherent peak is quantized to
  ~2 Hz; use `mod` with a long window.
- The modulation spectral centroid across renders with different noise levels — the
  noise skirt biases it; compare line *ratios* instead.
- A metric that has not been calibrated against an already accepted program: if it
  does not separate accepted from rejected work, "improved from 4.4 to 3.0 dB" means
  nothing.

## 5. Where new analysis goes

New measurement code goes into `intrasynth/tools/analysis/lib/` and gets a CLI
subcommand (or a column in `fit-report.mjs`) — not into a copy-pasted probe in
`.scratch/`. Probes are drafts; a measurement that is needed a second time, or for a
second instrument, moves into the library the same day
(`intrasynth/docs/tasks/active/20260926-VoiceFamilyArcLog.md`, §12).

## References

- `intrasynth/tools/analysis/README.md` — чем и как мерить, разбор каждого вывода
- `intrasynth/tools/analysis/fit-report.mjs` — единый отчёт по инструменту
- `intrasynth/docs/tasks/active/20260926-VoiceFamilyArcLog.md` — сводный лог хоровой/голосовой работы: правило шума по классам,
  форма в герцах, юбки против полки, длина таблицы, FLAC, где живёт инструмент и мера
  (§12 — по строке на каждое решение, §5 — отклонённые механизмы)
- `intrasynth/docs/Architecture.md` — карта проекта и инварианты
- `.agents/skills/stack-freebuff-cloud-session/SKILL.md` — правила среды
