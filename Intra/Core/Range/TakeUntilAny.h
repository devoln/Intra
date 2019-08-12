#pragma once

#include "Core/Range/Concepts.h"



#include "Core/Range/Search/Single.h"
#include "Core/Range/Search/Subrange.h"
#include "Take.h"

INTRA_CORE_RANGE_BEGIN
//TODO: Implement class RTakeUntilAny for InputRange


//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
//! из диапазона поддиапазонов subranges в этот диапазон.
//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
//! с концом в случае, когда ни один поддиапазон не найден.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges окажется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
//! элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<typename R, typename RWs> INTRA_CONSTEXPR2 Requires<
	CFiniteForwardRange<RWs> &&
	!CConst<R> &&
	CFiniteForwardRange<TValueTypeOf<RWs>> &&
	!CConst<RWs>,
TTakeResult<R>> TakeUntilAdvanceAnyAdvance(R&& range,
	RWs&& subranges, size_t* ioIndex, size_t* oSubrangeIndex=null)
{
	auto rangeCopy = range;
	size_t index = CountUntilAdvanceAnyAdvance(range, subranges, oSubrangeIndex);
	if(ioIndex != null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! Последовательно удаляет элементы диапазона до тех пор, пока не:
//! 1) встретится элемент, равный одному из значений из whats
//! 2) будет достигнут конец диапазона.
//! Возвращает диапазон пройденных элементов.
template<class R, class Ws,
	typename AsWs = TRangeOfType<Ws>
> INTRA_CONSTEXPR2 Requires<
	CNonInfiniteForwardRange<R> &&
	!CConst<R> &&
	CNonInfiniteForwardRange<AsWs> &&
	CConvertible<TValueTypeOf<AsWs>, TValueTypeOf<R>>,
TTakeResult<R>> TakeUntilAdvanceAny(R& range, Ws&& whats, size_t* ioIndex=null, size_t* oWhatIndex = null)
{
	auto rangeCopy = range;
	size_t index = CountUntilAdvanceAny(range, ForwardAsRange<Ws>(whats), oWhatIndex);
	if(ioIndex != null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! Последовательно просматривает элементы диапазона до тех пор, пока не:
//! 1) встретится элемент, равный одному из значений из whats
//! 2) будет достигнут конец диапазона.
//! Возвращает диапазон пройденных элементов.
template<class R, class Ws,
	typename AsR = TRangeOfType<R>,
	typename AsWs = TRangeOfType<Ws>
> INTRA_NODISCARD INTRA_CONSTEXPR2 Requires<
	CNonInfiniteForwardRange<AsR> &&
	CNonInfiniteForwardRange<AsWs> &&
	CConvertible<TValueTypeOf<AsWs>, TValueTypeOf<AsR>>,
TTakeResult<R>> TakeUntilAny(R&& range, Ws&& whats,
	size_t* ioIndex=null, size_t* oWhatIndex = null)
{
	const size_t index = CountUntilAny(ForwardAsRange<R>(range), ForwardAsRange<Ws>(whats), oWhatIndex);
	if(ioIndex != null) *ioIndex += index;
	return Take(range, index);
}


//! Прочитать количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
//! с концом в случае, когда ни один поддиапазон не найден.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges окажется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
//! элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<class R, class RWs,
	typename AsRWs = TRangeOfType<RWs>
> INTRA_CONSTEXPR2 Requires<
	CNonInfiniteForwardRange<R> &&
	!CConst<R> &&
	CNonInfiniteForwardRange<AsRWs> &&
	CAsNonInfiniteForwardRange<TValueTypeOf<AsRWs>>,
TTakeResult<R>> TakeUntilAdvanceAny(R& range,
	RWs&& subranges, size_t* ioIndex = null, size_t* oSubrangeIndex = null)
{
	auto rangeCopy = range;
	size_t index = CountUntilAdvanceAny(range, ForwardAsRange<RWs>(subranges), oSubrangeIndex);
	if(ioIndex != null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
//! из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<class R, class RWs,
	typename AsR = TRangeOfType<R>
> INTRA_CONSTEXPR2 Requires<
	CNonInfiniteForwardRange<AsR> &&
	CNonInfiniteForwardRange<RWs> &&
	!CConst<RWs> &&
	CNonInfiniteForwardRange<TValueTypeOf<RWs>>,
TTakeResult<AsR>> TakeUntilAnyAdvance(R&& range,
	RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex = null)
{
	size_t index = CountUntilAnyAdvance(Range::RangeOf(range), subranges, oSubrangeIndex);
	if(ioIndex != null) *ioIndex += index;
	return Take(range, index);
}

//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
//! из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<class R, class RWs,
	typename AsR = TRangeOfType<R>,
	typename AsRWs = TRangeOfType<RWs>
> INTRA_NODISCARD INTRA_CONSTEXPR2 Requires<
	CNonInfiniteForwardRange<AsR> &&
	CNonInfiniteForwardRange<AsRWs> &&
	CAsNonInfiniteForwardRange<TValueTypeOf<AsRWs>>,
TTakeResult<R>> TakeUntilAny(R&& range, RWs&& subranges,
	size_t* ioIndex=null, size_t* oSubrangeIndex = null)
{
	const size_t index = CountUntilAny(RangeOf(range), ForwardAsRange<RWs>(subranges), oSubrangeIndex);
	if(ioIndex != null) *ioIndex += index;
	return Take(range, index);
}
INTRA_CORE_RANGE_END
