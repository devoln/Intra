#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Math/SineRange.h"
#include "Math/Math.h"

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class AbsModTimeSine
{
	float mModOmega, mK, mT0, mT1, mDT, mAmplitude;
	float mPhi, mDPhi;
	Math::SineRange<float> mOscillator;
public:
	AbsModTimeSine() = default;

	AbsModTimeSine(float modFrequency, float tstart, float koeff):
		mModOmega(float(2*Math::PI*modFrequency)), mK(koeff), mT0(tstart),
		mT1(mT0), mDT(0), mAmplitude(0), mPhi(0), mDPhi(0), mOscillator(0,0,0) {}
		
	void SetParams(float frequency, float ampl, double step)
	{
		mAmplitude = ampl;
		mDPhi = float(2*Math::PI*frequency*step);
		mOscillator = Math::SineRange<float>(mK, 0, float(mModOmega*step));
		mPhi = 0;
		mT1 = mT0;
		mDT = float(step);
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
		mT1 -= mDT;
	}

	forceinline float First() const
	{return mAmplitude*Math::Sin(mPhi + mT1 * mOscillator.First());}
};

INTRA_WARNING_POP

}}}}
