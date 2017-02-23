﻿#pragma once

#include "Range/Concepts.h"
#include "Range/Decorators/Take.h"
#include "Range/Decorators/Transversal.h"
#include "Single.h"
#include "Algo/Comparison/StartsWith.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Найти первое вхождение диапазона what в этот диапазон.
//! Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
//! \param what Искомый диапазон.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \returns Возвращает ссылку на себя.
template<typename R, typename RW> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsNonInfiniteForwardRange<RW>::_ &&
	Meta::IsConvertible<ValueTypeOf<RW>, ValueTypeOf<R>>::_,
R&> FindAdvance(R& range, const RW& what, size_t* ioIndex=null)
{
	while(!range.Empty() && !StartsWith(range, what))
	{
		range.PopFirst();
		if(ioIndex!=null) ++*ioIndex;
		FindAdvance(range, what.First(), ioIndex);
	}
	return range;
}


//! Найти первое вхождение диапазона what в этот диапазон.
//! \param what Искомый диапазон.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество
//! элементов, предшествующих найденной позиции. Может быть null.
//! \returns null, если значение не найдено. Часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
template<typename R, typename RW> forceinline Meta::EnableIf<
	IsAsConsumableRange<R>::_ &&
	IsAsNonInfiniteForwardRange<RW>::_ &&
	Meta::IsConvertible<ValueTypeOfAs<RW>, ValueTypeOfAs<R>>::_,
Meta::RemoveConstRef<AsRangeResult<R>>> Find(R&& range, RW&& what, size_t* ioIndex=null)
{
	auto rangeCopy = Range::Forward<R>(range);
	FindAdvance(rangeCopy, Range::Forward<RW>(what), ioIndex);
	return rangeCopy;
}

//! Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
//! с концом в случае, когда ни один из поддиапазонов не найден.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges окажется в исходном состоянии.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество
//! элементов, предшествующих найденной позиции. Может быть null.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
//! элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
//! \return Возвращает ссылку на себя.
template<typename R, typename RWs> Meta::EnableIf<
	IsNonInfiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsNonInfiniteForwardRange<RWs>::_ && !Meta::IsConst<RWs>::_ &&
	IsAsNonInfiniteForwardRange<ValueTypeOf<RWs>>::_,
R&> FindAdvanceAnyAdvance(R& range, RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	auto subrangesCopy = subranges;
	while(!range.Empty() && !StartsWithAnyAdvance(range, subranges, oSubrangeIndex))
	{
		subranges = subrangesCopy;
		range.PopFirst();
		if(ioIndex!=null) ++*ioIndex;
		FindAdvanceAny(range, Range::FirstTransversal(subranges), ioIndex);
	}
	return range;
}

//! Найти количество символов, предшествующих первому вхождению любого диапазона
//! из диапазона поддиапазонов subranges в этот диапазон.
//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
//! с концом в случае, когда ни один поддиапазон не найден.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges окажется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
//! элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает количество пройденных элементов.
template<typename R, typename RWs> Meta::EnableIf<
	IsNonInfiniteForwardRange<R>::_ &&
	IsNonInfiniteForwardRange<RWs>::_ &&
	IsAsNonInfiniteForwardRange<ValueTypeOf<RWs>>::_,
size_t> CountUntilAdvanceAnyAdvance(R& range, RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAdvanceAnyAdvance(range, subranges, &index, oSubrangeIndex);
	return index;
}


//! Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
//! \param subranges Диапазон искомых поддиапазонов.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на
//! количество элементов, предшествующих найденной позиции. Может быть null.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
//! элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
//! \return Возвращает ссылку на себя.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	IsNonInfiniteForwardRange<R>::_ &&
	IsAsNonInfiniteForwardRange<RWs>::_ &&
	IsAsNonInfiniteForwardRange<ValueTypeOfAs<RWs>>::_,
R&> FindAdvanceAny(R& range, RWs&& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	auto subrangesCopy = Range::Forward<RWs>(subranges);
	return FindAdvanceAnyAdvance(range, subrangesCopy, ioIndex, oSubrangeIndex);
}

//! Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
//! с концом в случае, когда ни один поддиапазон не найден.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
//! элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает количество пройденных элементов.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	IsNonInfiniteForwardRange<R>::_ &&
	IsAsNonInfiniteForwardRange<RWs>::_ &&
	IsAsNonInfiniteForwardRange<ValueTypeOfAs<RWs>>::_,
size_t> CountUntilAdvanceAny(R& range, RWs&& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAdvanceAny(range, Range::Forward<RWs>(subranges), &index, oSubrangeIndex);
	return index;
}



//! Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges окажется в исходном состоянии.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается
//! на количество элементов, предшествующих найденной позиции. Может быть null.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
//! \return Возвращает диапазон, полученный из этого удалением всех элементов до первого вхождения любого из искомых диапазонов.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsNonInfiniteForwardRange<RWs>::_ && !Meta::IsConst<RWs>::_ &&
	IsAsNonInfiniteForwardRange<ValueTypeOf<RWs>>::_,
Meta::RemoveConstRef<AsRangeResult<R>>> FindAnyAdvance(R&& range,
	RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	auto rangeCopy = Range::Forward<R>(range);
	FindAdvanceAnyAdvance(rangeCopy, subranges, ioIndex, oSubrangeIndex);
	return rangeCopy;
}

//! Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс
//! найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает количество пройденных элементов.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsNonInfiniteForwardRange<RWs>::_ && !Meta::IsConst<RWs>::_ &&
	IsAsNonInfiniteForwardRange<ValueTypeOf<RWs>>::_,
size_t> CountUntilAnyAdvance(R&& range, RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAnyAdvance(Range::Forward<R>(range), subranges, &index, oSubrangeIndex);
	return index;
}



//! Найти первое вхождение любого поддиапазона из диапазона subranges в этот диапазон.
//! \param subranges Искомые поддиапазоны.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного поддиапазона
//! в диапазоне subranges. Если элемент не был найден, будет записано значение whats.Count().
template<typename R, typename RWs> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsAsNonInfiniteForwardRange<RWs>::_ &&
	IsAsNonInfiniteForwardRange<Range::ValueTypeOfAs<RWs>>::_,
AsRangeResultNoCRef<R>> FindAny(R&& range, RWs&& subranges,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	auto rangeCopy = Range::Forward<R>(range);
	FindAdvanceAny(rangeCopy, Range::Forward<RWs>(subranges), ioIndex, oWhatIndex);
	return rangeCopy;
}

//! Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс
//! найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает количество пройденных элементов.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsAsNonInfiniteForwardRange<RWs>::_ &&
	IsAsNonInfiniteForwardRange<ValueTypeOfAs<RWs>>::_,
size_t> CountUntilAny(R&& range, RWs&& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAny(Range::Forward<R>(range), Range::Forward<RWs>(subranges), &index, oSubrangeIndex);
	return index;
}




//! Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
//! Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
//! \param what Искомый диапазон.
//! \returns Возвращает количество пройденных элементов.
template<typename R, typename RW> forceinline Meta::EnableIf<
	IsNonInfiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsNonInfiniteForwardRange<RW>::_ &&
	Meta::IsConvertible<ValueTypeOfAs<RW>, ValueTypeOf<R>>::_,
size_t> CountUntilAdvance(R& range, RW&& what)
{
	size_t index=0;
	FindAdvance(range, Range::Forward<RW>(what), &index);
	return index;
}

//! Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
//! \param what Искомый диапазон.
//! \returns Возвращает количество пройденных элементов.
template<typename R, typename RW> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsAsNonInfiniteForwardRange<RW>::_ &&
	Meta::IsConvertible<ValueTypeOfAs<RW>, ValueTypeOfAs<R>>::_,
size_t> CountUntil(R&& range, RW&& what)
{
	auto rangeCopy = Range::Forward<R>(range);
	return CountUntilAdvance(rangeCopy, Range::Forward<RW>(what));
}
	

template<typename R, typename RW> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsAsNonInfiniteForwardRange<RW>::_,
bool> Contains(R&& range, RW&& what)
{return !Find(Range::Forward<R>(range), Range::Forward<RW>(what)).Empty();}

template<typename R, typename RW> Meta::EnableIf<
	IsNonInfiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsNonInfiniteForwardRange<RW>::_,
size_t> CountAdvance(R& range, const RW& what)
{
	size_t result = 0;
	size_t whatCount = Range::Count(what);
	while(FindAdvance(range, what), !range.Empty())
	{
		Range::PopFirstExactly(range, whatCount);
		result++;
	}
	return result;
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsAsNonInfiniteForwardRange<RW>::_,
size_t> Count(R&& range, RW&& what)
{
	auto rangeCopy = Range::Forward<R>(range);
	return CountAdvance(rangeCopy, Range::Forward<RW>(what));
}

INTRA_WARNING_POP

}}
