#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Math/SineRange.h"
#include "Math/Math.h"

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class AbsModSine
{
	float mModOmega, mK, mAmplitude;
	float mPhi, mDPhi;
	Math::SineRange<float> mOscillator;
public:
	AbsModSine(float koeff, float modFrequency):
		mModOmega(2*float(Math::PI)*modFrequency), mK(koeff),
		mAmplitude(0), mPhi(0), mDPhi(0), mOscillator(0,0,0) {}

	void SetParams(float frequency, float ampl, double step)
	{
		mAmplitude = ampl;
		mDPhi = float(2*Math::PI*frequency*step);
		mOscillator = Math::SineRange<float>(mK, 0, float(mModOmega*step));
		mPhi = 0;
	}

	forceinline float NextSample()
	{
		PopFirst();
		return First();
	}

	forceinline void PopFirst()
	{
		mOscillator.PopFirst();
		mPhi += mDPhi;
	}

	forceinline float First() const
	{return mAmplitude*Math::Sin(mPhi + mOscillator.First());}
};

INTRA_WARNING_POP

}}}}
