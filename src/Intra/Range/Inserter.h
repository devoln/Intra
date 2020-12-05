#pragma once

#include "Intra/Type.h"
#include "Intra/Container/Concepts.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
template<typename C> struct LastAppender
{
	C& Dst;

	constexpr void Put(TListValue<C>&& v) {Dst.push_back(Move(v));}
	constexpr void Put(const TListValue<C>& v) {Dst.push_back(v);}

	template<typename T> requires CHas_emplace_back<T&&>
	constexpr void Put(T&& v) {Dst.emplace_back(INTRA_FWD(v));}
};

template<typename C> struct FirstAppender
{
	C& Dst;

	constexpr void Put(TRangeValue<C>&& v) {Dst.push_front(Move(v));}
	constexpr void Put(const TRangeValue<C>& v) {Dst.push_front(v);}

	template<typename T> requires CHas_emplace_front<T&&>
	constexpr void Put(T&& v) {Dst.emplace_front(INTRA_FWD(v));}
};
INTRA_END
