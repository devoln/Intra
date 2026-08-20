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

	INTRA_FORCEINLINE float First() const {return operator()(Time);}
	INTRA_FORCEINLINE void PopFirst() {Time += DT;}

	INTRA_FORCEINLINE float Next()
	{
		const float result = First();
		PopFirst();
		return result;
	}

	INTRA_FORCEINLINE float operator()(float t)
	{return Generator(Freq, t)*Amplitude;}
};

template<typename T> INTRA_FORCEINLINE FunctionGenerator<T> CreateFunctionGenerator(T generator)
{return FunctionGenerator<T>(generator);}

INTRA_WARNING_POP

}
