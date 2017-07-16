#include "WhiteNoiseSampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

Span<float> WhiteNoiseSampler::operator()(Span<float> inOutSamples, bool add)
{
	if(add) while(!inOutSamples.Empty())
	{
		inOutSamples.Next() += mAmplitude*Random::FastUniformNoise::Linear(mT);
		mT += mDT;
	}
	else while(!inOutSamples.Empty())
	{
		inOutSamples.Next() = mAmplitude*Random::FastUniformNoise::Linear(mT);
		mT += mDT;
	}
	return inOutSamples;
}

}}}

INTRA_WARNING_POP
