#pragma once

#include "Range/Concepts.h"

namespace Intra { namespace Algo {

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, равных x.
template<typename R, typename X> Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsConvertible<X, Range::ValueTypeOf<R>>::_,
R&&> TrimLeftAdvance(R&& range, const X& x)
{
	while(!range.Empty() && range.First()==x)
		range.PopFirst();
	return range;
}
	
//! Последовательно удаляет элементы из начала диапазона, пока выполняется предикат pred.
//! Останавливается на первом элементе, для которого это условие не выполнено.
template<typename R, typename P> Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
R&&> TrimLeftAdvance(R&& range, P pred)
{
	while(!range.Empty() && pred(range.First()))
		range.PopFirst();
	return Meta::Forward<R>(range);
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, равных x.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Meta::IsConvertible<X, Range::ValueTypeOf<R>>::_,
R> TrimLeft(const R& range, const X& x)
{
	R result = range;
	return TrimLeftAdvance(result, x);
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, для которых выполнен предикат pred.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
R> TrimLeft(const R& range, P pred)
{
	R result = range;
	return TrimLeftAdvance(result, pred);
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, равных x.
template<typename R, typename X> Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsConvertible<X, Range::ValueTypeOf<R>>::_,
R&&> TrimRightAdvance(R&& range, X x)
{
	while(!range.Empty() && range.Last()==x)
		range.PopLast();
	return Meta::Forward<R>(range);
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, для которых выполнен предикат pred.
template<typename R, typename P> Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
R&&> TrimRightAdvance(R&& range, P pred)
{
	while(!range.Empty() && pred(range.Last()))
		range.PopLast();
	return Meta::Forward<R>(range);
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых последних символов,
//! для которых выполнен предикат valOrPred, если это предикат, или который равен valOrPred, если это не предикат.
template<typename R, typename X> Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsConvertible<X, Range::ValueTypeOf<R>>::_ || Meta::IsCallable<X, Range::ValueTypeOf<R>>::_,
R&&> TrimAdvance(R&& range, X valOrPred) {return TrimRightAdvance(TrimLeftAdvance(Meta::Forward<R>(range), valOrPred), valOrPred);}


//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, равных x.
template<typename R, typename X> Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_ &&
	Meta::IsConvertible<X, Range::ValueTypeOf<R>>::_,
Meta::RemoveConstRef<R>> TrimRight(R&& range, X x)
{return TrimRightAdvance(Meta::RemoveConstRef<R>(Meta::Forward<R>(range)), x);}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, для которых выполнен предикат pred.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
Meta::RemoveConstRef<R>> TrimRight(R&& range, P pred)
{return TrimRightAdvance(Meta::RemoveConstRef<R>(Meta::Forward<R>(range)), pred);}

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых и последних символов,
//! для которых выполнен предикат valOrPred, если это предикат, или равных valOrPred.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_ &&
	Meta::IsConvertible<X, Range::ValueTypeOf<R>>::_ || Meta::IsCallable<X, Range::ValueTypeOf<R>>::_,
Meta::RemoveConstRef<R>> Trim(R&& range, X x)
{return TrimRight(TrimLeft(Meta::Forward<R>(range), x), x);}

}}
