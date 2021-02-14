#pragma once

#include "Intra/Range/Concepts.h"

#include "Intra/Range/Search/Single.h"
#include "Intra/Range/Search/Subrange.h"
#include "Take.h"

namespace Intra { INTRA_BEGIN
//TODO: Implement a class RTakeUntilAny for InputRange

/// Прочитать количество символов, предшествующих первому вхождению любого диапазона
/// из диапазона поддиапазонов subranges в этот диапазон.
/// Начало этого диапазона смещается к найденному поддиапазону или совмещается
/// с концом в случае, когда ни один поддиапазон не найден.
/// @param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges окажется в исходном состоянии.
/// @param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
/// элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
/// @return Возвращает диапазон прочитанных элементов.
template<typename R, typename RWs> constexpr Requires<
	CFiniteForwardRange<RWs> &&
	!CConst<R> &&
	CFiniteForwardRange<TRangeValue<RWs>> &&
	!CConst<RWs>,
TTakeResult<R>> TakeUntilAdvanceAnyAdvance(R&& range,
	RWs&& subranges, size_t* ioIndex, size_t* oSubrangeIndex=nullptr)
{
	auto rangeCopy = range;
	size_t index = CountUntilAdvanceAnyAdvance(range, subranges, oSubrangeIndex);
	if(ioIndex != nullptr) *ioIndex += index;
	return Take(rangeCopy, index);
}

/// Последовательно удаляет элементы диапазона до тех пор, пока не:
/// 1) встретится элемент, равный одному из значений из whats
/// 2) будет достигнут конец диапазона.
/// @return диапазон пройденных элементов.
template<class R, class Ws,
	typename AsWs = TRangeOfRef<Ws>
> constexpr Requires<
	CNonInfiniteForwardRange<R> &&
	!CConst<R> &&
	CNonInfiniteForwardRange<AsWs> &&
	CConvertibleTo<TRangeValue<AsWs>, TRangeValue<R>>,
TTakeResult<R>> TakeUntilAdvanceAny(R& range, Ws&& whats, size_t* ioIndex=nullptr, size_t* oWhatIndex = nullptr)
{
	auto rangeCopy = range;
	size_t index = CountUntilAdvanceAny(range, ForwardAsRange<Ws>(whats), oWhatIndex);
	if(ioIndex != nullptr) *ioIndex += index;
	return Take(rangeCopy, index);
}

/// Последовательно просматривает элементы диапазона до тех пор, пока не:
/// 1) встретится элемент, равный одному из значений из whats
/// 2) будет достигнут конец диапазона.
/// @return диапазон пройденных элементов.
template<class R, class Ws,
	typename AsR = TRangeOfRef<R>,
	typename AsWs = TRangeOfRef<Ws>
> [[nodiscard]] constexpr Requires<
	CNonInfiniteForwardRange<AsR> &&
	CNonInfiniteForwardRange<AsWs> &&
	CConvertibleTo<TRangeValue<AsWs>, TRangeValue<AsR>>,
TTakeResult<R>> TakeUntilAny(R&& range, Ws&& whats,
	size_t* ioIndex=nullptr, size_t* oWhatIndex = nullptr)
{
	const size_t index = CountUntilAny(ForwardAsRange<R>(range), ForwardAsRange<Ws>(whats), oWhatIndex);
	if(ioIndex != nullptr) *ioIndex += index;
	return Take(range, index);
}


/// Прочитать количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
/// Начало этого диапазона смещается к найденному поддиапазону или совмещается
/// с концом в случае, когда ни один поддиапазон не найден.
/// @param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges окажется в исходном состоянии.
/// @param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
/// элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
/// @return Возвращает диапазон прочитанных элементов.
template<class R, class RWs,
	typename AsRWs = TRangeOfRef<RWs>
> constexpr Requires<
	CNonInfiniteForwardRange<R> &&
	!CConst<R> &&
	CNonInfiniteForwardRange<AsRWs> &&
	CNonInfiniteForwardList<TRangeValue<AsRWs>>,
TTakeResult<R>> TakeUntilAdvanceAny(R& range,
	RWs&& subranges, Optional<index_t&> ioIndex = nullptr, Optional<index_t&> oSubrangeIndex = nullptr)
{
	auto rangeCopy = range;
	const index_t index = CountUntilAdvanceAny(range, ForwardAsRange<RWs>(subranges), oSubrangeIndex);
	if(ioIndex) ioIndex.Unwrap() += index;
	return Take(rangeCopy, index);
}

/// Прочитать количество символов, предшествующих первому вхождению любого диапазона
/// из диапазона поддиапазонов subranges в этот диапазон.
/// @param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges останется в исходном состоянии.
/// @param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
/// @return Возвращает диапазон прочитанных элементов.
template<class R, class RWs,
	typename AsR = TRangeOfRef<R>
> constexpr Requires<
	CNonInfiniteForwardRange<AsR> &&
	CNonInfiniteForwardRange<RWs> &&
	!CConst<RWs> &&
	CNonInfiniteForwardRange<TRangeValue<RWs>>,
TTakeResult<AsR>> TakeUntilAnyAdvance(R&& range,
	RWs& subranges, Optional<index_t&> ioIndex = nullptr, Optional<index_t&> oSubrangeIndex = nullptr)
{
	const index_t index = CountUntilAnyAdvance(RangeOf(range), subranges, oSubrangeIndex);
	if(ioIndex) ioIndex.Unwrap() += index;
	return Take(range, index);
}

/// Прочитать количество символов, предшествующих первому вхождению любого диапазона
/// из диапазона поддиапазонов subranges в этот диапазон.
/// @param subranges[inout] Диапазон искомых поддиапазонов.
/// После вызова этой функции начало subranges смещается к первому совпавшему элементу.
/// Если совпадений не найдено, subranges останется в исходном состоянии.
/// @param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
/// @return Возвращает диапазон прочитанных элементов.
template<class R, class RWs,
	typename AsR = TRangeOfRef<R>,
	typename AsRWs = TRangeOfRef<RWs>
> [[nodiscard]] constexpr Requires<
	CNonInfiniteForwardRange<AsR> &&
	CNonInfiniteForwardRange<AsRWs> &&
	CNonInfiniteForwardList<TRangeValue<AsRWs>>,
TTakeResult<R>> TakeUntilAny(R&& range, RWs&& subranges,
	Optional<index_t&> ioIndex = nullptr, Optional<index_t&> oSubrangeIndex = nullptr)
{
	const size_t index = CountUntilAny(RangeOf(range), ForwardAsRange<RWs>(subranges), oSubrangeIndex);
	if(ioIndex) ioIndex.Unwrap() += index;
	return Take(range, index);
}
} INTRA_END
