#include "WhiteNoiseSampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

size_t WhiteNoiseSampler::GenerateMono(Span<float> precomputedSamples)
{
	size_t samplesToProcess = precomputedSamples.Length();
	while(!precomputedSamples.Empty())
	{
		precomputedSamples.Next() += mAmplitude*Random::FastUniformNoise::Linear(mT);
		mT += mDT;
	}
	return samplesToProcess;
}

// Честный стерео-рендер (Update 18): тот же шум с тем же уровнем, что и в
// моно-режиме (0.5/0.5 на канал, суммарная мощность как раньше), чтобы
// громкость не прыгала между нотами с модификаторами и без них.
size_t WhiteNoiseSampler::GenerateStereo(Span<float> inOutLeft, Span<float> inOutRight)
{
	const size_t n = Math::Min(inOutLeft.Length(), inOutRight.Length());
	for(size_t i = 0; i < n; i++)
	{
		const float s = mAmplitude*Random::FastUniformNoise::Linear(mT)*0.5f;
		inOutLeft[i] += s;
		inOutRight[i] += s;
	}
	mT += mDT*n;
	return n;
}

INTRA_WARNING_POP
