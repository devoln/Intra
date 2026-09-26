---
title: "Tune non-piano instruments (guitars, mallets, others) against the SF2 reference"
status: "planned"
created: 2026-08-21
updated: 2026-08-21
risk_level: medium
related_files:
  - intrasynth/src/Intra/Synth/InstrumentLibrary.cpp
  - intrasynth/src/Intra/Synth/KarplusStrongSampler.cpp
  - intrasynth/src/Intra/Synth/GaussianStringSampler.cpp
  - intrasynth/src/Intra/Synth/SpectralStringSampler.cpp
  - intrasynth/src/Intra/Synth/WaveTableSampler.cpp
  - intrasynth/src/Intra/Synth/MidiInstrumentMapping.cpp
related_decisions: []
related_skills:
  - shared-task-init-discipline
  - shared-human-verification-queue
---

# Task

## Goal

Bring the remaining (non-AcousticPiano) programs closer to the same SF2
reference (`/tmp/sf2extract/Titanic 200 GM-GS v1.2.sf2`) that was used to fit
the acoustic piano. Only the AcousticPiano program was tuned sample-by-sample
so far. Every other program currently runs on approximate physical models /
wavetables and was never compared against the SF2 by ear or by measurement.

Candidate groups, in rough priority order:

1. Piano-family programs sharing the `PianoRegions.h` table (BrightAcousticPiano,
   ElectricGrandPiano, HonkyTonkPiano, ElectricPiano1/2, Harpsichord, Clavinet):
   verify they still sound right after the shared top-octave/decay/unison edits.
2. Plucked strings via Karplus-Strong / GaussianString / SpectralString
   (ElectricGuitarJazz, ElectricGuitarClean, basses, sitar, etc.).
3. Mallet / chromatic percussion (Celesta, Glockenspiel, Music Box, Vibraphone,
   Marimba, Xylophone, Tubular Bells, Dulcimer).
4. Anything else that maps to a real SF2 program and currently sounds off.

## Out of Scope

- Further AcousticPiano tuning (tracked separately in
  `docs/tasks/active/20260820-PianoAttackTimbreFix.md`).
- WASM size optimization.
- Adding new instruments that do not exist in the SF2.

## Milestones

- [ ] `M1:` Enumerate which GM programs map to which synth model
  (`MidiInstrumentMapping.cpp` + `InstrumentLibrary.cpp`).
- [ ] `M2:` For one reference program per model family, render the same key
  with fluidsynth and with our synth, and compare attack/timbre/decay the same
  way the piano work did (per-harmonic windows, not just loudness).
- [ ] `M3:` Tune one family (start with the piano-family programs sharing the
  region table, since they already inherit most of the piano fixes).
- [ ] `M4:` Human listening pass per family; iterate only on families the user
  actually cares about.
- [ ] `M5:` Document which programs were verified and which remain approximate.

## Next Safe Step

Do not start until the acoustic-piano work is finished and the user explicitly
asks to move on to other instruments. First concrete action is `M1` (map
program → model) with no code changes.
