#pragma once

#include "Type.h"
#include "CArray.h"
#include "Core/Range/Concepts.h"

INTRA_BEGIN

INTRA_DEFINE_CONCEPT_REQUIRES2(CHas_push_back, Val<T1>().push_back(Val<T2>()),, = TValueTypeOf<U1>);
INTRA_DEFINE_CONCEPT_REQUIRES2(CHas_push_front, Val<T1>().push_front(Val<T2>()),, = TValueTypeOf<U1>);

INTRA_DEFINE_CONCEPT_REQUIRES(CHas_clear, Val<T>().clear());
INTRA_DEFINE_CONCEPT_REQUIRES(CHas_resize, Val<T>().resize(size_t()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHasSetCountUninitialized, Val<T>().SetCountUninitialized(size_t()));

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


template<typename C> Requires<
	CHas_reserve<C>
> Reserve(C& container, size_t capacity) {container.reserve(capacity);}

template<typename C> Requires<
	!CHas_reserve<C>
> Reserve(C&, size_t) {}

template<typename C> Requires<
	CHasSetCountUninitialized<C>
> SetCountTryNotInit(C& container, size_t newCount) {container.SetCountUninitialized(newCount);}

template<typename C> Requires<
	!CHasSetCountUninitialized<C> &&
	CHas_resize<C>
> SetCountTryNotInit(C& container, size_t newCount) {container.resize(newCount);}
INTRA_END
