#pragma once

#include "Types.h"

namespace Intra { namespace Audio { namespace Synth {

template<typename T> AttenuationPass CreateAttenuatorPass(T attenuator)
{
	return [attenuator](float noteDuration, Span<float> inOutSamples, uint sampleRate)
	{
		auto attenuatorCopy = attenuator;
		attenuatorCopy.SetAttenuationStep(1.0f/sampleRate);
		for(auto& s: inOutSamples)
			s *= attenuatorCopy.NextSample();
		(void)noteDuration;
	};
}

}}}
