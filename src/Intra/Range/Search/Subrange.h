#pragma once

#include <Intra/Concepts.h>
#include <Intra/Range.h>

#include "Intra/Range/Comparison.h"

namespace Intra { INTRA_BEGIN
/// Найти первое вхождение диапазона what в этот диапазон.
/// Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
/// \param what Искомый диапазон.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть nullptr.
/// \returns Возвращает ссылку на себя.
template<CRange R, CNonInfiniteForwardRange RW> requires (!CConst<R> && CEqualityComparable<TRangeValue<RW>, TRangeValue<R>>)
constexpr R& FindAdvance(R& range, const RW& what, Optional<index_t&> ioIndex = {})
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
/// элементов, предшествующих найденной позиции. Может быть nullptr.
/// \returns nullptr, если значение не найдено. Часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
template<CConsumableList L, CNonInfiniteForwardRange LW> requires CEqualityComparable<TListValue<LW>, TListValue<L>>
constexpr auto Find(L&& range, LW&& what, Optional<index_t&> ioIndex = {})
{
	auto rangeCopy = RangeOf(INTRA_FWD(range));
	FindAdvance(rangeCopy, RangeOf(INTRA_FWD(what)), ioIndex);
	return rangeCopy;
}

/// Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// Начало этого диапазона смещается к найденному поддиапазону или совмещается
/// с концом в случае, когда ни один из поддиапазонов не найден.
/// \param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges окажется в исходном состоянии.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество
/// элементов, предшествующих найденной позиции. Может быть nullptr.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
/// элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
/// \return Возвращает ссылку на себя.
template<CNonInfiniteForwardRange R, CNonInfiniteForwardRange RWs> requires (!CConst<R> && !CConst<RWs> && CNonInfiniteForwardList<TRangeValue<RWs>>)
constexpr R& FindAdvanceAnyAdvance(R& range, RWs& subranges, Optional<index_t&> ioIndex = {}, Optional<index_t&> oSubrangeIndex = {})
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
template<CNonInfiniteForwardRange R, CNonInfiniteForwardRange RWs> requires CNonInfiniteForwardList<TRangeValue<RWs>>
constexpr index_t CountUntilAdvanceAnyAdvance(R& range, RWs& subranges, Optional<index_t&> oSubrangeIndex = {})
{
	index_t index = 0;
	FindAdvanceAnyAdvance(range, subranges, OptRef(index), oSubrangeIndex);
	return index;
}


/// Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
/// \param subranges Диапазон искомых поддиапазонов.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается на
/// количество элементов, предшествующих найденной позиции. Может быть nullptr.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
/// элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
/// \return Возвращает ссылку на себя.
template<CNonInfiniteForwardRange R, CNonInfiniteForwardList LWs> requires CNonInfiniteForwardList<TListValue<LWs>>
constexpr R& FindAdvanceAny(R& range, LWs&& subranges, Optional<index_t&> ioIndex = {}, Optional<index_t&> oSubrangeIndex = {})
{
	auto subrangesCopy = RangeOf(INTRA_FWD(subranges));
	return FindAdvanceAnyAdvance(range, subrangesCopy, ioIndex, oSubrangeIndex);
}

/// Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// Начало этого диапазона смещается к найденному поддиапазону или совмещается
/// с концом в случае, когда ни один поддиапазон не найден.
/// \param subranges[inout] Диапазон искомых поддиапазонов.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
/// элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
/// \return Возвращает количество пройденных элементов.
template<CNonInfiniteForwardList L, CNonInfiniteForwardList LWs> requires CNonInfiniteForwardList<TListValue<LWs>>
constexpr index_t CountUntilAdvanceAny(L&& range, LWs&& subranges, Optional<index_t&> oSubrangeIndex = {})
{
	index_t index = 0;
	FindAdvanceAny(range, RangeOf(INTRA_FWD(subranges)), OptRef(index), oSubrangeIndex);
	return index;
}



/// Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// \param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges окажется в исходном состоянии.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается
/// на количество элементов, предшествующих найденной позиции. Может быть nullptr.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
/// \return Возвращает диапазон, полученный из этого удалением всех элементов до первого вхождения любого из искомых диапазонов.
template<CNonInfiniteForwardList L, CNonInfiniteForwardList LWs> requires (!CConst<LWs> && CNonInfiniteForwardList<TListValue<LWs>>)
INTRA_FORCEINLINE L FindAnyAdvance(L&& range, LWs& subranges,
	Optional<index_t&> ioIndex = {}, Optional<index_t&> oSubrangeIndex = {})
{
	auto rangeCopy = RangeOf(INTRA_FWD(range));
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
template<CNonInfiniteForwardList L, CNonInfiniteForwardRange RWs> requires (!CConst<RWs> && CNonInfiniteForwardList<TRangeValue<RWs>>)
constexpr index_t CountUntilAnyAdvance(L&& range, RWs& subranges, Optional<index_t&> oSubrangeIndex = {})
{
	index_t index = 0;
	FindAnyAdvance(RangeOf(INTRA_FWD(range)), subranges, &index, oSubrangeIndex);
	return index;
}



/// Найти первое вхождение любого поддиапазона из диапазона subranges в этот диапазон.
/// \param subranges Искомые поддиапазоны.
/// \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть nullptr.
/// \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного поддиапазона
/// в диапазоне subranges. Если элемент не был найден, будет записано значение whats.Count().
template<CNonInfiniteForwardList L, CNonInfiniteForwardList LWs> requires CNonInfiniteForwardList<TListValue<LWs>>
[[nodiscard]] constexpr auto FindAny(L&& range, LWs&& subranges,
	Optional<index_t&> ioIndex = {}, Optional<index_t&> oWhatIndex = {})
{
	auto rangeCopy = RangeOf(INTRA_FWD(range));
	FindAdvanceAny(rangeCopy, RangeOf(INTRA_FWD(subranges)), ioIndex, oWhatIndex);
	return rangeCopy;
}

/// Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// \param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges останется в исходном состоянии.
/// \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс
/// найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
/// \return Возвращает количество пройденных элементов.
template<CNonInfiniteForwardList L, CNonInfiniteForwardList LWs> requires CNonInfiniteForwardList<TListValue<LWs>>
constexpr index_t CountUntilAny(L&& range, LWs&& subranges, Optional<index_t&> oSubrangeIndex = {})
{
	index_t index = 0;
	FindAny(RangeOf(INTRA_FWD(range)), RangeOf(INTRA_FWD(subranges)), &index, oSubrangeIndex);
	return index;
}




/// Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
/// Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
/// \param what Искомый диапазон.
/// \returns Возвращает количество пройденных элементов.
template<CNonInfiniteForwardRange R, CNonInfiniteForwardList LW> requires (CEqualityComparable<TListValue<LW>, TRangeValue<R>>)
constexpr index_t CountUntilAdvance(R& range, LW&& what)
{
	index_t index = 0;
	FindAdvance(range, RangeOf(INTRA_FWD(what)), OptRef(index));
	return index;
}

/// Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
/// \param what Искомый диапазон.
/// \returns Возвращает количество пройденных элементов.
template<CNonInfiniteForwardList L, CNonInfiniteForwardList LW> requires (CEqualityComparable<TListValue<L>, TListValue<LW>>)
[[nodiscard]] constexpr index_t CountUntil(L&& range, LW&& what)
{
	auto rangeCopy = RangeOf(INTRA_FWD(range));
	return CountUntilAdvance(rangeCopy, RangeOf(INTRA_FWD(what)));
}
	

template<CNonInfiniteForwardList L, CNonInfiniteForwardList LW>
[[nodiscard]] constexpr bool ContainsSubrange(L&& range, LW&& what)
{return !Find(RangeOf(INTRA_FWD(range)), RangeOf(INTRA_FWD(what))).Empty();}

template<CNonInfiniteForwardRange R, CNonInfiniteForwardRange RW> requires (!CConst<R>)
index_t CountAdvance(R& range, const RW& what)
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

template<CNonInfiniteForwardList L, CNonInfiniteForwardList LW>
INTRA_FORCEINLINE index_t CountSubrangeOccurences(L&& range, LW&& what)
{
	auto rangeCopy = RangeOf(INTRA_FWD(range));
	return CountAdvance(rangeCopy, RangeOf(INTRA_FWD(what)));
}
} INTRA_END
