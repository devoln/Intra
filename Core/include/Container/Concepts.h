#pragma once

#include "Meta/Type.h"
#include "Range/Concepts.h"

namespace Intra { namespace Container { namespace Concepts {

INTRA_DEFINE_EXPRESSION_CHECKER2(Has_push_back, Meta::Val<T1>().push_back(Meta::Val<T2>()),, = Range::ValueTypeOf<U1>);
INTRA_DEFINE_EXPRESSION_CHECKER2(Has_push_front, Meta::Val<T1>().push_front(Meta::Val<T2>()),, = Range::ValueTypeOf<U1>);

INTRA_DEFINE_EXPRESSION_CHECKER(Has_clear, Meta::Val<T>().clear());
INTRA_DEFINE_EXPRESSION_CHECKER(Has_resize, Meta::Val<T>().resize(size_t()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasSetCountUninitialized, Meta::Val<T>().SetCountUninitialized(size_t()));

INTRA_DEFINE_EXPRESSION_CHECKER(Has_size, static_cast<size_t>(Meta::Val<T>().size()));
using Range::Concepts::HasLength;
INTRA_DEFINE_EXPRESSION_CHECKER(Has_empty, static_cast<bool>(Meta::Val<T>().empty()));
using Range::Concepts::HasEmpty;
using Range::Concepts::ValueTypeOf;

INTRA_DEFINE_EXPRESSION_CHECKER(Has_data, Meta::Val<T>().data());

INTRA_DEFINE_EXPRESSION_CHECKER(Has_reserve, Meta::Val<T>().reserve(size_t()));

template<typename C> struct IsSequentialContainer: Meta::TypeFromValue<bool,
	Has_push_back<Meta::RemoveConst<C>>::_ && Has_size<C>::_ && Has_empty<C>::_
> {};

template<typename C> struct IsDynamicArrayContainer: Meta::TypeFromValue<bool,
	IsSequentialContainer<C>::_ && Has_resize<Meta::RemoveConst<C>>::_ && Has_data<C>::_
> {};

template<typename C> struct IsStaticArrayContainer: Meta::TypeFromValue<bool,
	!Has_push_back<Meta::RemoveConst<C>>::_ &&
	!Has_resize<Meta::RemoveConst<C>>::_ &&
	!Has_clear<Meta::RemoveConst<C>>::_ &&
	Has_data<C>::_ && Has_size<C>::_
> {};

}
using namespace Concepts;

}}
