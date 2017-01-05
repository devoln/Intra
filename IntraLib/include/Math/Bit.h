#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Meta/Type.h"

namespace Intra { namespace Math {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> constexpr forceinline Meta::EnableIf<
	Meta::IsScalarType<T>::_,
flag32> BitsOf(T value)
{
	return flag32(value);
}

template<typename T> constexpr forceinline Meta::EnableIf<
	Meta::IsScalarType<T>::_,
flag32> BitsOf(T value, uint offset)
{
	return BitsOf(value) << offset;
}


template<typename T> Meta::EnableIf<
	Meta::IsUnsignedIntegralType<T>::_,
uint> Count1Bits(T mask)
{
	uint bitCount = 0;
	while(mask!=0) mask &= mask-1, bitCount++;
	return bitCount;
}

template<typename T> forceinline Meta::EnableIf<
	Meta::IsUnsignedIntegralType<T>::_,
uint> FindBitPosition(T mask)
{
	return Count1Bits((mask&(~mask+1))-1);
}

forceinline uint BitCountToMask(uint bitCount)
{
	return (bitCount==32)? 0xFFFFFFFFu: (1u << bitCount)-1u;
}

INTRA_WARNING_POP

}}
