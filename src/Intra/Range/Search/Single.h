#pragma once

#include "Intra/Operations.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Take.h"
#include "Intra/Range/Operations.h"
#include "Intra/Functional.h"
#include "Intra/Range/Iota.h"

INTRA_BEGIN

/// Найти первое вхождение любого элемента из диапазона whats в этот диапазон.
/// Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
/// \param whats Диапазон искомых элементов. Если один из этих элементов найден,
/// начало whats устанавливается на найденный элемент. Иначе whats становится пустым
/// \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
/// \return Возвращает ссылку на себя.
template<class R, class Ws> requires(
	CRange<R> && !CConst<R> &&
	CNonInfiniteForwardRange<Ws> && !CConst<Ws> &&
	CEqualityComparable<TRangeValue<R>, TRangeValue<Ws>>)
constexpr R& PopFirstUntilAnyAdvance(R&& range, Ws& whats,
	Optional<index_t&> oWhatIndex = null)
{
#if 0 //TODO: this is a shorter functional implementation, not finished
	auto pred = IsOneOf(Zip(whats, IotaInf<index_t>()));
	PopFirstUntil(range, pred);
	if(oWhatIndex) oWhatIndex.Unwrap() = pred.Which().Field<1>();
#endif

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

/// Найти первое вхождение любого элемента из диапазона whats в этот диапазон.
/// Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
/// \param whats Диапазон искомых элементов.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
/// \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
/// \return Возвращает ссылку на себя.
template<class R, class Ws,
	typename AsWs = TRangeOfRef<Ws>
> requires(
	CRange<R> && !CConst<R> &&
	CNonInfiniteForwardRange<AsWs> &&
	CEqualityComparable<TRangeValue<R>, TRangeValue<AsWs>>)
constexpr R& PopFirstUntilAny(R&& range, Ws&& whats, Optional<index_t&> ioIndex = null, Optional<index_t&> oWhatIndex = null)
{
	auto whatsCopy = ForwardAsRange<Ws>(whats);
	return FindAdvanceAnyAdvance(range, whatsCopy, ioIndex, oWhatIndex);
}

/// Последовательно удаляет элементы из начала диапазона до тех пор,
/// пока не встретится элемент, равный любому из элементов диапазона whats.
/// @param whats[inout] Диапазон искомых элементов. Если элемент найден, начало диапазона whats смещается к найденному элементу.
/// @return Возвращает количество пройденных элементов.
template<class R, class Ws> requires(
	CFiniteRange<R> && !CConst<R> &&
	CForwardRange<Ws> && !CConst<Ws> &&
	CEqualityComparable<TRangeValue<R>, TRangeValue<Ws>>)
constexpr index_t CountUntilAdvanceAnyAdvance(R&& range, Ws&& whats)
{
	index_t index = 0;
	FindAdvanceAnyAdvance(range, whats, OptRef(index));
	return index;
}

/// Последовательно удаляет элементы из начала диапазона до тех пор,
/// пока не встретится элемент, равный любому из элементов диапазона whats.
/// @param whats[inout] Диапазон искомых элементов.
/// @return Возвращает количество пройденных элементов.
template<class R, class Ws> requires(
	CRange<R> && !CConst<R> &&
	CForwardList<Ws> &&
	CEqualityComparable<TRangeValue<R>, TListValue<Ws>>)
constexpr index_t CountUntilAdvanceAny(R&& range, Ws&& whats)
{
	index_t index = 0;
	PopFirstUntilAny(range, ForwardAsRange<Ws>(whats), OptRef(index));
	return index;
}

INTRA_END
