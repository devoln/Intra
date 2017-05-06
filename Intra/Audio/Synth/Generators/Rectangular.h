#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct Rectangular
{
	Rectangular(): mTwoFreqs(0), mAmplitude(0), mT(0), mDT(0) {}

	void SetParams(float frequency, float ampl, double step)
	{
		mTwoFreqs = frequency*2;
		mAmplitude = ampl;
		mDT = float(step);
	}

	forceinline float NextSample() {PopFirst(); return First();}
	void PopFirst() {mT += mDT;}
	float First() const {return (float(int(mT*mTwoFreqs)%2)*2.0f - 1.0f)*mAmplitude;}
	bool Empty() const {return false;}

private:
	float mTwoFreqs, mAmplitude;
	float mT, mDT;
};

INTRA_WARNING_POP

}}}}
