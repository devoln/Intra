#pragma once

#include "Range/Concepts.h"
#include "Range/Construction/Take.h"
#include "Range/Iteration/Transversal.h"
#include "Single.h"
#include "Algo/Comparison.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Найти первое вхождение диапазона what в этот диапазон.
//! Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
//! \param what Искомый диапазон.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \returns Возвращает ссылку на себя.
template<typename R, typename RW> Meta::EnableIf<
	Range::IsFiniteForwardRange<RW>::_ && !Meta::IsConst<R>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_,
R&> FindAdvance(R&& range, const RW& what, size_t* ioIndex=null)
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
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RW>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_,
R> Find(const R& range, const RW& what, size_t* ioIndex=null)
{return FindAdvance(R(range), what, ioIndex);}

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
	Range::IsFiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ && !Meta::IsConst<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
R&> FindAdvanceAnyAdvance(R&& range, RWs&& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	RWs subrangesCopy = subranges;
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
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
size_t> CountUntilAdvanceAnyAdvance(RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAdvanceAnyAdvance(subranges, &index, oSubrangeIndex);
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
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
R&> FindAdvanceAny(R& range, const RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	auto subrangesCopy = subranges;
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
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
size_t> CountUntilAdvanceAny(R& range, const RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAdvanceAny(range, subranges, &index, oSubrangeIndex);
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
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ && !Meta::IsConst<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
R> FindAnyAdvance(const R& range, RWs&& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{return FindAdvanceAnyAdvance(R(range), subranges, ioIndex, oSubrangeIndex);}

//! Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс
//! найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает количество пройденных элементов.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ && !Meta::IsConst<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
size_t> CountUntilAnyAdvance(const R& range, RWs&& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAnyAdvance(range, subranges, &index, oSubrangeIndex);
	return index;
}



//! Найти первое вхождение любого поддиапазона из диапазона subranges в этот диапазон.
//! \param subranges Искомые поддиапазоны.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного поддиапазона
//! в диапазоне subranges. Если элемент не был найден, будет записано значение whats.Count().
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
R> FindAny(const R& range, const RWs& subranges,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{return FindAdvanceAny(R(range), subranges, ioIndex, oWhatIndex);}

//! Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс
//! найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает количество пройденных элементов.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
size_t> CountUntilAny(const R& range,
	const RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAny(range, subranges, &index, oSubrangeIndex);
	return index;
}




//! Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
//! Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
//! \param what Искомый диапазон.
//! \returns Возвращает количество пройденных элементов.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsFiniteForwardRange<RW>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_,
size_t> CountUntilAdvance(R&& range, const RW& what)
{
	size_t index=0;
	FindAdvance(range, what, &index);
	return index;
}

//! Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
//! \param what Искомый диапазон.
//! \returns Возвращает количество пройденных элементов.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RW>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_,
size_t> CountUntil(const R& range, const RW& what)
{return CountUntilAdvance(R(range), what);}
	

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	(Range::IsFiniteForwardRange<RW>::_ || Range::HasAsRange<RW>::_),
bool> Contains(const R& range, const RW& what)
{return !Find(range, Range::AsRange(what)).Empty();}

template<typename R, size_t N, typename T = Range::ValueTypeOf<R>> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_,
bool> Contains(const R& range, const T(&what)[N])
{
	const size_t len = N-size_t(Meta::IsCharType<Range::ValueTypeOf<R>>::_);
	return !Contains(range, Range::ArrayRange<const T>(what, len));
}

template<typename R, typename RW> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsFiniteForwardRange<RW>::_,
size_t> CountAdvance(R&& range, const RW& what)
{
	size_t result=0;
	size_t whatCount = Range::Count(what);
	while(FindAdvance(range, what), !range.Empty())
	{
		Range::PopFirstExactly(range, whatCount);
		result++;
	}
	return result;
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RW>::_,
size_t> Count(const R& range, const RW& what)
{return CountAdvance(R(range), what);}

INTRA_WARNING_POP

}}
