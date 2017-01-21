#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Math/MathEx.h"

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct Sawtooth
{
	Sawtooth(float updownRatio):
		mP(0), mDP(0), mFreq(0),
		mUpdownValue(updownRatio/(updownRatio+1)),
		mC1(0), mC2(0), mAmplitude(0) {}

	void SetParams(float newFreq, float newAmplitude, double step)
	{
		mFreq = newFreq;
		mAmplitude = newAmplitude;
		mC1 = 2.0f*mAmplitude/mUpdownValue;
		mC2 = 2.0f*mAmplitude/(1.0f-mUpdownValue);
		mP = mUpdownValue*0.5f;
		mDP = (step*mFreq);
	}

	forceinline float NextSample() {PopFirst(); return First();}

	forceinline void PopFirst() {mP += mDP;}

	float First() const
	{
		float sawPos = float(Math::Fract(mP));
		return sawPos<mUpdownValue?
			sawPos*mC1-mAmplitude:
			mAmplitude-(sawPos-mUpdownValue)*mC2;
	}

	forceinline bool Empty() const {return false;}

private:
	double mP, mDP;
	float mFreq;
	float mUpdownValue, mC1, mC2, mAmplitude;
};

INTRA_WARNING_POP

}}}}
