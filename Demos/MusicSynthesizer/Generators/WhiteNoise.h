#pragma once

#include <Cpp/Warnings.h>
#include <Cpp/Features.h>
#include <Random/FastUniformNoise.h>

namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct WhiteNoise
{
	WhiteNoise(): mAmplitude(0), mDS(0), mS(0) {}

	void SetParams(float frequency, float amplitude, double step)
	{
		mAmplitude = amplitude;
		mDS = float(frequency*step);
	}

	INTRA_FORCEINLINE void PopFirst() {mS += mDS;}
	INTRA_FORCEINLINE float First() const {return mAmplitude*Random::FastUniformNoise::Linear(mS);}
	INTRA_FORCEINLINE bool Empty() const {return false;}

private:
	float mAmplitude;
	float mDS;
	float mS;
};

INTRA_WARNING_POP

}
