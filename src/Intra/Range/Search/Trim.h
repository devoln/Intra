#pragma once

#include "Intra/Range/Concepts.h"

INTRA_BEGIN
/// Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, равных x.
template<typename R, typename X> constexpr Requires<
	CRange<R> &&
	!CConst<R> &&
	CConvertibleTo<X, TRangeValue<R>>,
R&> TrimLeftAdvance(R& range, const X& x)
{
	while(!range.Empty() && range.First()==x)
		range.PopFirst();
	return range;
}
	
/// Последовательно удаляет элементы из начала диапазона, пока выполняется предикат pred.
/// Останавливается на первом элементе, для которого это условие не выполнено.
template<typename R, typename P> constexpr Requires<
	CRange<R> &&
	!CConst<R> &&
	CCallable<P, TRangeValue<R>>,
R&> TrimLeftAdvance(R& range, P pred)
{
	while(!range.Empty() && pred(range.First()))
		range.PopFirst();
	return range;
}

/// Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов:
/// 1) которые равны valOrPred.
/// 2) для которых выполнен предикат valOrPred.
template<typename R, typename X> constexpr Requires<
	CAccessibleList<R> &&
	(CConvertibleTo<X, TListValue<R>> ||
		CCallable<X, TListValue<R>>),
TRangeOf<R&&>> TrimLeft(R&& range, const X& valOrPred)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	TrimLeftAdvance(rangeCopy, valOrPred);
	return rangeCopy;
}

/// Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, равных x.
template<typename R, typename X> constexpr Requires<
	CBidirectionalRange<R> &&
	!CConst<R> &&
	CConvertibleTo<X, TRangeValue<R>>,
R&> TrimRightAdvance(R& range, X x)
{
	while(!range.Empty() && range.Last()==x)
		range.PopLast();
	return range;
}

/// Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, для которых выполнен предикат pred.
template<typename R, typename P> constexpr Requires<
	CBidirectionalRange<R> &&
	!CConst<R> &&
	CCallable<P, TRangeValue<R>>,
R&> TrimRightAdvance(R& range, P pred)
{
	while(!range.Empty() && pred(range.Last()))
		range.PopLast();
	return range;
}

/// Возвращает диапазон, полученный из этого диапазона удалением всех первых последних символов,
/// для которых выполнен предикат valOrPred, если это предикат, или который равен valOrPred, если это не предикат.
template<typename R, typename X> constexpr Requires<
	CBidirectionalRange<R> &&
	!CConst<R> &&
	(CConvertibleTo<X, TRangeValue<R>> ||
		CCallable<X, TRangeValue<R>>),
R&> TrimAdvance(R& range, X valOrPred)
{return TrimRightAdvance(TrimLeftAdvance(range, valOrPred), valOrPred);}


/// Возвращает диапазон, полученный из этого диапазона удалением всех последних символов:
/// 1) которые равны значению valOrPred;
/// 2) для которых выполнен предикат pred.
/// @param valOrPred Значение, с которым сравнивается каждый элемент диапазона или предикат, принимающий элементы диапазона.
template<typename R, typename X,
	typename AsR = TRangeOf<R>,
	typename T = TRangeValue<AsR>
> [[nodiscard]] constexpr Requires<
	CBidirectionalRange<AsR> &&
	(CConvertibleTo<X, T> ||
		CCallable<X, T>),
AsR> TrimRight(R&& range, X valOrPred)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	TrimRightAdvance(rangeCopy, valOrPred);
	return rangeCopy;
}

/// Возвращает диапазон, полученный из этого диапазона удалением всех последних символов:
/// 1) которые равны значению valOrPred;
/// 2) для которых выполнен предикат pred.
/// @param valOrPred Значение, с которым сравнивается каждый элемент диапазона или предикат, принимающий элементы диапазона.
template<typename R, typename X,
	typename AsR = TRangeOf<R>,
	typename T = TRangeValue<AsR>
> [[nodiscard]] constexpr Requires<
	CBidirectionalRange<AsR> &&
	(CConvertibleTo<X, T> ||
		CCallable<X, T>),
AsR> Trim(R&& range, X x)
{return TrimRight(TrimLeft(ForwardAsRange<R>(range), x), x);}
INTRA_END
