#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> class FunctionGenerator
{
	float mFreq, mAmplitude;
	float mTime, mDT;
	T mGenerator;
public:
	FunctionGenerator(const FunctionGenerator&) = default;

	FunctionGenerator(T generator): mFreq(0), mAmplitude(0),
		mTime(0), mDT(0), mGenerator(generator) {}

	void SetParams(float frequency, float amplitude, double step)
	{
		mFreq = frequency;
		mAmplitude = amplitude;
		mDT = float(step);
	}

	forceinline float NextSample()
	{
		const float result = operator()(mTime);
		mTime += mDT;
		return result;
	}

	forceinline float operator()(float t)
	{return mGenerator(mFreq, t)*mAmplitude;}
};

template<typename T> forceinline FunctionGenerator<T> CreateFunctionGenerator(T generator)
{return FunctionGenerator<T>(generator);}

INTRA_WARNING_POP

}}}}
