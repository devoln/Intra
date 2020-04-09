#pragma once

#include "Intra/Operations.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Take.h"
#include "Intra/Range/Operations.h"
#include "Intra/Functional.h"

INTRA_BEGIN

/** Pop elements of \p range one by one until the first element for which \p pred returns true.
  @param pred A predicate taking an element of the range.
  @note The found element index equals to the number of the false evaluations of \p pred.
  @return The remaining part of \p range including found element.
*/
template<class R, typename P> constexpr Requires<
	CInputRange<R> && !CConst<R> &&
	CCallable<P&&, TValueTypeOf<R>>,
R&> PopFirstUntil(R& range, P&& pred)
{
	while(!range.Empty() && !pred(range.First()))
		range.PopFirst();
	return range;
}


//! Count the number of the elements of \p range until the first element for which \p pred returns true or until range becomes empty.
template<class R, typename P> [[nodiscard]] constexpr Requires<
	CAsAccessibleRange<R> &&
	CCallable<P, TValueTypeOfAs<R>>,
index_t> Count(R&& range, P&& pred)
{
	index_t result = 0;
	for(auto&& x: ForwardAsRange<R>(range))
		if(pred(x)) result++;
	return result;
}


/** Pop the \p range elements until the first element for which \p pred returns true or until range becomes empty.
  @return The number of false evaluations of \p pred.
*/
template<class R, class P> constexpr Requires<
	CInputRange<R> && !CConst<R> &&
	CCallable<P&&, TValueTypeOf<R>>,
index_t> CountUntilAdvance(R&& range, P&& pred)
{
	auto countingPred = CountFalses{ForwardAsFunc<P>(pred)};
	FindAdvance(range, countingPred);
	return countingPred.Counter;
}

//! Call \p pred sequentially for every \p range element until the first element for which \p pred returns true or until range becomes empty.
//! @return The number of false evaluations of \p pred.
template<class R, class P> [[nodiscard]] constexpr Requires<
	CAsAccessibleRange<R> &&
	CCallable<P&&, TValueTypeOfAs<R>>,
index_t> CountUntil(R&& range, P&& pred)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return CountUntilAdvance(rangeCopy, pred);
}


//! Find the first element for which /p pred is true.
//! @param pred Predicate which returns true for search item.
//! @returns null, если значение не найдено. Часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
template<class R, typename P> constexpr Requires<
	CAsAccessibleRange<R> &&
	CCallable<P, TValueTypeOfAs<R>>,
TRangeOf<R>> DropUntil(R&& range, P&& pred)
{
	auto result = ForwardAsRange<R>(range);
	FindAdvance(result, Forward<P>(pred));
	return result;
}

template<class R, typename P> [[nodiscard]] constexpr Requires<
	CAsAccessibleRange<R> &&
	CCallable<P, TValueTypeOfAs<R>>,
bool> Contains(R&& range, P&& pred)
{return !DropUntil(ForwardAsRange<R>(range), Forward<P>(pred)).Empty();}





//! Найти первое вхождение любого элемента из диапазона whats в этот диапазон.
//! Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
//! \param whats Диапазон искомых элементов. Если один из этих элементов найден,
//! начало whats устанавливается на найденный элемент. Иначе whats становится пустым
//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
//! \return Возвращает ссылку на себя.
template<class R, class Ws> constexpr Requires<
	CInputRange<R> && !CConst<R> &&
	CNonInfiniteForwardRange<Ws> && !CConst<Ws> &&
	CEqualityComparable<TValueTypeOf<R>, TValueTypeOf<Ws>>,
R&> PopFirstUntilAnyAdvance(R&& range, Ws& whats,
	Optional<index_t&> oWhatIndex = null)
{
	PopFirstUntil(range, TIsOneOf(Zip(Iota(), whats)));
	index_t whatIndex = Count(whats);
	Ws whatsCopy = Forward<Ws>(whats);
	for(; !range.Empty(); range.PopFirst())
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
			if(oWhatIndex) oWhatIndex.Unwrap() = whatIndex;
			return range;
		}
	}
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
	typename AsWs = TRangeOfRef<Ws>
> constexpr Requires<
	CInputRange<R> && !CConst<R> &&
	CNonInfiniteForwardRange<AsWs> &&
	CEqualityComparable<TValueTypeOf<R>, TValueTypeOf<AsWs>>,
R&> PopFirstUntilAny(R&& range, Ws&& whats, Optional<index_t&> ioIndex = null, Optional<index_t&> oWhatIndex = null)
{
	auto whatsCopy = ForwardAsRange<Ws>(whats);
	return FindAdvanceAnyAdvance(range, whatsCopy, ioIndex, oWhatIndex);
}

//! Последовательно удаляет элементы из начала диапазона до тех пор,
//! пока не встретится элемент, равный любому из элементов диапазона whats.
//! @param whats[inout] Диапазон искомых элементов. Если элемент найден, начало диапазона whats смещается к найденному элементу.
//! @return Возвращает количество пройденных элементов.
template<class R, class Ws> constexpr Requires<
	CFiniteInputRange<R> && !CConst<R> &&
	CForwardRange<Ws> && !CConst<Ws> &&
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
template<class R, class Ws> constexpr Requires<
	CInputRange<R> && !CConst<R> &&
	CAsForwardRange<Ws> &&
	CEqualityComparable<TValueTypeOf<R>, TValueTypeOfAs<Ws>>,
index_t> CountUntilAdvanceAny(R&& range, Ws&& whats)
{
	index_t index = 0;
	PopFirstUntilAny(range, ForwardAsRange<Ws>(whats), OptRef(index));
	return index;
}

INTRA_END
