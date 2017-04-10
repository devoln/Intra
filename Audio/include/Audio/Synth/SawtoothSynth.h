#pragma once

#include "Types.h"
#include "Range/ForwardDecls.h"

namespace Intra { namespace Audio { namespace Synth {

void PerfectSawtooth(double upPercent, float volume,
	float freq, uint sampleRate, Span<float> inOutSamples, bool add);

void FastSawtooth(double upPercent, float volume, float freq,
	uint sampleRate, Span<float> inOutSamples, bool add);

SynthPass CreateSawtoothSynthPass(float updownRatio=1,
	float scale=1, ushort harmonics=1, float freqMultiplyer=1);

}}}
