#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Take.h"
#include "Intra/Range/Transversal.h"
#include "Intra/Range/Comparison.h"
#include "Intra/Range/Search/Single.h"

INTRA_BEGIN
/// Найти первое вхождение диапазона what в этот диапазон.
/// Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
/// \param what Искомый диапазон.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
/// \returns Возвращает ссылку на себя.
template<typename R, typename RW> constexpr Requires<
	CRange<R> &&
	!CConst<R> &&
	CNonInfiniteForwardRange<RW> &&
	CEqualityComparable<TRangeValue<RW>, TRangeValue<R>>,
R&> FindAdvance(R& range, const RW& what, Optional<index_t&> ioIndex = null)
{
	while(!range.Empty() && !StartsWith(range, what))
	{
		range.PopFirst();
		if(ioIndex) ioIndex.Unwrap()++;
		FindAdvance(range, what.First(), ioIndex);
	}
	return range;
}


/// Найти первое вхождение диапазона what в этот диапазон.
/// \param what Искомый диапазон.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество
/// элементов, предшествующих найденной позиции. Может быть null.
/// \returns null, если значение не найдено. Часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
template<typename R, typename RW,
	typename AsR = TRangeOf<R>,
	typename AsRW = TRangeOfRef<RW>
> constexpr Requires<
	CConsumableRange<AsR> &&
	CNonInfiniteForwardRange<AsRW> &&
	CEqualityComparable<TRangeValue<AsRW>, TRangeValue<AsR>>,
AsR> Find(R&& range, RW&& what, Optional<index_t&> ioIndex = null)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	FindAdvance(rangeCopy, ForwardAsRange<RW>(what), ioIndex);
	return rangeCopy;
}

/// Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// Начало этого диапазона смещается к найденному поддиапазону или совмещается
/// с концом в случае, когда ни один из поддиапазонов не найден.
/// \param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges окажется в исходном состоянии.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество
/// элементов, предшествующих найденной позиции. Может быть null.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
/// элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
/// \return Возвращает ссылку на себя.
template<typename R, typename RWs> constexpr Requires<
	CNonInfiniteForwardRange<R> &&
	!CConst<R> &&
	CNonInfiniteForwardRange<RWs> &&
	!CConst<RWs> &&
	CNonInfiniteForwardList<TRangeValue<RWs>>,
R&> FindAdvanceAnyAdvance(R& range, RWs& subranges, Optional<index_t&> ioIndex = null, Optional<index_t&> oSubrangeIndex = null)
{
	auto subrangesCopy = subranges;
	while(!range.Empty() && !StartsWithAnyAdvance(range, subranges, oSubrangeIndex))
	{
		subranges = subrangesCopy;
		range.PopFirst();
		if(ioIndex) ioIndex.Unwrap()++;
		FindAdvanceAny(range, FirstTransversal(subranges), ioIndex);
	}
	return range;
}

/// Найти количество символов, предшествующих первому вхождению любого диапазона
/// из диапазона поддиапазонов subranges в этот диапазон.
/// Начало этого диапазона смещается к найденному поддиапазону или совмещается
/// с концом в случае, когда ни один поддиапазон не найден.
/// \param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges окажется в исходном состоянии.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
/// элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
/// \return Возвращает количество пройденных элементов.
template<typename R, typename RWs> constexpr Requires<
	CNonInfiniteForwardRange<R> &&
	CNonInfiniteForwardRange<RWs> &&
	CNonInfiniteForwardList<TRangeValue<RWs>>,
index_t> CountUntilAdvanceAnyAdvance(R& range, RWs& subranges, Optional<index_t&> oSubrangeIndex = null)
{
	index_t index = 0;
	FindAdvanceAnyAdvance(range, subranges, OptRef(index), oSubrangeIndex);
	return index;
}


/// Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
/// \param subranges Диапазон искомых поддиапазонов.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается на
/// количество элементов, предшествующих найденной позиции. Может быть null.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
/// элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
/// \return Возвращает ссылку на себя.
template<typename R, typename RWs,
	typename AsRWs = TRangeOfRef<RWs>
> constexpr Requires<
	CNonInfiniteForwardRange<R> &&
	CNonInfiniteForwardRange<AsRWs> &&
	CNonInfiniteForwardList<TRangeValue<AsRWs>>,
R&> FindAdvanceAny(R& range, RWs&& subranges, Optional<index_t&> ioIndex = null, Optional<index_t&> oSubrangeIndex = null)
{
	auto subrangesCopy = ForwardAsRange<RWs>(subranges);
	return FindAdvanceAnyAdvance(range, subrangesCopy, ioIndex, oSubrangeIndex);
}

/// Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// Начало этого диапазона смещается к найденному поддиапазону или совмещается
/// с концом в случае, когда ни один поддиапазон не найден.
/// \param subranges[inout] Диапазон искомых поддиапазонов.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
/// элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
/// \return Возвращает количество пройденных элементов.
template<typename R, typename RWs,
	typename AsRWs = TRangeOfRef<RWs>
> constexpr Requires<
	CNonInfiniteForwardRange<R> &&
	CNonInfiniteForwardRange<AsRWs> &&
	CNonInfiniteForwardList<TRangeValue<AsRWs>>,
index_t> CountUntilAdvanceAny(R& range, RWs&& subranges, Optional<index_t&> oSubrangeIndex = null)
{
	index_t index = 0;
	FindAdvanceAny(range, ForwardAsRange<RWs>(subranges), OptRef(index), oSubrangeIndex);
	return index;
}



/// Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// \param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges окажется в исходном состоянии.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается
/// на количество элементов, предшествующих найденной позиции. Может быть null.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
/// \return Возвращает диапазон, полученный из этого удалением всех элементов до первого вхождения любого из искомых диапазонов.
template<typename R, typename RWs,
	typename AsR = TRangeOf<R>
> INTRA_FORCEINLINE Requires<
	CNonInfiniteForwardRange<AsR> &&
	CNonInfiniteForwardRange<RWs> &&
	!CConst<RWs> &&
	CNonInfiniteForwardList<TRangeValue<RWs>>,
AsR> FindAnyAdvance(R&& range, RWs& subranges,
	Optional<index_t&> ioIndex = null, Optional<index_t&> oSubrangeIndex = null)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	FindAdvanceAnyAdvance(rangeCopy, subranges, ioIndex, oSubrangeIndex);
	return rangeCopy;
}

/// Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// \param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges останется в исходном состоянии.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс
/// найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
/// \return Возвращает количество пройденных элементов.
template<typename R, typename RWs> constexpr Requires<
	CNonInfiniteForwardList<R> &&
	CNonInfiniteForwardRange<RWs> &&
	!CConst<RWs> &&
	CNonInfiniteForwardList<TRangeValue<RWs>>,
index_t> CountUntilAnyAdvance(R&& range, RWs& subranges, Optional<index_t&> oSubrangeIndex = null)
{
	index_t index = 0;
	FindAnyAdvance(ForwardAsRange<R>(range), subranges, &index, oSubrangeIndex);
	return index;
}



/// Найти первое вхождение любого поддиапазона из диапазона subranges в этот диапазон.
/// \param subranges Искомые поддиапазоны.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
/// \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного поддиапазона
/// в диапазоне subranges. Если элемент не был найден, будет записано значение whats.Count().
template<typename R, typename RWs,
	typename AsR = TRangeOf<R>,
	typename AsRWs = TRangeOfRef<RWs>
> [[nodiscard]] constexpr Requires<
	CNonInfiniteForwardRange<AsR> &&
	CNonInfiniteForwardRange<AsRWs> &&
	CNonInfiniteForwardList<TRangeValue<AsRWs>>,
AsR> FindAny(R&& range, RWs&& subranges,
	Optional<index_t&> ioIndex = null, Optional<index_t&> oWhatIndex = null)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	FindAdvanceAny(rangeCopy, ForwardAsRange<RWs>(subranges), ioIndex, oWhatIndex);
	return rangeCopy;
}

/// Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// \param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges останется в исходном состоянии.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс
/// найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
/// \return Возвращает количество пройденных элементов.
template<typename R, typename RWs,
	typename AsR = TRangeOfRef<R>,
	typename AsRWs = TRangeOfRef<RWs>
> constexpr Requires<
	CNonInfiniteForwardRange<AsR> &&
	CNonInfiniteForwardRange<AsRWs> &&
	CNonInfiniteForwardList<TRangeValue<AsRWs>>,
index_t> CountUntilAny(R&& range, RWs&& subranges, Optional<index_t&> oSubrangeIndex = null)
{
	index_t index = 0;
	FindAny(ForwardAsRange<R>(range), ForwardAsRange<RWs>(subranges), &index, oSubrangeIndex);
	return index;
}




/// Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
/// Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
/// \param what Искомый диапазон.
/// \returns Возвращает количество пройденных элементов.
template<typename R, typename RW,
	typename AsRW = TRangeOfRef<RW>
> constexpr Requires<
	CNonInfiniteForwardRange<R> &&
	!CConst<R> &&
	CNonInfiniteForwardList<RW> &&
	CEqualityComparable<TRangeValue<AsRW>, TRangeValue<R>>,
index_t> CountUntilAdvance(R& range, RW&& what)
{
	index_t index = 0;
	FindAdvance(range, ForwardAsRange<RW>(what), OptRef(index));
	return index;
}

/// Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
/// \param what Искомый диапазон.
/// \returns Возвращает количество пройденных элементов.
template<typename R, typename RW,
	typename AsR = TRangeOfRef<R>,
	typename AsRW = TRangeOfRef<RW>
> [[nodiscard]] constexpr Requires<
	CNonInfiniteForwardRange<AsR> &&
	CNonInfiniteForwardRange<AsRW> &&
	CEqualityComparable<TRangeValue<AsRW>, TRangeValue<AsR>>,
index_t> CountUntil(R&& range, RW&& what)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return CountUntilAdvance(rangeCopy, ForwardAsRange<RW>(what));
}
	

template<typename R, typename RW> [[nodiscard]] constexpr Requires<
	CNonInfiniteForwardList<R> &&
	CNonInfiniteForwardList<RW>,
bool> Contains(R&& range, RW&& what)
{return !Find(ForwardAsRange<R>(range), ForwardAsRange<RW>(what)).Empty();}

template<typename R, typename RW> Requires<
	CNonInfiniteForwardRange<R> &&
	!CConst<R> &&
	CNonInfiniteForwardRange<RW>,
index_t> CountAdvance(R& range, const RW& what)
{
	index_t result = 0;
	const index_t whatCount = Count(what);
	while(FindAdvance(range, what), !range.Empty())
	{
		PopFirstExactly(range, whatCount);
		result++;
	}
	return result;
}

template<typename R, typename RW> INTRA_FORCEINLINE Requires<
	CNonInfiniteForwardList<R> &&
	CNonInfiniteForwardList<RW>,
index_t> Count(R&& range, RW&& what)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return CountAdvance(rangeCopy, ForwardAsRange<RW>(what));
}
INTRA_END
