#pragma once

#include "Core/Operations.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Take.h"
#include "Core/Range/Operations.h"

INTRA_BEGIN

/** Pop elements of ``range`` one by one until the first occurence of ``what`` is found or range becomes empty.
  @param what Element to found using operator ==.
  @param ioIndex[inout] Pointer to counter that must be incremented by the number of popped elements. Optional, may be null.
  @return The remaining part of ``range``.
*/
template<class R, typename X> constexpr Requires<
	CFiniteInputRange<R> &&
	!CConst<R> &&
	!CCallable<X, TValueTypeOf<R>> &&
	!(CFiniteForwardRange<X> &&
		CEqualityComparable<TValueTypeOf<R>, TValueTypeOf<X>>),
R&&> FindAdvance(R&& range, const X& what, Optional<index_t&> ioIndex = null)
{
	size_t index = 0;
	while(!range.Empty() && !(range.First() == what))
	{
		range.PopFirst();
		index++;
	}
	if(ioIndex) ioIndex.Unwrap() += index;
	return Forward<R>(range);
}

/** Pop elements of ``range`` one by one until the first element for which ``pred`` returns true.
  @param pred Predicate taking an element of the range.
  @param ioIndex[inout] Pointer to counter that must be incremented by the number of popped elements. Optional, may be null.
  @return The remaining part of ``range``.
*/
template<class R, typename P> constexpr Requires<
	CFiniteInputRange<R> &&
	!CConst<R> &&
	CCallable<P, TValueTypeOf<R>>,
R&> FindAdvance(R& range, P pred, Optional<index_t&> ioIndex = null)
{
	index_t index = 0;
	while(!range.Empty() && !pred(range.First()))
	{
		range.PopFirst();
		index++;
	}
	if(ioIndex) ioIndex.Unwrap() += index;
	return range;
}


//! Найти первое вхождение любого элемента из диапазона whats в этот диапазон.
//! Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
//! \param whats Диапазон искомых элементов. Если один из этих элементов найден,
//! начало whats устанавливается на найденный элемент. Иначе whats становится пустым
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
//! \return Возвращает ссылку на себя.
template<class R, class Ws> constexpr Requires<
	CNonInfiniteInputRange<R> &&
	!CConst<R> &&
	CNonInfiniteForwardRange<Ws> &&
	!CConst<Ws> &&
	CEqualityComparable<TValueTypeOf<R>, TValueTypeOf<Ws>>,
R&> FindAdvanceAnyAdvance(R& range, Ws& whats,
	Optional<index_t&> ioIndex = null, Optional<index_t&> oWhatIndex = null)
{
	size_t index = 0, whatIndex = Count(whats);
	Ws whatsCopy = Forward<Ws>(whats);
	for(; !range.Empty(); range.PopFirst(), index++)
	{
		whats = whatsCopy;
		whatIndex = 0;
		while(!whats.Empty())
		{
			if(!(range.First() == whats.First()))
			{
				whats.PopFirst();
				whatIndex++;
				continue;
			}
			if(ioIndex) ioIndex.Unwrap() += index;
			if(oWhatIndex) oWhatIndex.Unwrap() = whatIndex;
			return range;
		}
	}
	if(ioIndex) ioIndex.Unwrap() += index;
	if(oWhatIndex) oWhatIndex.Unwrap() = whatIndex;
	return range;
}

//! Найти первое вхождение любого элемента из диапазона whats в этот диапазон.
//! Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
//! \param whats Диапазон искомых элементов.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
//! \return Возвращает ссылку на себя.
template<class R, class Ws,
	typename AsWs = TRangeOfType<Ws>
> constexpr forceinline Requires<
	CNonInfiniteInputRange<R> &&
	!CConst<R> &&
	CNonInfiniteForwardRange<AsWs> &&
	CEqualityComparable<TValueTypeOf<R>, TValueTypeOf<AsWs>>,
R&> FindAdvanceAny(R& range, Ws&& whats, Optional<index_t&> ioIndex = null, Optional<index_t&> oWhatIndex = null)
{
	auto whatsCopy = ForwardAsRange<Ws>(whats);
	return FindAdvanceAnyAdvance(range, whatsCopy, ioIndex, oWhatIndex);
}


//! Последовательно удаляет все элементы из диапазона, подсчитывая количество элементов, равных x.
template<class R, typename X> constexpr Requires<
	CInputRange<R> &&
	!CConst<R> &&
	!CCallable<X, TValueTypeOf<R>> &&
	CEqualityComparable<TReturnValueTypeOf<R>, const X&>,
index_t> CountAdvance(R& range, const X& x)
{
	index_t result = 0;
	while(!range.Empty())
	{
		if(range.First() == x) result++;
		range.PopFirst();
	}
	return result;
}


//! Последовательно удаляет все элементы из диапазона, подсчитывая количество элементов, для которых выполнено условие pred.
template<class R, typename P> constexpr Requires<
	CInputRange<R> &&
	!CConst<R> &&
	CCallable<P, TValueTypeOf<R>>,
index_t> CountAdvance(R& range, P pred)
{
	index_t result = 0;
	while(!range.Empty())
	{
		if(pred(range.First())) result++;
		range.PopFirst();
	}
	return result;
}

//! Подсчитать количество элементов, равных x или для которых выполнен предикат x.
template<class R, typename X,
	typename AsR = TRangeOfType<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CConsumableRange<AsR> &&
	(CCallable<X, TValueTypeOf<AsR>> ||
		CEqualityComparable<TReturnValueTypeOf<AsR>, const X&>),
index_t> Count(R&& range, const X& x)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return CountAdvance(rangeCopy, x);
}


//! Последовательно удаляет элементы из начала диапазона, до тех пор, пока не:
//! 1) встретится элемент, для которого выполнится предикат valOrPred, если это предикат;
//! 2) встретится элемент, равный valOrPred, если это не предикат;
//! 3) будет достигнут конец диапазона.
//! \return Возвращает количество пройденных элементов.
template<class R, typename X> constexpr forceinline Requires<
	CNonInfiniteInputRange<R> &&
	!CConst<R> &&
	!(CNonInfiniteForwardRange<X> &&
	CEqualityComparable<TValueTypeOf<R>, TValueTypeOf<X>>),
index_t> CountUntilAdvance(R&& range, const X& valOrPred)
{
	index_t index = 0;
	FindAdvance(range, valOrPred, OptRef(index));
	return index;
}

//! Последовательно удаляет элементы из начала диапазона до тех пор,
//! пока не встретится элемент, равный любому из элементов диапазона whats.
//! @param whats[inout] Диапазон искомых элементов. Если элемент найден, начало диапазона whats смещается к найденному элементу.
//! @return Возвращает количество пройденных элементов.
template<class R, class Ws> constexpr forceinline Requires<
	CFiniteInputRange<R> &&
	!CConst<R> &&
	CForwardRange<Ws> &&
	!CConst<Ws> &&
	CEqualityComparable<TValueTypeOf<R>, TValueTypeOf<Ws>>,
index_t> CountUntilAdvanceAnyAdvance(R&& range, Ws&& whats)
{
	index_t index = 0;
	FindAdvanceAnyAdvance(range, whats, OptRef(index));
	return index;
}

//! Последовательно удаляет элементы из начала диапазона до тех пор,
//! пока не встретится элемент, равный любому из элементов диапазона whats.
//! @param whats[inout] Диапазон искомых элементов.
//! @return Возвращает количество пройденных элементов.
template<class R, class Ws,
	typename AsWs = TRangeOfType<Ws>
> constexpr forceinline Requires<
	CNonInfiniteInputRange<R> &&
	!CConst<R> &&
	CForwardRange<AsWs> &&
	CEqualityComparable<TValueTypeOf<R>, TValueTypeOf<AsWs>>,
index_t> CountUntilAdvanceAny(R&& range, Ws&& whats)
{
	index_t index = 0;
	FindAdvanceAny(range, ForwardAsRange<Ws>(whats), OptRef(index));
	return index;
}



//! Последовательно просматривает элементы диапазона до тех пор,
//! пока не встретится элемент, для которого либо выполнен предикат valueOrPred,
//! либо равный valueOrPred, если это не предикат, или не будет достигнут конец диапазона.
//! Возвращает количество пройденных элементов.
template<class R, typename X,
	typename AsR = TRangeOfType<R>,
	typename T = TValueTypeOf<AsR>
> INTRA_NODISCARD constexpr forceinline Requires<
	CNonInfiniteForwardRange<AsR> &&
	(CEqualityComparable<T, X> ||
		CCallable<X, T>),
index_t> CountUntil(R&& range, const X& valueOrPred)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return CountUntilAdvance(rangeCopy, valueOrPred);
}


//! Найти первое вхождение элемента what в этот диапазон.
//! \param what Искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \returns null, если значение не найдено. Часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
template<class R, typename X,
	typename AsR = TRangeOfTypeNoCRef<R>
> constexpr Requires<
	CNonInfiniteForwardRange<AsR> &&
	CEqualityComparable<TValueTypeOf<AsR>, X>,
AsR> Find(R&& range, const X& what, Optional<index_t&> ioIndex = null)
{
	auto result = ForwardAsRange<R>(range);
	FindAdvance(result, what, ioIndex);
	return result;
}

template<class R, typename X,
	typename AsR = TRangeOfType<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CNonInfiniteForwardRange<AsR> &&
	CEqualityComparable<TValueTypeOf<AsR>, X>,
bool> Contains(R&& range, const X& what)
{return !Find(ForwardAsRange<R>(range), what).Empty();}

//! Найти первое вхождение элемента, удовлетворяющего некоторому условию, в этот диапазон.
//! \param pred Условие, которому должен удовлетворять искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
template<class R, typename P,
	typename AsR = TRangeOfTypeNoCRef<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CNonInfiniteForwardRange<AsR> &&
	CCallable<P, TValueTypeOf<AsR>>,
AsR> Find(R&& range, P pred, Optional<index_t&> ioIndex = null)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return FindAdvance(rangeCopy, pred, ioIndex);
}
INTRA_END
