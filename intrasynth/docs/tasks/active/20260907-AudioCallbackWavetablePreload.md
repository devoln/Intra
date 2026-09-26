---
title: "Preload exact file keys: no wavetable generation inside the audio callback"
status: "active"
created: 2026-09-07
started: 2026-09-07
updated: 2026-09-07
risk_level: medium
related_files:
  - Intra/Audio/Midi/MidiFileParser.h
  - Intra/Audio/Midi/MidiFileParser.cpp
  - intrasynth/src/Intra/Synth/Instrument.h
  - intrasynth/src/Intra/Synth/InstrumentSet.cpp
  - intrasynth/src/Intra/Synth/MusicalInstrument.cpp
  - intrasynth/src/Intra/Synth/MusicalInstrument.h
  - intrasynth/src/Intra/Synth/EmscriptenInterface.cpp
related_tasks:
  - 20260820-WebAudioOverflowCheck
related_skills:
  - shared-task-closeout-discipline
---

# Task

User report: in the browser preview the sound keeps getting interrupted; it
feels like "one note of one instrument interrupts another" (звук прерывается,
одна нота прерывает другую). Fix the interruptions for file playback in the
preview.

## Risk level

Medium: touches the shared MIDI-info struct and the instrument preload path;
must not change the rendered audio at all.

## Root cause (measured)

The browser renders through a ScriptProcessorNode with 256-sample callbacks
(~5.8 ms budget at 44.1 kHz). The first note of each (instrument, exact key)
generates its wavetable lazily in `WaveTableCache::Get` — which is called from
`WaveTableInstrument::operator()` at `OnNoteOn` inside the render callback
(`ProcessEvent → OnNoteOn → CreateSampler`).

Timing probe (`node .scratch/timing-probe.mjs`, celine.mid, 256-sample pulls):

| build | calls over 5.8 ms budget | worst |
|---|---|---|
| before | 10 (6.5–17.6 ms) | 17.6 ms |
| after  | 0 | — |

Each over-budget callback drops audio for **all** currently sounding notes,
exactly at the moment a previously-unheard key of some instrument starts —
matching the user's description.

Why the existing preload missed it: `PreloadTables` (and `mapping.Preload` in
`SourceCreateFromMidiFileData`) preload only an octave grid
{55, 110, 220, 440, 880, 1760, 3520} = the A keys. `WaveTableCache::Get`
matches on exact frequency (±0.01 %), so every other key (C, D, F#…) still
generates inside the callback. `SourceCreate` was 383 ms before, 597 ms after
(one-time, off the audio path).

## Fix

- `MidiFileInfo` now records, per used melodic instrument, the exact MIDI keys
  that sound in the file (`StaticBitset<128> UsedKeysPerInstrument[128]`,
  filled by the existing counting device).
- `MidiInstrumentSet::Preload` calls `MusicalInstrument::PreloadKey(freq)` for
  every used key (same frequency formula as `NoteOn::Frequency`, so the cache
  key matches exactly), after the octave-grid preload.
- New virtual `Instrument::PreloadKey(float freq, unsigned sampleRate)`;
  `MusicalInstrument::PreloadTables` refactored to loop over the grid through
  `PreloadKey` (single shared path).
- Caches live on the shared static library instruments, so re-loads/re-seeks
  of the same file are cheap and the tables survive across sources.

## Deterministic checks (all passed)

- Timing probe: 0 over-budget callbacks after (was 10).
- Audio unchanged: celine render with the new wasm is **bit-identical** to the
  pre-fix build (`cmp` on 12 438 184-sample stereo wav).
- `scripts/smoke-intrasynth.js` PASSED (0 non-finite, peaks 0.164/0.190).
- `scripts/smoke-test-wasm.mjs` PASSED (0 non-finite).
- `dist/` reassembled; `dist/IntraSynth.wasm == web/generated/IntraSynth.wasm`
  (197 780 B). Preview serves `dist/` via `scripts/serve.js`.

## Persona Review

- `RISK`: the live-keyboard path (`SourceCreateLive`) still preloads only the
  octave grid on program change — a new non-A key stalls one callback once.
  Lesser issue; follow-up candidate (full-key preload is ~1.2 s per
  instrument, too slow per program switch as-is).
- `NOTE`: `MidiFileInfo` grew by 2 KB (128×128 bits) — irrelevant for a
  per-source parse.
- `NOTE`: `scripts/build-wasm.sh` requires the Emscripten SDK; it is present
  at `/opt/emsdk` in this sandbox, so the wasm was rebuilt here.

## Needs Human Verification

| Priority | Area | Why human | Steps | Expected | Observed by agent | Devices |
|---|---|---|---|---|---|---|
| High | Preview playback of MIDI files | The interruption is an audible real-time artifact; automated probes only measure timing | Open preview, load Celine (or any file), listen through the dense passages (e.g. ~60 s, ~195-260 s) | No dropouts; note starts no longer cut the currently sounding notes | Timing probe shows 0 over-budget callbacks; audio bit-identical to before | Desktop browser |
| Low | Live keyboard / Web MIDI | First press of a new non-A key still pays one table generation in the callback | Play several different keys in the live panel | At most a rare single short blip on first note of a new key | Not measured in browser | Desktop browser |

## Session log

- 2026-09-07: diagnosed (stale-`dist/` hypothesis disproved by bit-identical
  renders of both wasm files; real cause = in-callback table generation),
  implemented the per-key preload, rebuilt wasm, verified 0 over-budget calls
  + bit-identical audio, reassembled `dist/`.

## Next-Step Handoff

- Listen in the preview (Needs Human Verification above).
- If the live-keyboard blip matters: preload all 128 keys lazily per program
  in the background after program change, or generate tables on the JS thread
  at idle, instead of inside the callback.