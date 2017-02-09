#pragma once

#include "Meta/Type.h"
#include "Range/Concepts.h"

namespace Intra { namespace Container {

INTRA_DEFINE_EXPRESSION_CHECKER2(HasAddLast, Meta::Val<T1>().AddLast(Meta::Val<T2>()),, = Range::ValueTypeOf<U1>);
INTRA_DEFINE_EXPRESSION_CHECKER2(HasAddFirst, Meta::Val<T1>().AddFirst(Meta::Val<T2>()),, = Range::ValueTypeOf<U1>);
INTRA_DEFINE_EXPRESSION_CHECKER2(Has_push_back, Meta::Val<T1>().push_back(Meta::Val<T2>()),, = Range::ValueTypeOf<U1>);
INTRA_DEFINE_EXPRESSION_CHECKER2(Has_push_front, Meta::Val<T1>().push_front(Meta::Val<T2>()),, = Range::ValueTypeOf<U1>);

INTRA_DEFINE_EXPRESSION_CHECKER(HasClear, Meta::Val<T>().Clear());
INTRA_DEFINE_EXPRESSION_CHECKER(Has_clear, Meta::Val<T>().clear());
INTRA_DEFINE_EXPRESSION_CHECKER(HasSetCount, Meta::Val<T>().SetCount(size_t()));
INTRA_DEFINE_EXPRESSION_CHECKER(Has_resize, Meta::Val<T>().resize(size_t()));

INTRA_DEFINE_EXPRESSION_CHECKER(HasCount, static_cast<size_t>(Meta::Val<T>().Count()));
INTRA_DEFINE_EXPRESSION_CHECKER(Has_size, static_cast<size_t>(Meta::Val<T>().size()));
INTRA_DEFINE_EXPRESSION_CHECKER(Has_length, static_cast<size_t>(Meta::Val<T>().length()));
using Range::HasLength;

INTRA_DEFINE_EXPRESSION_CHECKER(HasData, Meta::Val<T>().Data()==static_cast<Range::ValueTypeOfAs<T>*>(null));

INTRA_DEFINE_EXPRESSION_CHECKER(HasReserve, Meta::Val<T>().Reserve(size_t()));
INTRA_DEFINE_EXPRESSION_CHECKER(Has_reserve, Meta::Val<T>().reserve(size_t()));

template<typename C> struct IsClearable: Meta::TypeFromValue<bool,
	HasClear<C>::_ || Has_clear<C>::_ || HasSetCount<C>::_
> {};

template<typename C> struct IsLastAppendable: Meta::TypeFromValue<bool,
	HasAddLast<C>::_ || Has_push_back<C>::_
> {};

template<typename C> struct IsFirstAppendable: Meta::TypeFromValue<bool,
	HasAddFirst<C>::_ || Has_push_front<C>::_
> {};

template<typename C> struct IsResizable: Meta::TypeFromValue<bool,
	HasSetCount<C>::_ || Has_resize<C>::_
> {};

template<typename C> Meta::EnableIf<
	HasAddLast<C>::_
> AddLast(C& dst, Range::ValueTypeOf<C>&& v) {dst.AddLast(Meta::Move(v));}

template<typename C> Meta::EnableIf<
	HasAddLast<C>::_
> AddLast(C& dst, const Range::ValueTypeOf<C>& v) {dst.AddLast(v);}

template<typename C> Meta::EnableIf<
	!HasAddLast<C>::_ && Has_push_back<C>::_
> AddLast(C& dst, Range::ValueTypeOf<C>&& v) {dst.push_back(Meta::Move(v));}

template<typename C> Meta::EnableIf<
	!HasAddLast<C>::_ && Has_push_back<C>::_
> AddLast(C& dst, Range::ValueTypeOf<C>& v) {dst.push_back(v);}


template<typename C> Meta::EnableIf<
	HasAddFirst<C>::_
> AddFirst(C& dst, Range::ValueTypeOf<C>&& v) {dst.AddFirst(Meta::Move(v));}

template<typename C> Meta::EnableIf<
	HasAddFirst<C>::_
> AddFirst(C& dst, const Range::ValueTypeOf<C>& v) {dst.AddFirst(v);}

template<typename C> Meta::EnableIf<
	!HasAddFirst<C>::_ && Has_push_front<C>::_
> AddFirst(C& dst, Range::ValueTypeOf<C>&& v) {dst.push_front(Meta::Move(v));}

template<typename C> Meta::EnableIf<
	!HasAddFirst<C>::_ && Has_push_front<C>::_
> AddFirst(C& dst, const Range::ValueTypeOf<C>& v) {dst.push_front(v);}


template<typename C> Meta::EnableIf<
	HasSetCount<C>::_
> SetCount0(C& container) {container.SetCount(0);}

template<typename C> Meta::EnableIf<
	!HasSetCount<C>::_ && HasClear<C>::_
> SetCount0(C& container) {container.Clear();}

template<typename C> Meta::EnableIf<
	!HasSetCount<C>::_ && !HasClear<C>::_ && Has_clear<C>::_
> SetCount0(C& container) {container.clear();}

template<typename C> Meta::EnableIf<
	HasReserve<C>::_
> Reserve(C& container, size_t capacity) {container.Reserve(capacity);}

template<typename C> Meta::EnableIf<
	!HasReserve<C>::_ && Has_reserve<C>::_
> Reserve(C& container, size_t capacity) {container.reserve(capacity);}

template<typename C> Meta::EnableIf<
	!HasReserve<C>::_ && !Has_reserve<C>::_
> Reserve(C&, size_t) {}

template<typename C> Meta::EnableIf<
	HasSetCount<C>::_
> SetCount(C& container, size_t newCount) {container.SetCount(newCount);}

template<typename C> Meta::EnableIf<
	!HasSetCount<C>::_ && Has_resize<C>::_
> SetCount(C& container, size_t newCount) {container.resize(newCount);}

}}
