#pragma once

#include "Core/Core.h"

namespace Intra { namespace Math {



template<typename T> struct Random;

template<> struct Random<byte>
{
	Random(uint seed=157898685): Seed(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; 255]
	forceinline byte operator()()
	{
		return (Seed*=16807) >> 24;
	}

	//! Возвращает псевдослучайное число в диапазоне [0; max)
	forceinline byte operator()(byte max)
	{
		INTRA_ASSERT(0 < max);
		return operator()() % max;
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max)
	forceinline byte operator()(byte min, byte max)
	{
		INTRA_ASSERT(min < max);
		return min + operator()(max-min);
	}

	uint Seed;

	INTRA_OPTIONAL_THREAD_LOCAL static Random Global;
};

template<> struct Random<sbyte>: Random<byte>
{
	Random(uint seed=157898685): Random<byte>(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; 65535]
	forceinline sbyte operator()()
	{
		return (sbyte)Random<byte>::operator()();
	}

	//! Возвращает псевдослучайное число в диапазоне [0; max)
	forceinline sbyte operator()(sbyte max)
	{
		INTRA_ASSERT(0 < max);
		return sbyte(Random<byte>::operator()() % max);
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max)
	forceinline sbyte operator()(sbyte min, sbyte max)
	{
		INTRA_ASSERT(min < max);
		return sbyte(min + (sbyte)Random<byte>::operator()(max-min));
	}

	uint Seed;

	INTRA_OPTIONAL_THREAD_LOCAL static Random Global;
};

template<> struct Random<ushort>
{
	Random(uint seed=157898685): Seed(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; 65535]
	forceinline ushort operator()()
	{
		return (Seed*=16807) >> 16;
	}

	//! Возвращает псевдослучайное число в диапазоне [0; max)
	forceinline ushort operator()(ushort max)
	{
		INTRA_ASSERT(0 < max);
		return operator()() % max;
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max)
	forceinline ushort operator()(ushort min, ushort max)
	{
		INTRA_ASSERT(min < max);
		return min + operator()(max-min);
	}

	uint Seed;

	INTRA_OPTIONAL_THREAD_LOCAL static Random Global;
};

template<> struct Random<short>: Random<ushort>
{
	Random(uint seed=157898685): Random<ushort>(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; 65535]
	forceinline short operator()()
	{
		return (short)Random<ushort>::operator()();
	}

	//! Возвращает псевдослучайное число в диапазоне [0; max)
	forceinline short operator()(short max)
	{
		INTRA_ASSERT(0 < max);
		return Random<ushort>::operator()() % max;
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max)
	forceinline short operator()(short min, short max)
	{
		INTRA_ASSERT(min < max);
		return min + (short)Random<ushort>::operator()(max-min);
	}

	uint Seed;

	INTRA_OPTIONAL_THREAD_LOCAL static Random Global;
};

template<> struct Random<uint>
{
	Random(uint seed=157898685): Seed(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; UINT_MAX]
	forceinline uint operator()()
	{
		const uint lowPart = (Seed*=16807) >> 16;
		const uint highPart = (Seed*=16807) >> 16;
		return lowPart|(highPart << 16);
	}

	//! Возвращает псевдослучайное число в диапазоне [0; max)
	forceinline uint operator()(uint max)
	{
		INTRA_ASSERT(0 < max);
		return operator()() % max;
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max)
	forceinline uint operator()(uint min, uint max)
	{
		INTRA_ASSERT(min < max);
		return min + operator()(max-min);
	}

	uint Seed;

	INTRA_OPTIONAL_THREAD_LOCAL static Random Global;
};

template<> struct Random<int>: Random<uint>
{
	Random(uint seed=157898685): Random<uint>(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; 65535]
	forceinline int operator()()
	{
		return (int)Random<uint>::operator()();
	}

	//! Возвращает псевдослучайное число в диапазоне [0; max)
	forceinline int operator()(int max)
	{
		INTRA_ASSERT(0 < max);
		return operator()() % max;
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max)
	forceinline int operator()(int min, int max)
	{
		INTRA_ASSERT(min < max);
		return min + operator()(max-min);
	}

	uint Seed;

	INTRA_OPTIONAL_THREAD_LOCAL static Random Global;
};

template<> struct Random<ulong64>: Random<uint>
{
	Random(uint seed=157898685): Seed(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; UINT_MAX]
	forceinline ulong64 operator()()
	{
		const uint lowPart = Random<uint>::operator()();
		const uint highPart = Random<uint>::operator()();
		return ulong64(lowPart)|(ulong64(highPart) << 32);
	}

	//! Возвращает псевдослучайное число в диапазоне [0; max)
	forceinline ulong64 operator()(ulong64 max)
	{
		INTRA_ASSERT(0 < max);
		return operator()() % max;
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max)
	forceinline ulong64 operator()(ulong64 min, ulong64 max)
	{
		INTRA_ASSERT(min < max);
		return min + operator()(max-min);
	}

	uint Seed;

	INTRA_OPTIONAL_THREAD_LOCAL static Random Global;
};

template<> struct Random<long64>: Random<ulong64>
{
	Random(uint seed=157898685): Random<ulong64>(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; 65535]
	forceinline long64 operator()()
	{
		return (long64)Random<ulong64>::operator()();
	}

	//! Возвращает псевдослучайное число в диапазоне [0; max)
	forceinline long64 operator()(long64 max)
	{
		INTRA_ASSERT(0 < max);
		return operator()() % max;
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max)
	forceinline long64 operator()(long64 min, long64 max)
	{
		INTRA_ASSERT(min < max);
		return min + operator()(max-min);
	}

	uint Seed;

	INTRA_OPTIONAL_THREAD_LOCAL static Random Global;
};

template<> struct Random<float>
{
	Random(uint seed=157898685): Seed(seed) {}

	//! Возвращает псевдослучайное число в диапазоне [0; 1]
	forceinline float operator()()
	{
		Seed *= 16807;
		return (int)Seed * 2.32830645e-10f + 0.5f;
	}

	//! Возвращает псевдослучайное число в диапазоне [0; maxOrMin], maxOrMin>=0, или [maxOrMin, 0], maxOrMin<0
	forceinline float operator()(float maxOrMin)
	{
		return operator()() * maxOrMin;
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max]
	forceinline float operator()(float min, float max)
	{
		INTRA_ASSERT(min < max);
		return min + operator()(max-min);
	}

	//! Возвращает псевдослучайное число в диапазоне [-1; 1]
	float SignedNext()
	{
		Seed *= 16807;
		return (int)Seed * 4.6566129e-10f;
	}

	uint Seed;

	INTRA_OPTIONAL_THREAD_LOCAL static Random Global;
};


template<> struct Random<double>
{
	Random(uint seed=157898685): Seed(seed) {}

	//! Возвращает псевдослучайное число в диапазоне [0; 1]
	forceinline double operator()()
	{
		Seed *= 16807;
		return (int)Seed * 2.32830645e-10 + 0.5;
	}

	//! Возвращает псевдослучайное число в диапазоне [0; maxOrMin], maxOrMin>=0, или [maxOrMin, 0], maxOrMin<0
	forceinline double operator()(double maxOrMin)
	{
		return operator()() * maxOrMin;
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max]
	forceinline double operator()(double min, double max)
	{
		INTRA_ASSERT(min < max);
		return min + operator()(max-min);
	}

	//! Возвращает псевдослучайное число в диапазоне [-1; 1]
	double SignedNext()
	{
		Seed *= 16807;
		return (int)Seed * 4.6566129e-10;
	}

	uint Seed;

	INTRA_OPTIONAL_THREAD_LOCAL static Random Global;
};




struct RandomNoise
{
	struct Data
	{
		Data()
		{
			Random<float> frandom(325242341);
			for(auto& f: discreteNoise) f = frandom();
		}

		float discreteNoise[16384];
	};

	static Data* data;

	static forceinline float Discrete(float t)
	{
		return data->discreteNoise[int(t+0.5f) & 16383];
	}

	static forceinline float Linear(float t)
	{
		int n=int(t);
		return LinearMix(data->discreteNoise[n & 16383], data->discreteNoise[(n+1) & 16383], Fract(t));
	}

	static forceinline float Cubic(float t)
	{
		int n=int(t);
		const float x = Fract(t), x2 = x*x;
		return 0.5f*( ( ( (2.0f-x)*x2 - x ) )*data->discreteNoise[(n+16383)&16383] + 
			(x2*(3.0f*x-5.0f) + 2.0f)*data->discreteNoise[n] + 
			((4.0f-3.0f*x)*x2 + x)*data->discreteNoise[(n+1)&16383] + 
			(x-1.0f)*x2*data->discreteNoise[(n+2)&16383]);
	}
};


}}

