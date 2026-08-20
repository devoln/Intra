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
	Array<WaveInstrument> Waves;
	Array<WaveTableInstrument> WaveTables;
	WhiteNoiseInstrument WhiteNoise;
	Array<GenericInstrument> GenericInstruments;

	ExponentAttenuatorFactory ExponentAttenuation;
	EnvelopeFactory Envelope;
	ChorusFactory Chorus;
	Array<GenericModifierFactory> GenericModifiers;

	void MoveConstruct(void* dst) override {new(dst) MusicalInstrument(Move(*this));}

	Sampler& CreateSampler(float freq, float volume, unsigned sampleRate,
		SamplerContainer& dst, uint16* oIndex = nullptr) const override;

private:
	NoteSampler BuildNoteSampler(float freq, float volume, unsigned sampleRate) const;
};

INTRA_WARNING_POP
