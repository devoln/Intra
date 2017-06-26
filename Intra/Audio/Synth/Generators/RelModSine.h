#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Math/SineRange.h"
#include "Math/Math.h"

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class RelModSine
{
	float mModK, mK, mAmplitude;
	float mPhi, mDPhi;
	Math::SineRange<float> mOscillator;

public:
	RelModSine(float koeff, float modKoeff):
		mModK(modKoeff), mK(koeff),
		mAmplitude(0), mPhi(0), mDPhi(0), mOscillator(0,0,0) {}

	void SetParams(float frequency, float amplitude, double step)
	{
		mAmplitude = amplitude;
		const double radFreq = 2*Math::PI*frequency;
		mDPhi = float(radFreq*step);
		mPhi = 0;
		mOscillator = Math::SineRange<float>(mK, 0, float(radFreq*mModK*step));
	}

	forceinline float NextSample()
	{
		PopFirst();
		return First();
	}

	forceinline float First() const
	{return mAmplitude*Math::Sin(mPhi + mOscillator.First());}

	forceinline void PopFirst()
	{
		mOscillator.PopFirst();
		mPhi += mDPhi;
	}
};

INTRA_WARNING_POP

}}}}
