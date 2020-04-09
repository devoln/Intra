#pragma once

#include "Intra/Type.h"
#include "Intra/Concepts.h"
#include "Intra/Range/Concepts.h"

INTRA_BEGIN

INTRA_DEFINE_CONCEPT_REQUIRES2(CHas_push_back, Val<T1>().push_back(Val<T2>()),, = TValueTypeOf<U1>);
INTRA_DEFINE_CONCEPT_REQUIRES2(CHas_push_front, Val<T1>().push_front(Val<T2>()),, = TValueTypeOf<U1>);

INTRA_DEFINE_CONCEPT_REQUIRES(CHas_clear, Val<T>().clear());
INTRA_DEFINE_CONCEPT_REQUIRES(CHas_resize, Val<T>().resize(size_t()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHasSetCountUninitialized, Val<T>().SetCountUninitialized(index_t()));

INTRA_DEFINE_CONCEPT_REQUIRES(CHas_empty, static_cast<bool>(Val<T>().empty()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHas_reserve, Val<T>().reserve(size_t()));

template<typename C> concept CSequentialContainer =
	CHas_push_back<TRemoveConst<C>> &&
	CHas_size<C> &&
	CHas_empty<C>;

template<typename C> concept CDynamicArrayContainer =
	CSequentialContainer<C> &&
	CHas_resize<TRemoveConst<C>> &&
	CHas_data<C>;

template<typename C> concept CStaticArrayContainer =
	!CHas_push_back<TRemoveConst<C>> &&
	!CHas_resize<TRemoveConst<C>> &&
	!CHas_clear<TRemoveConst<C>> &&
	CHas_data<C> &&
	CHas_size<C>;


template<typename C, typename = Requires<CHas_reserve<C>>>
constexpr void Reserve(C& container, index_t capacity)
{
	if constexpr(CHas_reserve<C>)
		container.reserve(size_t(capacity));
}

template<typename C, typename = Requires<CHasSetCountUninitialized<C> || CHas_resize<C>>>
constexpr void SetCountTryNotInit(C& container, index_t newCount)
{
	if constexpr(CHasSetCountUninitialized<C>)
		container.SetCountUninitialized(newCount);
	else container.resize(size_t(newCount));
}
INTRA_END
