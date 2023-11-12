#pragma once

#include <Intra/Core.h>
#include <Intra/Concepts.h>

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
template<typename C> struct LastAppender
{
	C& Dst;

	template<CSameUnqualReF<TListValue<C>> T> constexpr auto Put(T&& v) -> decltype(Dst.push_back(INTRA_FWD(v))) {Dst.push_back(INTRA_FWD(v));}
	template<CSameUnqualReF<TListValue<C>> T> constexpr auto Put(T&& v) -> decltype(Dst.AddLast(INTRA_FWD(v))) {Dst.AddLast(INTRA_FWD(v));}

	template<typename T> constexpr auto Put(T&& v) -> decltype(Dst.emplace_back(INTRA_FWD(v))) {Dst.emplace_back(INTRA_FWD(v));}
};

template<typename C> struct FirstAppender
{
	C& Dst;

	constexpr void Put(TRangeValue<C>&& v) {Dst.push_front(Move(v));}
	constexpr void Put(const TRangeValue<C>& v) {Dst.push_front(v);}

	template<typename T> requires CHas_emplace_front<T&&>
	constexpr void Put(T&& v) {Dst.emplace_front(INTRA_FWD(v));}
};
} INTRA_END
