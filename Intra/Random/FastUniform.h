#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/PlatformDetect.h"
#include "Utils/Debug.h"
#include "Utils/Unique.h"

namespace Intra { namespace Random {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct FastUniform;

template<> struct FastUniform<byte>
{
	FastUniform(uint seed=157898685): Seed(seed) {}

	byte First() const {return byte(Seed >> 24);}
	

	//! Возвращает псевдослучайное целое число в диапазоне [0; 255].
	forceinline byte operator()()
	{return byte((Seed*=16807) >> 24);}

	//! Возвращает псевдослучайное число в диапазоне [0; rangeEnd).
	forceinline byte operator()(byte rangeEnd)
	{
		INTRA_DEBUG_ASSERT(0 < rangeEnd);
		return byte(operator()() % rangeEnd);
	}

	//! Возвращает псевдослучайное число в диапазоне [rangeBegin; rangeEnd).
	forceinline byte operator()(byte rangeBegin, byte rangeEnd)
	{
		INTRA_DEBUG_ASSERT(rangeBegin < rangeEnd);
		return byte(rangeBegin + operator()(byte(rangeEnd-rangeBegin)));
	}

	uint Seed;
};

template<> struct FastUniform<sbyte>: FastUniform<byte>
{
	FastUniform(uint seed=157898685): FastUniform<byte>(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; 65535].
	forceinline sbyte operator()()
	{return sbyte(FastUniform<byte>::operator()());}

	//! Возвращает псевдослучайное число в диапазоне [0; rangeEnd).
	forceinline sbyte operator()(sbyte rangeEnd)
	{
		INTRA_DEBUG_ASSERT(0 < rangeEnd);
		return sbyte(FastUniform<byte>::operator()() % rangeEnd);
	}

	//! Возвращает псевдослучайное число в диапазоне [rangeBegin; rangeEnd)
	forceinline sbyte operator()(sbyte rangeBegin, sbyte rangeEnd)
	{
		INTRA_DEBUG_ASSERT(rangeBegin < rangeEnd);
		return sbyte(rangeBegin + sbyte(FastUniform<byte>::operator()(byte(rangeEnd-rangeBegin))));
	}
};

template<> struct FastUniform<ushort>
{
	forceinline FastUniform(uint seed=157898685): Seed(seed) {}

	//! Возвращает следующее псевдослучайное целое число в диапазоне [0; 65535].
	forceinline ushort operator()() {return ushort((Seed *= 16807) >> 16);}

	//! Возвращает псевдослучайное число в диапазоне [0; rangeEnd)
	forceinline ushort operator()(ushort rangeEnd)
	{
		INTRA_DEBUG_ASSERT(0 < rangeEnd);
		return ushort(operator()() % rangeEnd);
	}

	//! Возвращает псевдослучайное число в диапазоне [rangeBegin; rangeEnd)
	forceinline ushort operator()(ushort rangeBegin, ushort rangeEnd)
	{
		INTRA_DEBUG_ASSERT(rangeBegin < rangeEnd);
		return ushort(rangeBegin + operator()(ushort(rangeEnd-rangeBegin)));
	}

	uint Seed;
};

template<> struct FastUniform<short>: FastUniform<ushort>
{
	FastUniform(uint seed=157898685): FastUniform<ushort>(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; 65535]
	forceinline short operator()()
	{return short(FastUniform<ushort>::operator()());}

	//! Возвращает псевдослучайное число в диапазоне [0; rangeEnd)
	forceinline short operator()(short rangeEnd)
	{
		INTRA_DEBUG_ASSERT(0 < rangeEnd);
		return short(FastUniform<ushort>::operator()() % rangeEnd);
	}

	//! Возвращает псевдослучайное число в диапазоне [rangeBegin; rangeEnd)
	forceinline short operator()(short rangeBegin, short rangeEnd)
	{
		INTRA_DEBUG_ASSERT(rangeBegin < rangeEnd);
		return short(rangeBegin + short(FastUniform<ushort>::operator()(ushort(rangeEnd-rangeBegin))));
	}
};

template<> struct FastUniform<uint>
{
	FastUniform(uint seed=157898685): Seed(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; UINT_MAX]
	forceinline uint operator()()
	{
		const uint lowPart = (Seed *= 16807) >> 16;
		const uint highPart = (Seed *= 16807) >> 16;
		return lowPart|(highPart << 16);
	}

	//! Возвращает псевдослучайное число в диапазоне [0; rangeEnd)
	forceinline uint operator()(uint rangeEnd)
	{
		INTRA_DEBUG_ASSERT(0 < rangeEnd);
		return operator()() % rangeEnd;
	}

	//! Возвращает псевдослучайное число в диапазоне [rangeBegin; rangeEnd)
	forceinline uint operator()(uint rangeBegin, uint rangeEnd)
	{
		INTRA_DEBUG_ASSERT(rangeBegin < rangeEnd);
		return rangeBegin + operator()(rangeEnd-rangeBegin);
	}

	uint Seed;
};

template<> struct FastUniform<int>: FastUniform<uint>
{
	FastUniform(uint seed=157898685): FastUniform<uint>(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; 65535]
	forceinline int operator()()
	{
		return int(FastUniform<uint>::operator()());
	}

	//! Возвращает псевдослучайное число в диапазоне [0; rangeEnd)
	forceinline int operator()(int rangeEnd)
	{
		INTRA_DEBUG_ASSERT(0 < rangeEnd);
		return operator()() % rangeEnd;
	}

	//! Возвращает псевдослучайное число в диапазоне [rangeBegin; rangeEnd)
	forceinline int operator()(int rangeBegin, int rangeEnd)
	{
		INTRA_DEBUG_ASSERT(rangeBegin < rangeEnd);
		return rangeBegin + operator()(rangeEnd-rangeBegin);
	}
};

template<> struct FastUniform<ulong64>: FastUniform<uint>
{
	FastUniform(uint seed=157898685): FastUniform<uint>(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; UINT_MAX]
	forceinline ulong64 operator()()
	{
		const uint lowPart = FastUniform<uint>::operator()();
		const uint highPart = FastUniform<uint>::operator()();
		return ulong64(lowPart)|(ulong64(highPart) << 32);
	}

	//! Возвращает псевдослучайное число в диапазоне [0; rangeEnd)
	forceinline ulong64 operator()(ulong64 rangeEnd)
	{
		INTRA_DEBUG_ASSERT(0 < rangeEnd);
		return operator()() % rangeEnd;
	}

	//! Возвращает псевдослучайное число в диапазоне [rangeBegin; rangeEnd)
	forceinline ulong64 operator()(ulong64 rangeBegin, ulong64 rangeEnd)
	{
		INTRA_DEBUG_ASSERT(rangeBegin < rangeEnd);
		return rangeBegin + operator()(rangeEnd-rangeBegin);
	}
};

template<> struct FastUniform<long64>: FastUniform<ulong64>
{
	FastUniform(uint seed=157898685): FastUniform<ulong64>(seed) {}

	//! Возвращает псевдослучайное целое число в диапазоне [0; 65535]
	forceinline long64 operator()()
	{return long64(FastUniform<ulong64>::operator()());}

	//! Возвращает псевдослучайное число в диапазоне [0; rangeEnd)
	forceinline long64 operator()(long64 rangeEnd)
	{
		INTRA_DEBUG_ASSERT(0 < rangeEnd);
		return operator()() % rangeEnd;
	}

	//! Возвращает псевдослучайное число в диапазоне [rangeBegin; rangeEnd).
	forceinline long64 operator()(long64 rangeBegin, long64 rangeEnd)
	{
		INTRA_DEBUG_ASSERT(rangeBegin < rangeEnd);
		return rangeBegin + operator()(rangeEnd-rangeBegin);
	}
};

#if(INTRA_SIZEOF_LONG == 4)
template<> struct FastUniform<long>: FastUniform<int>
{FastUniform(uint seed=157898685): FastUniform<int>(seed) {}};

template<> struct FastUniform<unsigned long>: FastUniform<uint>
{FastUniform(uint seed=157898685): FastUniform<uint>(seed) {}};
#else
template<> struct FastUniform<long>: FastUniform<long64>
{FastUniform(uint seed=157898685): FastUniform<long64>(seed) {}};

template<> struct FastUniform<unsigned long>: FastUniform<ulong64>
{FastUniform(uint seed=157898685): FastUniform<ulong64>(seed) {}};
#endif


template<> struct FastUniform<float>
{
	FastUniform(uint seed=157898685): Seed(seed) {}

	//! Возвращает псевдослучайное число в диапазоне [0; 1]
	forceinline float operator()()
	{
		Seed *= 16807;
		return float(int(Seed)) * 2.32830645e-10f + 0.5f;
	}

	//! Возвращает псевдослучайное число в диапазоне [0; maxOrMin], maxOrMin >= 0, или [maxOrMin, 0], maxOrMin<0
	forceinline float operator()(float maxOrMin)
	{
		return operator()() * maxOrMin;
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max]
	forceinline float operator()(float min, float max)
	{
		INTRA_DEBUG_ASSERT(min < max);
		return min + operator()(max-min);
	}

	//! Возвращает псевдослучайное число в диапазоне [-1; 1]
	float SignedNext()
	{
		Seed *= 16807;
		return float(int(Seed)) * 4.6566129e-10f;
	}

	uint Seed;

	static FastUniform Global;
};


template<> struct FastUniform<double>
{
	FastUniform(uint seed=157898685): Seed(seed) {}

	//! Возвращает псевдослучайное число в диапазоне [0; 1]
	forceinline double operator()()
	{
		Seed *= 16807;
		return double(int(Seed)) * 2.32830645e-10 + 0.5;
	}

	//! Возвращает псевдослучайное число в диапазоне [0; maxOrMin], maxOrMin>=0, или [maxOrMin, 0], maxOrMin<0
	forceinline double operator()(double maxOrMin)
	{
		return operator()() * maxOrMin;
	}

	//! Возвращает псевдослучайное число в диапазоне [min; max]
	forceinline double operator()(double min, double max)
	{
		INTRA_DEBUG_ASSERT(min < max);
		return min + operator()(max-min);
	}

	//! Возвращает псевдослучайное число в диапазоне [-1; 1]
	double SignedNext()
	{
		Seed *= 16807;
		return double(int(Seed)) * 4.6566129e-10;
	}

	uint Seed;

	static FastUniform Global;
};

INTRA_WARNING_POP

}}
