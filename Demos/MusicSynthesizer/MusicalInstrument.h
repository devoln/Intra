#pragma once


#include "Intra/Range/Span.h"
#include "IntraX/Container/Sequential/Array.h"

#include "Types.h"
#include "WaveTableSampler.h"
#include "WhiteNoiseSampler.h"
#include "ExponentialAttenuation.h"
#include "Chorus.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct MusicalInstrument
{
	Array<WaveInstrument> Waves;
	Array<WaveTableInstrument> WaveTables;
	WhiteNoiseInstrument WhiteNoise;
	Array<GenericInstrument> GenericInstruments;

	ExponentAttenuatorFactory ExponentAttenuation;
	EnvelopeFactory Envelope;
	ChorusFactory Chorus;
	Array<GenericModifierFactory> GenericModifiers;

	Sampler& CreateSampler(void* addrToConstructSampler, float freq, float volume, unsigned sampleRate) const override;
};

INTRA_WARNING_POP
