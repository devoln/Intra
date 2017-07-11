#include "WhiteNoiseSampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

Span<float> WhiteNoiseSampler::operator()(Span<float> inOutSamples, bool add)
{
	if(add) for(float& sample: inOutSamples)
	{
		sample += mAmplitude*Random::FastUniformNoise::Linear(mT);
		mT += mDT;
	}
	else for(float& sample: inOutSamples)
	{
		sample = mAmplitude*Random::FastUniformNoise::Linear(mT);
		mT += mDT;
	}
}

}}}

INTRA_WARNING_POP
