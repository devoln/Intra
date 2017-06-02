#pragma once

#include "Cpp/Warnings.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, равных x.
template<typename R, typename X> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsConvertible<X, Concepts::ValueTypeOf<R>>::_,
R&> TrimLeftAdvance(R& range, const X& x)
{
	while(!range.Empty() && range.First()==x)
		range.PopFirst();
	return range;
}
	
//! Последовательно удаляет элементы из начала диапазона, пока выполняется предикат pred.
//! Останавливается на первом элементе, для которого это условие не выполнено.
template<typename R, typename P> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<R>>::_,
R&> TrimLeftAdvance(R& range, P pred)
{
	while(!range.Empty() && pred(range.First()))
		range.PopFirst();
	return range;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов:
//! 1) которые равны valOrPred.
//! 2) для которых выполнен предикат valOrPred.
template<typename R, typename X> forceinline Meta::EnableIf<
	Concepts::IsAsAccessibleRange<R>::_ &&
	(Meta::IsConvertible<X, Concepts::ValueTypeOfAs<R>>::_ ||
		Meta::IsCallable<X, Concepts::ValueTypeOfAs<R>>::_),
Concepts::RangeOfTypeNoCRef<R&&>> TrimLeft(R&& range, const X& valOrPred)
{
	auto rangeCopy = Range::Forward<R>(range);
	TrimLeftAdvance(rangeCopy, valOrPred);
	return rangeCopy;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, равных x.
template<typename R, typename X> Meta::EnableIf<
	Concepts::IsBidirectionalRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsConvertible<X, Concepts::ValueTypeOf<R>>::_,
R&> TrimRightAdvance(R& range, X x)
{
	while(!range.Empty() && range.Last()==x)
		range.PopLast();
	return range;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, для которых выполнен предикат pred.
template<typename R, typename P> Meta::EnableIf<
	Concepts::IsBidirectionalRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<R>>::_,
R&> TrimRightAdvance(R& range, P pred)
{
	while(!range.Empty() && pred(range.Last()))
		range.PopLast();
	return range;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых последних символов,
//! для которых выполнен предикат valOrPred, если это предикат, или который равен valOrPred, если это не предикат.
template<typename R, typename X> Meta::EnableIf<
	Concepts::IsBidirectionalRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	(Meta::IsConvertible<X, Concepts::ValueTypeOf<R>>::_ ||
		Meta::IsCallable<X, Concepts::ValueTypeOf<R>>::_),
R&> TrimAdvance(R& range, X valOrPred)
{return TrimRightAdvance(TrimLeftAdvance(range, valOrPred), valOrPred);}


//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов:
//! 1) которые равны значению valOrPred;
//! 2) для которых выполнен предикат pred.
//! @param valOrPred Значение, с которым сравнивается каждый элемент диапазона или предикат, принимающий элементы диапазона.
template<typename R, typename X,
	typename AsR = Concepts::RangeOfTypeNoCRef<R>,
	typename T = Concepts::ValueTypeOf<AsR>
> Meta::EnableIf<
	Concepts::IsBidirectionalRange<AsR>::_ &&
	(Meta::IsConvertible<X, T>::_ ||
		Meta::IsCallable<X, T>::_),
AsR> TrimRight(R&& range, X valOrPred)
{
	auto rangeCopy = Range::Forward<R>(range);
	TrimRightAdvance(rangeCopy, valOrPred);
	return rangeCopy;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов:
//! 1) которые равны значению valOrPred;
//! 2) для которых выполнен предикат pred.
//! @param valOrPred Значение, с которым сравнивается каждый элемент диапазона или предикат, принимающий элементы диапазона.
template<typename R, typename X,
	typename AsR = Concepts::RangeOfTypeNoCRef<R>,
	typename T = Concepts::ValueTypeOf<AsR>
> forceinline Meta::EnableIf<
	Concepts::IsBidirectionalRange<AsR>::_ &&
	(Meta::IsConvertible<X, T>::_ ||
		Meta::IsCallable<X, T>::_),
AsR> Trim(R&& range, X x)
{return TrimRight(TrimLeft(Range::Forward<R>(range), x), x);}

INTRA_WARNING_POP

}}
