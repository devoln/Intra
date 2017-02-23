#pragma once

#include "Range/Concepts.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, равных x.
template<typename R, typename X> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsConvertible<X, ValueTypeOf<R>>::_,
R&> TrimLeftAdvance(R& range, const X& x)
{
	while(!range.Empty() && range.First()==x)
		range.PopFirst();
	return range;
}
	
//! Последовательно удаляет элементы из начала диапазона, пока выполняется предикат pred.
//! Останавливается на первом элементе, для которого это условие не выполнено.
template<typename R, typename P> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, ValueTypeOf<R>>::_,
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
	IsAsAccessibleRange<R>::_ &&
	(Meta::IsConvertible<X, ValueTypeOfAs<R>>::_ ||
		Meta::IsCallable<X, ValueTypeOfAs<R>>::_),
AsRangeResultNoCRef<R&&>> TrimLeft(R&& range, const X& valOrPred)
{
	auto rangeCopy = Range::Forward<R>(range);
	TrimLeftAdvance(rangeCopy, valOrPred);
	return rangeCopy;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, равных x.
template<typename R, typename X> Meta::EnableIf<
	IsBidirectionalRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsConvertible<X, ValueTypeOf<R>>::_,
R&> TrimRightAdvance(R& range, X x)
{
	while(!range.Empty() && range.Last()==x)
		range.PopLast();
	return range;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, для которых выполнен предикат pred.
template<typename R, typename P> Meta::EnableIf<
	IsBidirectionalRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, ValueTypeOf<R>>::_,
R&> TrimRightAdvance(R& range, P pred)
{
	while(!range.Empty() && pred(range.Last()))
		range.PopLast();
	return range;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых последних символов,
//! для которых выполнен предикат valOrPred, если это предикат, или который равен valOrPred, если это не предикат.
template<typename R, typename X> Meta::EnableIf<
	IsBidirectionalRange<R>::_ && !Meta::IsConst<R>::_ &&
	(Meta::IsConvertible<X, ValueTypeOf<R>>::_ ||
		Meta::IsCallable<X, ValueTypeOf<R>>::_),
R&> TrimAdvance(R& range, X valOrPred)
{return TrimRightAdvance(TrimLeftAdvance(range, valOrPred), valOrPred);}


//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов:
//! 1) которые равны значению valOrPred;
//! 2) для которых выполнен предикат pred.
//! \param valOrPred Значение, с которым сравнивается каждый элемент диапазона или предикат, принимающий элементы диапазона.
template<typename R, typename X> Meta::EnableIf<
	IsBidirectionalRange<R>::_ &&
	(Meta::IsConvertible<X, ValueTypeOf<R>>::_ ||
		Meta::IsCallable<X, ValueTypeOf<R>>::_),
AsRangeResultNoCRef<R>> TrimRight(R&& range, X valOrPred)
{
	auto rangeCopy = Range::Forward<R>(range);
	TrimRightAdvance(rangeCopy, valOrPred);
	return rangeCopy;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов:
//! 1) которые равны значению valOrPred;
//! 2) для которых выполнен предикат pred.
//! \param valOrPred Значение, с которым сравнивается каждый элемент диапазона или предикат, принимающий элементы диапазона.
template<typename R, typename X> forceinline Meta::EnableIf<
	IsAsBidirectionalRange<R>::_ &&
	(Meta::IsConvertible<X, ValueTypeOfAs<R>>::_ ||
		Meta::IsCallable<X, ValueTypeOfAs<R>>::_),
AsRangeResultNoCRef<R&&>> Trim(R&& range, X x)
{return TrimRight(TrimLeft(Range::Forward<R>(range), x), x);}

INTRA_WARNING_POP

}}
