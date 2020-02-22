#pragma once

#include "Core/Type.h"

#include "Murmur.h"

INTRA_BEGIN
namespace Hash {

INTRA_DEFINE_CONCEPT_REQUIRES(HasToHashMethod, Val<T>().ToHash());

template<typename T> constexpr inline Requires<
	CIntegral<T>,
uint> ToHash(T k) {return uint(k*2659435761u);}

template<typename T> inline Requires<
	CFloatingPoint<T>,
uint> ToHash(T k)
{
	union {T t; IntegralTypeFromMinSize<sizeof(T)> i;};
	t = k;
	return ToHash(i);
}

template<typename T> inline uint ToHash(T* k)
{return ToHash(reinterpret_cast<size_t>(k));}

inline uint ToHash(StringView k)
{return Murmur3_32(k, 0);}

template<typename T> Requires<
	HasToHashMethod<const T>,
uint> ToHash(const T& value) {return value.ToHash();}

template<typename T> Requires<
	CPod<T> && !HasToHashMethod<T> &&
	!CPointer<T> && !CArithmetic<T>,
uint> ToHash(const T& value) {return ToHash(StringView(reinterpret_cast<char*>(&value), sizeof(T)));}

struct HasherObject {
	template<typename T> uint operator()(const T& k) const {return ToHash(k);}
};

}
using Hash::ToHash;
INTRA_END
