#pragma once

#include "Intra/Core.h"
#include "Intra/Numeric.h"
#include "Intra/Misc/RawMemory.h"

#include "Murmur.h"

namespace Intra { INTRA_BEGIN
template<typename T> concept CHasToHashMethod, Val<T>().ToHash());

template<typename T> requires CBasicIntegral<T>
constexpr uint32 ToHash(T k) {return uint32(k*2659435761u);}

template<typename T> requires CBasicFloatingPoint<T>
constexpr uint32 ToHash(T k)
{
	return ToHash(BitCastTo<TUnsignedIntOfSizeAtLeast<sizeof(T)>>(k));
}

template<typename T> inline unsigned ToHash(T* k)
{return ToHash(reinterpret_cast<size_t>(k));}

inline unsigned ToHash(StringView k)
{return Hash::Murmur3_32(k, 0);}

template<typename T> requires CHasToHashMethod<const T>
constexpr uint32 ToHash(const T& value) {return value.ToHash();}

template<typename T> requires CHasUniqueObjectRepresentations<T> &&
	(!CHasToHashMethod<T> && !CBasicPointer<T> && !CBasicArithmetic<T>),
uint32 ToHash(const T& value) {return ToHash(StringView(reinterpret_cast<char*>(&value), sizeof(T)));}

struct HasherObject {
	template<typename T> unsigned operator()(const T& k) const {return ToHash(k);}
};
} INTRA_END
