#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Math/MathRanges.h"

namespace Intra { namespace Audio { namespace Synth { namespace Modifiers {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct AddPulsator
{
	AddPulsator(float frequency, float basicVolume, float ampl):
		mOscillator(), mFreq(frequency),
		mBasicVolume(basicVolume), mAmplitude(ampl) {}

	void SetParams(float, double step)
	{mOscillator = Math::SineRange<float>(mAmplitude, 0, float(2*Math::PI*mFreq*step));}

	forceinline float NextSample(float sample)
	{
		const float result = sample*(mBasicVolume + mOscillator.First());
		mOscillator.PopFirst();
		return result;
	}

private:
	Math::SineRange<float> mOscillator;
	float mFreq, mBasicVolume;
	float mAmplitude;
};

INTRA_WARNING_POP

}}}}
