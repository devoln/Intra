#pragma once

#include "Meta/Type.h"
#include "Array.h"
#include "Range.h"

namespace Intra { namespace Concepts {

INTRA_DEFINE_EXPRESSION_CHECKER2(Has_push_back, Meta::Val<T1>().push_back(Meta::Val<T2>()),, = ValueTypeOf<U1>);
INTRA_DEFINE_EXPRESSION_CHECKER2(Has_push_front, Meta::Val<T1>().push_front(Meta::Val<T2>()),, = ValueTypeOf<U1>);

INTRA_DEFINE_EXPRESSION_CHECKER(Has_clear, Meta::Val<T>().clear());
INTRA_DEFINE_EXPRESSION_CHECKER(Has_resize, Meta::Val<T>().resize(size_t()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasSetCountUninitialized, Meta::Val<T>().SetCountUninitialized(size_t()));

INTRA_DEFINE_EXPRESSION_CHECKER(Has_empty, static_cast<bool>(Meta::Val<T>().empty()));
INTRA_DEFINE_EXPRESSION_CHECKER(Has_reserve, Meta::Val<T>().reserve(size_t()));

template<typename C> struct IsSequentialContainer: Meta::TypeFromValue<bool,
	Has_push_back<Meta::RemoveConst<C>>::_ &&
	Has_size<C>::_ &&
	Has_empty<C>::_
> {};

template<typename C> struct IsDynamicArrayContainer: Meta::TypeFromValue<bool,
	IsSequentialContainer<C>::_ &&
	Has_resize<Meta::RemoveConst<C>>::_ &&
	Has_data<C>::_
> {};

template<typename C> struct IsStaticArrayContainer: Meta::TypeFromValue<bool,
	!Has_push_back<Meta::RemoveConst<C>>::_ &&
	!Has_resize<Meta::RemoveConst<C>>::_ &&
	!Has_clear<Meta::RemoveConst<C>>::_ &&
	Has_data<C>::_ && Has_size<C>::_
> {};


template<typename C> Meta::EnableIf<
	Has_reserve<C>::_
> Reserve(C& container, size_t capacity) {container.reserve(capacity);}

template<typename C> Meta::EnableIf<
	!Has_reserve<C>::_
> Reserve(C&, size_t) {}

template<typename C> Meta::EnableIf<
	HasSetCountUninitialized<C>::_
> SetCountTryNotInit(C& container, size_t newCount) {container.SetCountUninitialized(newCount);}

template<typename C> Meta::EnableIf<
	!HasSetCountUninitialized<C>::_ && Has_resize<C>::_
> SetCountTryNotInit(C& container, size_t newCount) {container.resize(newCount);}

}}
