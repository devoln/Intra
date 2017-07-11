#pragma once

#include "Cpp/Warnings.h"
#include "Utils/Span.h"
#include "Container/Sequential/Array.h"

#include "Types.h"
#include "WaveTableSampler.h"
#include "WhiteNoiseSampler.h"
#include "ExponentialAttenuation.h"
#include "AttackDecayAttenuation.h"
#include "ADSR.h"
#include "NoteSampler.h"
#include "Chorus.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

struct MusicalInstrument
{
	Array<WaveInstrument> Waves;
	Array<WaveTableInstrument> WaveTables;
	WhiteNoiseInstrument WhiteNoise;
	Array<GenericInstrument> GenericInstruments;

	ExponentAttenuatorFactory ExponentAttenuation;
	AdsrAttenuatorFactory ADSR;
	ChorusFactory Chorus;
	Array<GenericModifierFactory> GenericModifiers;

	NoteSampler operator()(float freq, float volume, uint sampleRate, size_t sampleCount) const;
};

}}}

INTRA_WARNING_POP
