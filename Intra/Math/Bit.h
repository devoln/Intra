#pragma once

#include "Core/Core.h"
#include "Core/Type.h"

INTRA_BEGIN
inline namespace Math {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> constexpr forceinline Requires<
	CScalar<T>,
uint> BitsOf(T value)
{
	return uint(value);
}

template<typename T> constexpr forceinline Requires<
	CScalar<T>,
uint> BitsOf(T value, uint offset)
{
	return uint(value) << offset;
}


template<typename T> Requires<
	CUnsignedIntegral<T>,
uint> Count1Bits(T mask)
{
	uint bitCount = 0;
	while(mask != 0) mask &= mask-1, bitCount++;
	return bitCount;
}

template<typename T> forceinline Requires<
	CUnsignedIntegral<T>,
uint> FindBitPosition(T mask)
{
	return Count1Bits((mask&(~mask+1))-1);
}

forceinline uint BitCountToMask(uint bitCount)
{
	return (bitCount == 32)? 0xFFFFFFFFu: (1u << bitCount)-1u;
}

}
INTRA_END
