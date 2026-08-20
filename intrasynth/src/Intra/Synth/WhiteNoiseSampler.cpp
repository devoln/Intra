#include "WhiteNoiseSampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

size_t WhiteNoiseSampler::GenerateMono(Span<float> inOutSamples)
{
	size_t samplesToProcess = inOutSamples.Length();
	while(!inOutSamples.Empty())
	{
		inOutSamples.Next() += mAmplitude*Random::FastUniformNoise::Linear(mT);
		mT += mDT;
	}
	return samplesToProcess;
}

INTRA_WARNING_POP
