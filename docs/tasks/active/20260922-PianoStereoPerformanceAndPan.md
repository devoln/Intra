# Piano stereo: регресс производительности и MIDI pan

Дата: 2026-09-22

## Risk Level

Medium: горячий SIMD-цикл additive piano, жизненный цикл голосов и MIDI CC10.

## Goal

1. Воспроизвести резкое замедление Chopin после true stereo и отделить цену stereo от накопления затухших голосов.
2. Проверить фактический WASM SIMD.
3. Исправить только доказанную причину производительности без слышимой смены принятого fast-onset/true-stereo тембра.
4. Сделать MIDI pan/CC10 независимым внешним балансом поверх внутреннего stereo семпла.

## Invariants

- Принятые per-partial L/R gain+phase и fast onset не меняются.
- Внутренний stereo piano сохраняется при MIDI pan=0; CC10 применяется поверх него как channel balance.
- SIMD production path остаётся включён реально, не только флагом компилятора.

## Findings

### SIMD

Canonical Emscripten 6.0.9 build contains real wasm SIMD128 code. The slowdown was not a scalar-WASM regression.

### Root cause of the large slowdown

The accepted fast-onset loop assigned `mAtk = stringRiseStep` to every SIMD lane, including silent table rows and padding lanes where `crs==cis==0` and all decay/release steps are exactly 1. Those lanes ramped `mAmp` toward 1 while producing zero audio. After `NoteRelease`, `atk` became zero but `amp` stayed near 1 forever, so the existing `maxAmp < 1e-3` voice-cleanup gate never fired.

The fix applies the fast onset only to real measured partials (`crs^2+cis^2 > 0`). Full Chopin rendering no longer slows down as old notes accumulate.

### True-stereo cost

With the lifetime leak removed, browser performance is only about 10–15% slower than the pre-stereo build in the owner's measurement. This is lower than the earlier ~20–25% synthetic estimate and confirms that true stereo itself was not the large regression.

### MIDI pan / CC10

The accepted HEAD stored CC10 but additive piano did not receive it because `NoteSampler::SetPan()` forwarded only to waveform/wavetable samplers. CC10 also did not repan notes that were already sounding.

Fix:
- add no-op `IGenericSampler::SetPan`;
- forward `NoteSampler::SetPan` to generic samplers;
- additive piano applies MIDI pan as an outer linear balance over its measured per-partial stereo image;
- CC10 repans already sounding notes and is pushed to MIDI feedback.

Center pan is byte-for-byte identical to the accepted measured true-stereo render.

## Verification

Deterministic:
- production WASM contains SIMD128 instructions;
- full Chopin render completes without accumulating dead released voices;
- center-panned piano is bit-identical to the accepted true-stereo build;
- hard and dynamic CC10 move the additive-piano stereo image without rebuilding oscillators.

Human verification by project owner:
- fixed browser build works correctly;
- performance is only ~10–15% slower than before true stereo;
- Eiffel/D4 sounds correct;
- MIDI pan works correctly.

## Decision

Accept both fixes together:
1. do not ramp silent/padding additive lanes during fast onset, restoring normal voice cleanup;
2. treat MIDI CC10 as an outer channel-pan layer over the measured piano stereo image, including already sounding notes.

The accepted per-partial stereo fingerprint, fast onset, Amp/Decay fit and unison model remain unchanged.
