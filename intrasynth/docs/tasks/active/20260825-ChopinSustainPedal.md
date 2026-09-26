---
title: "Fix Chopin note cutoff — implement sustain pedal (CC64)"
status: "active"
created: 2026-08-25
started: 2026-08-25
updated: 2026-08-25
risk_level: low
related_files:
  - Intra/Audio/Midi/Messages.h
  - Intra/Audio/Midi/TrackParser.cpp
  - intrasynth/src/Intra/Synth/MidiSynth.h
  - intrasynth/src/Intra/Synth/MidiSynth.cpp
related_decisions: []
related_skills:
  - shared-task-init-discipline
---

# Task

The user reported that in Chopin (fantaisie-impromptu) notes at 3–6 s cut off
unrealistically ("ноты как-то обрываются нереалистично"). This is a long-standing
problem, not a recent regression.

## Root cause

The synthesizer **ignored the sustain pedal entirely**:

- `TrackParser::ProcessEvent` handled only CC7 (volume), CC10 (pan), CC123
  (all notes off) — CC64 was dropped.
- `IDevice` had no sustain callback at all.
- `MidiSynth::OnNoteOff` called `NoteRelease()` unconditionally, so every note
  was damped at its NoteOff instant, even while the pedal was held.
- The live path (`SendMidiEvent`) also ignored CC64.

In the Chopin MIDI the pedal is held from 0.01 s to 5.51 s (and again from
6.03 s). Without pedal support, ~24 chord notes released at NoteOff times
were damped instead of ringing until the pedal lift, which is what the user
heard as an abrupt, unnatural cutoff in the 3–6 s window.

## Fix

Added sustain pedal support end to end:

1. `Intra/Audio/Midi/Messages.h` — `IDevice::OnSustain(byte channel, bool down)`
   with a no-op default (so the counting device in `MidiFileParser.cpp` is
   unaffected).
2. `Intra/Audio/Midi/TrackParser.cpp` — CC64 now dispatches
   `device.OnSustain(channel, data1 >= 64)`.
3. `intrasynth/src/Intra/Synth/MidiSynth.h` — per-channel `mSustain[16]`;
   `NoteInfo` gained `SustainHold` / `Released` flags (guards against double
   damping when a key is re-struck or AllNotesOff fires); `OnSustain` override.
4. `intrasynth/src/Intra/Synth/MidiSynth.cpp`:
   - `OnNoteOff` with pedal down marks the voice `SustainHold` and leaves it in
     `mPlayingNoteMap` (a re-strike damps it through `OnNoteOn`, like a real
     piano).
   - `OnSustain(channel, false)` releases all pedal-held voices of the channel.
   - Live `SendMidiEvent` handles CC64 too.
   - Re-strike / AllNotesOff paths are now idempotent (`Released` guard).

## Verification

- Direct pedal test (C2, NoteOff at 0.5 s): with pedal the note keeps ringing
  (−39…−42 dB through 3–5 s, ends at sample end); without pedal it drops to
  −60 dB by 2–3 s. Pedal demonstrably holds notes.
- Chopin 0–8 s render: RMS stays flat at −25…−26 dB in the 3–6 s window (no
  cutoff); full render 290.5 s in 6.94 s (41.9× realtime), tail decays to
  −101 dB at the end — no stuck voices, pedal lift at end works.
- `web/generated` and `dist` rebuilt via `scripts/build-wasm.sh` +
  `scripts/build-web.js`.

## Next steps

- User listens to the updated build; adjust pedal behavior (e.g. release
  timing) if anything sounds off.
- The D5–E5 timbre issue remains open (tracked in
  `20260820-PianoAttackTimbreFix.md`).

## Session Log

### Session 1 (2026-08-25)

- Diagnosed: sustain pedal (CC64) ignored in file and live paths; notes damped
  at NoteOff even under pedal.
- Implemented end-to-end sustain support (see Fix).
- Rebuilt WASM + dist; verified pedal behavior, Chopin 3–6 s window, full
  render speed, and clean tail.
