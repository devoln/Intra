#pragma once

#include "Core/Range/Concepts.h"

INTRA_BEGIN
//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, равных x.
template<typename R, typename X> constexpr Requires<
	CInputRange<R> &&
	!CConst<R> &&
	CConvertible<X, TValueTypeOf<R>>,
R&> TrimLeftAdvance(R& range, const X& x)
{
	while(!range.Empty() && range.First()==x)
		range.PopFirst();
	return range;
}
	
//! Последовательно удаляет элементы из начала диапазона, пока выполняется предикат pred.
//! Останавливается на первом элементе, для которого это условие не выполнено.
template<typename R, typename P> constexpr Requires<
	CInputRange<R> &&
	!CConst<R> &&
	CCallable<P, TValueTypeOf<R>>,
R&> TrimLeftAdvance(R& range, P pred)
{
	while(!range.Empty() && pred(range.First()))
		range.PopFirst();
	return range;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов:
//! 1) которые равны valOrPred.
//! 2) для которых выполнен предикат valOrPred.
template<typename R, typename X> constexpr forceinline Requires<
	CAsAccessibleRange<R> &&
	(CConvertible<X, TValueTypeOfAs<R>> ||
		CCallable<X, TValueTypeOfAs<R>>),
TRangeOfTypeNoCRef<R&&>> TrimLeft(R&& range, const X& valOrPred)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	TrimLeftAdvance(rangeCopy, valOrPred);
	return rangeCopy;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, равных x.
template<typename R, typename X> constexpr Requires<
	CBidirectionalRange<R> &&
	!CConst<R> &&
	CConvertible<X, TValueTypeOf<R>>,
R&> TrimRightAdvance(R& range, X x)
{
	while(!range.Empty() && range.Last()==x)
		range.PopLast();
	return range;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, для которых выполнен предикат pred.
template<typename R, typename P> constexpr Requires<
	CBidirectionalRange<R> &&
	!CConst<R> &&
	CCallable<P, TValueTypeOf<R>>,
R&> TrimRightAdvance(R& range, P pred)
{
	while(!range.Empty() && pred(range.Last()))
		range.PopLast();
	return range;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых последних символов,
//! для которых выполнен предикат valOrPred, если это предикат, или который равен valOrPred, если это не предикат.
template<typename R, typename X> constexpr Requires<
	CBidirectionalRange<R> &&
	!CConst<R> &&
	(CConvertible<X, TValueTypeOf<R>> ||
		CCallable<X, TValueTypeOf<R>>),
R&> TrimAdvance(R& range, X valOrPred)
{return TrimRightAdvance(TrimLeftAdvance(range, valOrPred), valOrPred);}


//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов:
//! 1) которые равны значению valOrPred;
//! 2) для которых выполнен предикат pred.
//! @param valOrPred Значение, с которым сравнивается каждый элемент диапазона или предикат, принимающий элементы диапазона.
template<typename R, typename X,
	typename AsR = TRangeOfTypeNoCRef<R>,
	typename T = TValueTypeOf<AsR>
> INTRA_NODISCARD constexpr Requires<
	CBidirectionalRange<AsR> &&
	(CConvertible<X, T> ||
		CCallable<X, T>),
AsR> TrimRight(R&& range, X valOrPred)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	TrimRightAdvance(rangeCopy, valOrPred);
	return rangeCopy;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов:
//! 1) которые равны значению valOrPred;
//! 2) для которых выполнен предикат pred.
//! @param valOrPred Значение, с которым сравнивается каждый элемент диапазона или предикат, принимающий элементы диапазона.
template<typename R, typename X,
	typename AsR = TRangeOfTypeNoCRef<R>,
	typename T = TValueTypeOf<AsR>
> INTRA_NODISCARD constexpr forceinline Requires<
	CBidirectionalRange<AsR> &&
	(CConvertible<X, T> ||
		CCallable<X, T>),
AsR> Trim(R&& range, X x)
{return TrimRight(TrimLeft(ForwardAsRange<R>(range), x), x);}
INTRA_END
