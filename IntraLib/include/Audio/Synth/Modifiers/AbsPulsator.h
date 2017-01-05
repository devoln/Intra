#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Math/MathRanges.h"

namespace Intra { namespace Audio { namespace Synth { namespace Modifiers {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct AbsPulsator
{
	AbsPulsator() = default;
	AbsPulsator(float frequency, float baseAmplitude=0, float sinAmplitude=1):
		mOscillator(), mFreq(frequency),
		mBaseAmplitude(baseAmplitude),
		mSinAmplitude(sinAmplitude) {}

	void SetParams(float, double step)
	{mOscillator = Math::SineRange<float>(mSinAmplitude, 0, float(2*Math::PI*mFreq*step));}

	forceinline float NextSample(float sample)
	{
		const float result = sample*(mBaseAmplitude+mSinAmplitude * mOscillator.First());
		mOscillator.PopFirst();
		return result;
	}

private:
	Math::SineRange<float> mOscillator;
	float mFreq;
	float mBaseAmplitude, mSinAmplitude;
};

INTRA_WARNING_POP

}}}}
