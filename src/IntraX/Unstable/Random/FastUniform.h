#pragma once

#include "Intra/Numeric.h"
#include "IntraX/System/Debug.h"
#include "IntraX/Utils/Unique.h"

namespace Intra { INTRA_BEGIN
template<typename T> struct FastUniform;

template<> struct FastUniform<byte>
{
	FastUniform(unsigned seed = 157898685): Seed(seed) {}

	byte First() const {return byte(Seed >> 24);}

	/// @return pseudo-random number in range [0; 255].
	INTRA_FORCEINLINE byte operator()()
	{return byte((Seed *= 16807) >> 24);}

	/// @return pseudo-random number in range [0; rangeEnd).
	INTRA_FORCEINLINE byte operator()(byte rangeEnd)
	{
		INTRA_PRECONDITION(0 < rangeEnd);
		return byte(operator()() % rangeEnd);
	}

	/// @return pseudo-random number in range [rangeBegin; rangeEnd).
	INTRA_FORCEINLINE byte operator()(byte rangeBegin, byte rangeEnd)
	{
		INTRA_PRECONDITION(rangeBegin < rangeEnd);
		return byte(rangeBegin + operator()(byte(rangeEnd-rangeBegin)));
	}

	unsigned Seed;
};

template<> struct FastUniform<int8>: FastUniform<byte>
{
	FastUniform(unsigned seed=157898685): FastUniform<byte>(seed) {}

	/// @return pseudo-random number in range [0; 65535].
	INTRA_FORCEINLINE int8 operator()()
	{return int8(FastUniform<byte>::operator()());}

	/// @return pseudo-random number in range [0; rangeEnd).
	INTRA_FORCEINLINE int8 operator()(int8 rangeEnd)
	{
		INTRA_PRECONDITION(0 < rangeEnd);
		return int8(FastUniform<byte>::operator()() % rangeEnd);
	}

	/// @return pseudo-random number in range [rangeBegin; rangeEnd)
	INTRA_FORCEINLINE int8 operator()(int8 rangeBegin, int8 rangeEnd)
	{
		INTRA_PRECONDITION(rangeBegin < rangeEnd);
		return int8(rangeBegin + int8(FastUniform<byte>::operator()(byte(rangeEnd-rangeBegin))));
	}
};

template<> struct FastUniform<uint16>
{
	INTRA_FORCEINLINE FastUniform(unsigned seed = 157898685): Seed(seed) {}

	/// @return pseudo-random number in range [0; 65535].
	INTRA_FORCEINLINE uint16 operator()() {return uint16((Seed *= 16807) >> 16);}

	/// @return pseudo-random number in range [0; rangeEnd)
	INTRA_FORCEINLINE uint16 operator()(uint16 rangeEnd)
	{
		INTRA_PRECONDITION(0 < rangeEnd);
		return uint16(operator()() % rangeEnd);
	}

	/// @return pseudo-random number in range [rangeBegin; rangeEnd)
	INTRA_FORCEINLINE uint16 operator()(uint16 rangeBegin, uint16 rangeEnd)
	{
		INTRA_PRECONDITION(rangeBegin < rangeEnd);
		return uint16(rangeBegin + operator()(uint16(rangeEnd-rangeBegin)));
	}

	unsigned Seed;
};

template<> struct FastUniform<short>: FastUniform<uint16>
{
	FastUniform(unsigned seed=157898685): FastUniform<uint16>(seed) {}

	/// @return pseudo-random number in range [0; 65535]
	INTRA_FORCEINLINE short operator()()
	{return short(FastUniform<uint16>::operator()());}

	/// @return pseudo-random number in range [0; rangeEnd)
	INTRA_FORCEINLINE short operator()(short rangeEnd)
	{
		INTRA_PRECONDITION(0 < rangeEnd);
		return short(FastUniform<uint16>::operator()() % rangeEnd);
	}

	/// @return pseudo-random number in range [rangeBegin; rangeEnd)
	INTRA_FORCEINLINE short operator()(short rangeBegin, short rangeEnd)
	{
		INTRA_PRECONDITION(rangeBegin < rangeEnd);
		return short(rangeBegin + short(FastUniform<uint16>::operator()(uint16(rangeEnd-rangeBegin))));
	}
};

template<> struct FastUniform<uint32>
{
	FastUniform(uint32 seed = 157898685): Seed(seed) {}

	/// @return pseudo-random number in range [0; UINT_MAX]
	INTRA_FORCEINLINE uint32 operator()()
	{
		const uint32 lowPart = (Seed *= 16807) >> 16;
		const uint32 highPart = (Seed *= 16807) >> 16;
		return lowPart|(highPart << 16);
	}

	/// @return pseudo-random number in range [0; rangeEnd)
	INTRA_FORCEINLINE uint32 operator()(uint32 rangeEnd)
	{
		INTRA_PRECONDITION(0 < rangeEnd);
		return operator()() % rangeEnd;
	}

	/// @return pseudo-random number in range [rangeBegin; rangeEnd)
	INTRA_FORCEINLINE unsigned operator()(uint32 rangeBegin, uint32 rangeEnd)
	{
		INTRA_PRECONDITION(rangeBegin < rangeEnd);
		return rangeBegin + operator()(rangeEnd-rangeBegin);
	}

	unsigned Seed;
};

template<> struct FastUniform<int32>: FastUniform<uint32>
{
	FastUniform(unsigned seed=157898685): FastUniform<uint32>(seed) {}

	/// @return pseudo-random number in range [0; 65535]
	INTRA_FORCEINLINE int operator()()
	{
		return int(FastUniform<uint32>::operator()());
	}

	/// @return pseudo-random number in range [0; rangeEnd)
	INTRA_FORCEINLINE int operator()(int32 rangeEnd)
	{
		INTRA_PRECONDITION(0 < rangeEnd);
		return operator()() % rangeEnd;
	}

	/// @return pseudo-random number in range [rangeBegin; rangeEnd)
	INTRA_FORCEINLINE int operator()(int32 rangeBegin, int32 rangeEnd)
	{
		INTRA_PRECONDITION(rangeBegin < rangeEnd);
		return rangeBegin + operator()(rangeEnd-rangeBegin);
	}
};

template<> struct FastUniform<uint64>: FastUniform<unsigned>
{
	FastUniform(unsigned seed=157898685): FastUniform<unsigned>(seed) {}

	/// @return pseudo-random number in range [0; UINT_MAX]
	INTRA_FORCEINLINE uint64 operator()()
	{
		const unsigned lowPart = FastUniform<unsigned>::operator()();
		const unsigned highPart = FastUniform<unsigned>::operator()();
		return uint64(lowPart)|(uint64(highPart) << 32);
	}

	/// @return pseudo-random number in range [0; rangeEnd)
	INTRA_FORCEINLINE uint64 operator()(uint64 rangeEnd)
	{
		INTRA_PRECONDITION(0 < rangeEnd);
		return operator()() % rangeEnd;
	}

	/// @return pseudo-random number in range [rangeBegin; rangeEnd)
	INTRA_FORCEINLINE uint64 operator()(uint64 rangeBegin, uint64 rangeEnd)
	{
		INTRA_PRECONDITION(rangeBegin < rangeEnd);
		return rangeBegin + operator()(rangeEnd-rangeBegin);
	}
};

template<> struct FastUniform<int64>: FastUniform<uint64>
{
	FastUniform(unsigned seed=157898685): FastUniform<uint64>(seed) {}

	/// @return pseudo-random number in range [0; 65535]
	INTRA_FORCEINLINE int64 operator()()
	{return int64(FastUniform<uint64>::operator()());}

	/// @return pseudo-random number in range [0; rangeEnd)
	INTRA_FORCEINLINE int64 operator()(int64 rangeEnd)
	{
		INTRA_PRECONDITION(0 < rangeEnd);
		return operator()() % rangeEnd;
	}

	/// @return pseudo-random number in range [rangeBegin; rangeEnd).
	INTRA_FORCEINLINE int64 operator()(int64 rangeBegin, int64 rangeEnd)
	{
		INTRA_PRECONDITION(rangeBegin < rangeEnd);
		return rangeBegin + operator()(rangeEnd-rangeBegin);
	}
};

template<> struct FastUniform<long>: FastUniform<TSelect<int64, int, sizeof(long) == 8>>
{
	FastUniform(unsigned seed = 157898685):
		FastUniform<TSelect<int64, int, sizeof(long) == 8>>(seed) {}
};

template<> struct FastUniform<unsigned long>: FastUniform<TSelect<uint64, unsigned, sizeof(long) == 8>>
{
	FastUniform(unsigned seed = 157898685):
		FastUniform<TSelect<uint64, unsigned, sizeof(long) == 8>>(seed) {}
};


template<> struct FastUniform<float>
{
	FastUniform(unsigned seed = 157898685): Seed(seed) {}

	/// Возвращает псевдослучайное число в диапазоне [0; 1]
	INTRA_FORCEINLINE float operator()()
	{
		Seed *= 16807;
		return float(int(Seed)) * 2.32830645e-10f + 0.5f;
	}

	/// Возвращает псевдослучайное число в диапазоне [0; maxOrMin], maxOrMin >= 0, или [maxOrMin, 0], maxOrMin<0
	INTRA_FORCEINLINE float operator()(float maxOrMin)
	{
		return operator()() * maxOrMin;
	}

	/// Возвращает псевдослучайное число в диапазоне [min; max]
	INTRA_FORCEINLINE float operator()(float min, float max)
	{
		INTRA_PRECONDITION(min < max);
		return min + operator()(max-min);
	}

	/// Возвращает псевдослучайное число в диапазоне [-1; 1]
	float SignedNext()
	{
		Seed *= 16807;
		return float(int(Seed)) * 4.6566129e-10f;
	}

	unsigned Seed;

	static FastUniform Global;
};


template<> struct FastUniform<double>
{
	FastUniform(unsigned seed = 157898685): Seed(seed) {}

	/// Возвращает псевдослучайное число в диапазоне [0; 1]
	INTRA_FORCEINLINE double operator()()
	{
		Seed *= 16807;
		return double(int(Seed)) * 2.32830645e-10 + 0.5;
	}

	/// Возвращает псевдослучайное число в диапазоне [0; maxOrMin], maxOrMin>=0, или [maxOrMin, 0], maxOrMin<0
	INTRA_FORCEINLINE double operator()(double maxOrMin)
	{
		return operator()() * maxOrMin;
	}

	/// Возвращает псевдослучайное число в диапазоне [min; max]
	INTRA_FORCEINLINE double operator()(double min, double max)
	{
		INTRA_PRECONDITION(min < max);
		return min + operator()(max-min);
	}

	/// Возвращает псевдослучайное число в диапазоне [-1; 1]
	double SignedNext()
	{
		Seed *= 16807;
		return double(int(Seed)) * 4.6566129e-10;
	}

	unsigned Seed;

	static FastUniform Global;
};
} INTRA_END
