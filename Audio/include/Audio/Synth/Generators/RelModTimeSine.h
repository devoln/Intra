#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Math/MathRanges.h"
#include "Math/Math.h"

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class RelModTimeSine
{
	float mModK, mK, mT0, mT1, mDT, mAmplitude;
	float mPhi, mDPhi;
	Math::SineRange<float> mOscillator;
public:
	RelModTimeSine(float modKoeff, float tstart, float koeff):
		mModK(modKoeff), mK(koeff), mT0(tstart),
		mT1(0), mDT(0), mAmplitude(0), mPhi(0), mDPhi(0), mOscillator() {}

	void SetParams(float frequency, float amplitude, double step)
	{
		mAmplitude = amplitude;
		const double omega = 2*Math::PI*frequency;
		mDPhi = float(omega*step);
		mPhi = 0;
		mT1 = mT0;
		mDT = float(step);
		mOscillator = Math::SineRange<float>(mK, 0, float(omega*mModK*step));
	}

	forceinline float NextSample() {PopFirst(); return First();}
	forceinline float First() const {return mAmplitude*Math::Sin(mPhi + mT1 * mOscillator.First());}
	forceinline void PopFirst() {mPhi += mDPhi; mT1 -= mDT;}
};

INTRA_WARNING_POP

}}}}
