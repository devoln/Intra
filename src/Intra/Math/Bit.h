#pragma once

#include "Intra/Type.h"

INTRA_BEGIN
template<typename T> constexpr Requires<
	CScalar<T>,
unsigned> BitsOf(T value)
{
	return unsigned(value);
}

template<typename T> constexpr Requires<
	CScalar<T>,
unsigned> BitsOf(T value, unsigned offset)
{
	return unsigned(value) << offset;
}


template<typename T> Requires<
	CUnsignedIntegral<T>,
unsigned> Count1Bits(T mask)
{
	unsigned bitCount = 0;
	while(mask != 0) mask &= mask-1, bitCount++;
	return bitCount;
}

template<typename T> INTRA_FORCEINLINE Requires<
	CUnsignedIntegral<T>,
unsigned> FindBitPosition(T mask)
{
	return Count1Bits((mask&(~mask+1))-1);
}

INTRA_FORCEINLINE unsigned BitCountToMask(unsigned bitCount)
{
	return (bitCount == 32)? 0xFFFFFFFFu: (1u << bitCount)-1u;
}
INTRA_END
