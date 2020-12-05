#pragma once

#include "Intra/Type.h"
#include "Intra/Concepts.h"
#include "Intra/Range/Concepts.h"

INTRA_BEGIN

template<class C, typename T = TListValue<C>> concept CHas_push_back = requires(C c, T x) {c.push_back(x);};
template<class C, typename T = TListValue<C>> concept CHas_push_front = requires(C c, T x) {c.push_front(x);};
template<class C, typename... Args> concept CHas_emplace_back = requires(C c, Args&&... args) {c.emplace_back(INTRA_FWD(args)...);};
template<class C, typename... Args> concept CHas_emplace_front = requires(C c, Args&&... args) {c.emplace_front(INTRA_FWD(args)...);};

template<class C> concept CHas_clear = requires(C c) {c.clear();};
template<class C> concept CHas_resize = requires(C c) {c.resize(size_t());};
template<class C> concept CHasSetCountUninitialized = requires(C c) {c.SetCountUninitialized(index_t());};

template<class C> concept CHas_empty = requires(C c, bool& res) {res = c.empty();};
template<class C> concept CHas_reserve = requires(C c) {c.reserve(size_t());};

template<class C> concept CSequentialContainer =
	CHas_push_back<TRemoveConst<C>> &&
	CHas_size<C> &&
	CHas_empty<C>;

template<class C> concept CDynamicArrayContainer =
	CSequentialContainer<C> &&
	CHas_resize<TRemoveConst<C>> &&
	CHas_data<C>;

template<class C> concept CStaticArrayContainer =
	!CHas_push_back<TRemoveConst<C>> &&
	!CHas_resize<TRemoveConst<C>> &&
	!CHas_clear<TRemoveConst<C>> &&
	CHas_data<C> &&
	CHas_size<C>;


template<class C> requires CHas_reserve<C>
constexpr void Reserve(C& container, index_t capacity)
{
	if constexpr(CHas_reserve<C>)
		container.reserve(size_t(capacity));
}

template<class C> requires CHasSetCountUninitialized<C> || CHas_resize<C>
constexpr void SetCountTryNotInit(C& container, index_t newCount)
{
	if constexpr(CHasSetCountUninitialized<C>)
		container.SetCountUninitialized(newCount);
	else container.resize(size_t(newCount));
}
INTRA_END
