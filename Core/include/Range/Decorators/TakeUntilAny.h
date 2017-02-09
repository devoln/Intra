#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Algo/Search/Single.h"
#include "Algo/Search/Subrange.h"
#include "Take.h"

namespace Intra { namespace Range {

//TODO: Реализовать класс TakeUntilAnyResult для InputRange


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
template<typename R, typename RWs> Meta::EnableIf<
	IsFiniteForwardRange<RWs>::_ && !Meta::IsConst<R>::_ &&
	IsFiniteForwardRange<ValueTypeOf<RWs>>::_ && !Meta::IsConst<RWs>::_,
ResultOfTake<R>> TakeUntilAdvanceAnyAdvance(R&& range,
	RWs&& subranges, size_t* ioIndex, size_t* oSubrangeIndex=null)
{
	auto rangeCopy = range;
	size_t index = Algo::CountUntilAdvanceAnyAdvance(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! Последовательно удаляет элементы диапазона до тех пор, пока не:
//! 1) встретится элемент, равный одному из значений из whats
//! 2) будет достигнут конец диапазона.
//! Возвращает диапазон пройденных элементов.
template<class R, class Ws> forceinline Meta::EnableIf<
	IsNonInfiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsNonInfiniteForwardRange<Ws>::_ &&
	Meta::IsConvertible<ValueTypeOfAs<Ws>, ValueTypeOf<R>>::_,
ResultOfTake<R>> TakeUntilAdvanceAny(R& range, Ws&& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	auto rangeCopy = range;
	size_t index = Algo::CountUntilAdvanceAny(range, Range::Forward<Ws>(whats), oWhatIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! Последовательно просматривает элементы диапазона до тех пор, пока не:
//! 1) встретится элемент, равный одному из значений из whats
//! 2) будет достигнут конец диапазона.
//! Возвращает диапазон пройденных элементов.
template<class R, class Ws> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsAsNonInfiniteForwardRange<Ws>::_ &&
	Meta::IsConvertible<ValueTypeOfAs<Ws>, ValueTypeOfAs<R>>::_,
ResultOfTake<R>> TakeUntilAny(R&& range, Ws&& whats,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	size_t index = Algo::CountUntilAny(Range::Forward<R>(range), Range::Forward<Ws>(whats), oWhatIndex);
	if(ioIndex!=null) *ioIndex += index;
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
template<class R, class RWs> Meta::EnableIf<
	IsNonInfiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsNonInfiniteForwardRange<RWs>::_ &&
	IsAsNonInfiniteForwardRange<ValueTypeOfAs<RWs>>::_,
ResultOfTake<R>> TakeUntilAdvanceAny(R& range,
	RWs&& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	auto rangeCopy = range;
	size_t index = Algo::CountUntilAdvanceAny(range, Range::Forward<RWs>(subranges), oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
//! из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<class R, class RWs> Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsNonInfiniteForwardRange<RWs>::_ && !Meta::IsConst<RWs>::_ &&
	IsNonInfiniteForwardRange<ValueTypeOf<RWs>>::_,
ResultOfTake<R>> TakeUntilAnyAdvance(R&& range,
	RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	size_t index = Algo::CountUntilAnyAdvance(Range::AsRange(range), subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(range, index);
}

//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
//! из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<class R, class RWs> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsAsNonInfiniteForwardRange<RWs>::_ &&
	IsAsNonInfiniteForwardRange<ValueTypeOfAs<RWs>>::_,
ResultOfTake<R>> TakeUntilAny(R&& range, RWs&& subranges,
	size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	const size_t index = CountUntilAny(Range::AsRange(range), Range::Forward<RWs>(subranges), oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(range, index);
}

}}
