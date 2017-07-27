#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"

#include "Math/Math.h"

#include "FastUniform.h"

namespace Intra { namespace Random {
	
struct FastUniformNoise
{
private:
	struct Data
	{
		Data(): data(new float[16384])
		{
			FastUniform<float> fRandom(3259417823U);
			for(size_t i=0; i<16384; i++) data[i] = fRandom.SignedNext();
		}
		
		~Data() {delete[] data;}

		float* data;
	};

	static forceinline float get(size_t index) {static Data data; return data.data[index & 16383];}

public:
	static forceinline float Discrete(float t)
	{return get(size_t(t + 0.5f));}

	static forceinline float Linear(float t)
	{
		const size_t n = size_t(t);
		return Math::LinearMix(get(n), get(n+1), Math::Fract(t));
	}

	static forceinline float Cubic(float t)
	{
		const size_t n = size_t(t);
		const float x = Math::Fract(t), x2 = x*x;
		return 0.5f*( ( ( (2.0f-x)*x2 - x ) ) * get(n+16383) + 
			(x2*(3.0f*x-5.0f) + 2.0f) * get(n) + 
			((4.0f-3.0f*x)*x2 + x) * get(n+1) + 
			(x-1.0f)*x2*get(n+2));
	}
};
	
}}
