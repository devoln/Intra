#pragma once

#include "Intra/Type.h"
#include "Intra/Numeric.h"
#include "Intra/Misc/RawMemory.h"

#include "Murmur.h"

INTRA_BEGIN
INTRA_DEFINE_CONCEPT_REQUIRES(CHasToHashMethod, Val<T>().ToHash());

template<typename T> constexpr Requires<
	CIntegral<T>,
unsigned> ToHash(T k) {return unsigned(k*2659435761u);}

template<typename T>
#ifdef INTRA_CONSTEXPR_BITCAST_SUPPORT
constexpr
#endif
inline Requires<
	CFloatingPoint<T>,
unsigned> ToHash(T k)
{
	return ToHash(Misc::BitCast<TIntMin<sizeof(T)>>(k));
}

template<typename T> inline unsigned ToHash(T* k)
{return ToHash(reinterpret_cast<size_t>(k));}

inline unsigned ToHash(StringView k)
{return Hash::Murmur3_32(k, 0);}

template<typename T> Requires<
	CHasToHashMethod<const T>,
unsigned> ToHash(const T& value) {return value.ToHash();}

template<typename T> Requires<
	CHasUniqueObjectRepresentations<T> &&
	!CHasToHashMethod<T> &&
	!CPointer<T> &&
	!CArithmetic<T>,
unsigned> ToHash(const T& value) {return ToHash(StringView(reinterpret_cast<char*>(&value), sizeof(T)));}

struct HasherObject {
	template<typename T> unsigned operator()(const T& k) const {return ToHash(k);}
};
INTRA_END
