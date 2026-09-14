---
title: "ADSR-only direct-to-mix fast path"
status: "active"
created: 2026-09-08
started: 2026-09-08
updated: 2026-09-08
risk_level: high
related_files:
  - intrasynth/src/Intra/Synth/NoteSampler.cpp
  - intrasynth/src/Intra/Synth/NoteSampler.h
  - intrasynth/src/Intra/Synth/Types.h
  - intrasynth/src/Intra/Synth/AdditiveSampler.h
  - intrasynth/src/Intra/Synth/AdditiveSampler.cpp
  - intrasynth/src/Intra/Synth/KarplusStrongSampler.h
  - intrasynth/src/Intra/Synth/KarplusStrongSampler.cpp
  - intrasynth/src/Intra/Synth/GaussianStringSampler.h
  - intrasynth/src/Intra/Synth/GaussianStringSampler.cpp
  - intrasynth/src/Intra/Synth/SpectralStringSampler.h
  - intrasynth/src/Intra/Synth/SpectralStringSampler.cpp
  - intrasynth/src/Intra/Synth/Synth.h
related_tasks:
  - 20260908-NoteAdsrStereoMixIsolation
---

# Goal

Remove the save/zero/add mix-isolation overhead for voices whose only note-level
post-processing is the ADSR envelope, while preserving the invariant that no
voice may multiply the already accumulated shared mix.

# Design

The fast path is selected only when `NoteSampler` has no modifiers, no
WaveForm/WaveTable/WhiteNoise sources, and every generic source explicitly
supports envelope-aware stereo rendering. Each supported source receives the
current ADSR coefficients and multiplies its own value before adding to the
shared channels. Unsupported or composite voices retain the existing isolated
scratch-buffer path.

`RenderEnvelope` is a small value type containing the same exponential/linear
coefficients already used by `EnvelopeSegment`; it advances inside the source
sink, so no second buffer pass is introduced. The generic sampler interface
gets an opt-in capability rather than a fallback that could accidentally
modify shared mix data.

# Invariants

- Never call an in-place envelope/modifier pass on shared frame channels.
- Do not alter WaveTable's existing source envelope or modifier isolation.
- Keep source-local state progression and note cleanup equivalent to the old
  isolated path.
- Keep the additional WASM code small; measure the final artifact.

# Verification plan

- Compile the native synth and run the existing smoke checks.
- Rebuild WASM and record size against the current 198,269-byte baseline.
- Render representative ADSR generic voices and compare output to the isolated
  implementation for finite values, release completion, and no neighbor ducking.
- Re-run the five-melody performance benchmark when the project probe tooling is
  available.

# Session log

- 2026-09-08: source audit completed. WaveTable already bakes its own envelope
  into its render kernel. The new branch therefore targets generic samplers
  first; WaveTable, filters, and mixed source graphs remain isolated.

- 2026-09-08: implemented the opt-in direct path. `AdditiveSampler`,
  `KarplusStrongSampler`, `GaussianStringSampler`, and `SpectralStringSampler`
  now receive a copied `EnvelopeSegment` and multiply their own generated value
  before `+=` into L/R. `NoteSampler` advances the shared ADSR timeline once
  per chunk and removes completed sources without ever applying the envelope to
  the shared mix. Unsupported source graphs retain save/zero/render/apply/add
  isolation.

## Verification results (2026-09-08)

- Native CMake Release build passed:
  `.scratch/build-fastpath`, target `IntraSynth`.
- `node scripts/smoke-intrasynth.js` passed: 192,001 streamed samples,
  zero non-finite samples.
- `node scripts/smoke-test-wasm.mjs` passed: 44,100 rendered samples,
  zero non-finite samples.
- The refreshed scalar ADSR probe passed for attack and release. It also
  verified that a pre-existing neighbor mix remains unchanged after the direct
  render. The earlier failing result came from a stale probe executable, not
  from the engine; rebuilding the probe against the current native library
  produced exit code 0.

A supplemental three-run median benchmark used the five MIDI fixtures at 48 kHz
stereo with 4096-sample pulls. It compared the current generated pair
(`web/generated/IntraSynth.wasm`, 203,561 B) with the earlier 198,269 B pair in
`.scratch/bench-current`:

| melody | earlier pair | current direct path | wall-time change |
|---|---:|---:|---:|
| Celine (282.0 s) | 905.6 ms | **857.7 ms** | **5.3% faster** |
| Merry Christmas (67.3 s) | 161.9 ms | **144.3 ms** | **10.9% faster** |
| Tous Les Garcons (191.1 s) | 401.4 ms | **341.9 ms** | **14.8% faster** |
| Chopin (290.5 s) | 2993.3 ms | 3018.5 ms | 0.8% slower |
| Cherilady flute (215.3 s) | 127.8 ms | 127.8 ms | unchanged |

The result matches the design expectation: the largest gains are on generic
ADSR voices (especially the guitar-heavy Tous Les Garcons); the piano-dominant
Chopin and WaveTable flute are effectively unchanged. The current artifact is
5,292 B (+2.7%) larger than the 198,269 B artifact, but the two artifacts were
not rebuilt from a captured identical configuration, so this is an upper-bound
package comparison rather than a clean byte-for-byte attribution to this patch.
The implementation itself adds only the source-envelope sink/API and does not
introduce another general-purpose render kernel.

## Follow-up audit (2026-09-08)

### WaveTable and envelope ownership

`WaveTableSampler` is not intrinsically unsafe for direct mixing. Its own
`mEnvelope` is already fused into `renderDirect()`/`GenerateMono()` and the
source writes only with `+=`. The reason it remains outside the new
`NoteSampler` fast path is different: a note may have both the WaveTable's
source envelope and a separate note-level `NoteSampler::ADSR` (and possibly
modifiers). The safe isolation branch preserves both independent state
machines, their release/removal rules, source attenuation, vibrato, and true
stereo table offsets. Combining them would require passing and advancing two
envelopes per sample and defining their completion ordering; it is not a
simple optimization. For the common WaveTable voices with no outer ADSR, there
is no isolation overhead: they already use `fillStereo()` directly.

### Noise implementations

`WhiteNoiseSampler` is a small standalone periodic-time noise source. It is not
the same thing as a WaveTable: replacing it with a periodic noise table would
introduce repetition/correlation and would lose its continuous `mT`/`mDT`
state. The live instrument library does not construct `WhiteNoiseInstrument`;
its active breath/noise layers use the separate `NoiseSampler` in `Synth.h`.
`NoiseSampler` owns a generated/low-pass-filtered table and, for some layers,
its own source envelope. It is therefore a separate generic graph and correctly
remains on the conservative path until a two-envelope direct renderer is
explicitly designed. `WhiteNoiseSampler` is currently compatibility/dead-path
code rather than a contributor to the observed WASM increase; removing it can
be considered separately, but should not be mixed into this performance fix.

### GaussianStringSampler

`GaussianStringSampler` has no live factory use: the only production reference
is the explanatory comment in `InstrumentLibrary.cpp`. The active guitars use
`KarplusStrongInstrument`, and the overdriven/distortion guitars use
`SpectralStringInstrument`. The Gaussian translation unit is compiled, but its
functions do not appear in the linked current WASM after dead-code elimination.
It can be archived or deleted as a separate cleanup, but it is not the source
of the extra 5 KB and deleting it is not needed for the fast path.

### Correct WASM size attribution

The Emscripten SDK was not lost. `/opt/emsdk` is present and contains
Emscripten 3.1.74; the earlier failure was only because `emcc`/`emcmake` were
not on the shell PATH after sourcing the Bash-oriented environment script.
The build script's explicit-path fallback is working.

Fresh current-source builds with the same Emscripten configuration give:

| build | CODE | DATA | total |
|---|---:|---:|---:|
| current minimal (`INTRA_PIANO_ALL_TABLES=OFF`) | 160,246 B | 29,761 B | 191,552 B |
| current full (`INTRA_PIANO_ALL_TABLES=ON`) | 160,176 B | 41,840 B | 203,561 B |

Thus the five extra piano tables account for exactly **12,009 B** in this
build (`+12,079 B` DATA, with `-70 B` CODE from layout/codegen). They are not
the 5,292-B difference previously attributed to the ADSR work.

The older benchmark pair is a different source/build snapshot:

| old artifact | CODE | DATA | total |
|---|---:|---:|---:|
| before mix isolation | 154,563 B | 41,941 B | 198,031 B |
| after mix isolation | 154,952 B | 41,790 B | 198,269 B |

That controlled old A/B attributes only **238 B** to the isolation change
(`+389 B` CODE, `-151 B` DATA). Comparing the older 198,269-B artifact with
the fresh current full build gives `+5,224 B` CODE and `+50 B` DATA, or
`+5,274 B` in the wasm sections (`+5,292 B` including section metadata).
Those bytes are source/configuration drift between the two snapshots, not one
new sampler: the current tree contains substantial later changes in
`InstrumentLibrary.cpp`, `AdditiveSampler.cpp`, WaveTable/vibrato code, and
other synth files. Without rebuilding the exact pre-198,269 source snapshot
with the current SDK, assigning those 5,274 code bytes to individual commits
would be false precision. The evidence does establish that the ADSR isolation
change itself was hundreds of bytes, while the large cleanly attributable
payload increase is the separate piano-table option.

## Remaining risks

- Human listening is still required for the guitar/EP release interaction in
  the preview; the automated probe covers the mix invariant but not perceptual
  timbre.
- A same-current-source A/B with the direct path mechanically disabled would
  isolate the exact current fast-path code cost. The historical paired A/B
  already bounds the earlier isolation implementation at 238 B, but it is not
  a byte-perfect attribution for every later source edit.
- The native SSE attenuation path has a pre-existing lane-order difference from
  the scalar reference; the direct-path probe intentionally uses the scalar
  ADSR model and does not alter that unrelated legacy kernel.
