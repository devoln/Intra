#pragma once

#include "Core/FundamentalTypes.h"
#include "Platform/CppWarnings.h"
#include "Meta/Type.h"
#include "Murmur.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_DEFINE_EXPRESSION_CHECKER(HasToHashMethod, Meta::Val<T>().ToHash());

template<typename T> constexpr inline Meta::EnableIf<
	Meta::IsIntegralType<T>::_,
uint> ToHash(T k) {return uint(k*2659435761u);}

template<typename T> inline Meta::EnableIf<
	Meta::IsFloatType<T>::_,
uint> ToHash(T k)
{
	union {T t; Meta::IntegralTypeFromMinSize<sizeof(T)> i;};
	t = k;
	return ToHash(i);
}

template<typename T> inline uint ToHash(T* k)
{return ToHash(reinterpret_cast<size_t>(k));}

inline uint ToHash(StringView k)
{return Hash::Murmur3_32(k, 0);}

template<typename T> Meta::EnableIf<
	HasToHashMethod<const T>::_,
uint> ToHash(const T& value) {return value.ToHash();}

template<typename T> Meta::EnableIf<
	Meta::IsAlmostPod<T>::_ && !HasToHashMethod<T>::_ &&
	!Meta::IsPointerType<T>::_ && !Meta::IsArithmeticType<T>::_,
uint> ToHash(const T& value) {return ToHash(StringView(reinterpret_cast<char*>(&value), sizeof(T)));}

struct HasherObject {
	template<typename T> uint operator()(const T& k) const {return ToHash(k);}
};

INTRA_WARNING_POP

}}
