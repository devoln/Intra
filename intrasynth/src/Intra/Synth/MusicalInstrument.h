#pragma once

#include "Intra/Range/Span.h"
#include "Container/Sequential/Array.h"

#include "Types.h"
#include "Instrument.h"
#include "WaveFormSampler.h"
#include "WaveTableSampler.h"
#include "WhiteNoiseSampler.h"
#include "ExponentialAttenuation.h"
#include "Envelope.h"
#include "Chorus.h"
#include "NoteSampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct MusicalInstrument: public Instrument
{
	// Static instrument loudness calibration. Applied once after all voice
	// generators have been constructed, so it cannot affect random seeds,
	// velocity-dependent timbre or other initialization state.
	float VolumeScale = 1.0f;
#ifdef INTRA_RUNTIME_VELOCITY_MODULATORS
	// Optional bank-defined velocity modulator. Absent from the forced Titanic build.
	VelocityModulator Velocity;
#endif
	Array<WaveInstrument> Waves;
	Array<WaveTableInstrument> WaveTables;
	WhiteNoiseInstrument WhiteNoise;
	Array<GenericInstrument> GenericInstruments;
	DynamicGenericInstrument DynamicGeneric;

	ExponentAttenuatorFactory ExponentAttenuation;
	EnvelopeFactory Envelope;
	ChorusFactory Chorus;
	Array<GenericModifierFactory> GenericModifiers;

	void MoveConstruct(void* dst) override {new(dst) MusicalInstrument(Move(*this));}

#ifdef INTRA_RUNTIME_VELOCITY_MODULATORS
	NoteOnParams ResolveNoteOnParams(byte velocity) const override
	{
		return ResolveVelocityNoteOnParams(Velocity, velocity);
	}
#endif

	Sampler& CreateSampler(float freq, float volume, unsigned sampleRate,
		const NoteOnParams& noteParams, SamplerContainer& dst, uint16* oIndex = nullptr) const override;

	void PreloadTables(unsigned sampleRate) override;
	void PreloadKey(float freq, unsigned sampleRate) override;

private:
	NoteSampler BuildNoteSampler(float freq, float volume, unsigned sampleRate,
		const NoteOnParams& noteParams) const;
};

INTRA_WARNING_POP
