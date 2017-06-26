#pragma once

#include "Math/FixedPoint.h"
#include "Types.h"
#include "Utils/Span.h"

namespace Intra { namespace Audio { namespace Synth {

struct SineHarmonic
{
	FixedPoint<byte, 512> Scale;
	FixedPoint<byte, 16> FreqMultiplyer;
};

void PerfectSine(float volume, float freq, uint sampleRate, Span<float> inOutSamples, bool add);
void FastSine(float volume, float freq, uint sampleRate, Span<float> inOutSamples, bool add);

SynthPass CreateSineSynthPass(float scale=1, ushort harmonics=1, float freqMultiplyer=1);
SynthPass CreateMultiSineSynthPass(CSpan<SineHarmonic> harmonics);

}}}
