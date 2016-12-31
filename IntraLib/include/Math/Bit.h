#pragma once

#include "Core/Core.h"

namespace Intra { namespace Math {

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




}}

