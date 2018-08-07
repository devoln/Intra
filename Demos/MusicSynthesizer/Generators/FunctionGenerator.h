#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> class FunctionGenerator
{
	float Freq, Amplitude;
	float Time, DT;
	T Generator;

	FunctionGenerator(T generator, float frequency, float amplitude, float dt):
		Freq(frequency), Amplitude(amplitude),
		Time(0), DT(dt), Generator(generator) {}

	forceinline float First() const {return operator()(Time);}
	forceinline void PopFirst() {Time += DT;}

	forceinline float Next()
	{
		const float result = First();
		PopFirst();
		return result;
	}

	forceinline float operator()(float t)
	{return Generator(Freq, t)*Amplitude;}
};

template<typename T> forceinline FunctionGenerator<T> CreateFunctionGenerator(T generator)
{return FunctionGenerator<T>(generator);}

INTRA_WARNING_POP

}
