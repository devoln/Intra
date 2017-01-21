#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Math/MathRanges.h"

namespace Intra { namespace Audio { namespace Synth { namespace Modifiers {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct RelPulsator
{
	RelPulsator(float koeff):
		mK(koeff), mOscillator() {}

	void SetParams(float baseFrequency, double step)
	{
		const float deltaPhase = float(2*Math::PI*baseFrequency*mK*step);
		mOscillator = Math::SineRange<float>(1, 0, deltaPhase);
	}

	forceinline float NextSample(float sample)
	{
		const float result = sample * mOscillator.First();
		mOscillator.PopFirst();
		return result;
	}
	
private:
	float mK;
	Math::SineRange<float> mOscillator;
};

INTRA_WARNING_POP

}}}}
