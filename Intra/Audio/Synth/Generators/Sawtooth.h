#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Math/Math.h"


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

struct Sawtooth
{
	enum: bool {RangeIsInfinite = true};

	Sawtooth(float updownRatio, float freq, float amplitude, uint sampleRate):
		mUpdownValue(updownRatio / (updownRatio + 1)), mFreq(freq),
		mP(mUpdownValue/2), mDP(mFreq / sampleRate),
		mC1(2*amplitude/mUpdownValue), mC2(2*amplitude / (1 - mUpdownValue)), mAmplitude(amplitude)
	{}

	forceinline void PopFirst() {mP += mDP;}

	float First() const
	{
		const float sawPos = float(Math::Fract(mP));
		return sawPos < mUpdownValue?
			sawPos*mC1 - mAmplitude:
			mAmplitude - (sawPos - mUpdownValue)*mC2;
	}

	forceinline bool Empty() const {return false;}

private:
	float mUpdownValue, mFreq;
	double mP, mDP;
	float mC1, mC2, mAmplitude;
};

}}}}

INTRA_WARNING_POP
