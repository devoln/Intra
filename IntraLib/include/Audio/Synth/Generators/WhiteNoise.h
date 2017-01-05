#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Math/Random.h"

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct WhiteNoise
{
	WhiteNoise(): mAmplitude(0), mDS(0), mS(0) {}

	void SetParams(float frequency, float amplitude, double step)
	{
		mAmplitude = amplitude;
		mDS = float(frequency*step);
	}

	forceinline float NextSample() {PopFirst(); return First();}
	forceinline void PopFirst() {mS += mDS;}
	forceinline float First() const {return mAmplitude*Math::RandomNoise::Linear(mS);}
	bool Empty() const {return false;}

private:
	float mAmplitude;
	float mDS;
	float mS;
};

INTRA_WARNING_POP

}}}}
