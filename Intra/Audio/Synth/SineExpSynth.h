#pragma once

#include "Math/FixedPoint.h"
#include "Utils/Span.h"
#include "Types.h"

namespace Intra { namespace Audio { namespace Synth {

struct SineExpHarmonic
{
	Math::FixedPoint<byte, 512> Scale;
	Math::FixedPoint<byte, 8> AttenCoeff;
	Math::FixedPoint<byte, 16> FreqMultiplyer;
	Math::Norm8s LengthMultiplyer;
};

void FastSineExp(float volume, float coeff, float freq,
	uint sampleRate, Span<float> inOutSamples, bool add);

SynthPass CreateSineExpSynthPass(CSpan<SineExpHarmonic> harmonics);

}}}
