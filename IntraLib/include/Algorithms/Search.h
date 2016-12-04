#pragma once

#include "Meta/Type.h"
#include "Range/Concepts.h"

namespace Intra { namespace Algo {

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, равных x.
template<typename R, typename X> Meta::EnableIf<
	Range::IsInputRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
R&> TrimLeftAdvance(R& range, const X& x)
{
	while(!range.Empty() && range.First()==x)
		range.PopFirst();
	return range;
}
	
//! Последовательно удаляет элементы из начала диапазона, пока выполняется предикат pred.
//! Останавливается на первом элементе, для которого это условие не выполнено.
template<typename R, typename P> Meta::EnableIf<
	Range::IsInputRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
R&> TrimLeftAdvance(R& range, P pred)
{
	while(!range.Empty() && pred(range.First()))
		range.PopFirst();
	return range;
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, равных x.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
R> TrimLeft(const R& range, const X& x)
{
	R result = range;
	return TrimLeftAdvance(result, x);
}

//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, для которых выполнен предикат pred.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
R> TrimLeft(const R& range, P pred)
{
	R result = range;
	return TrimLeftAdvance(result, pred);
}


//! Найти первое вхождение элемента what в этот диапазон.
//! Изменяет диапазон в null, если значение не найдено. Иначе оставляет часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
//! \param what Искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
template<typename R, typename X> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Meta::IsConvertible<X, typename R::value_type>::_,
R&> FindAdvance(R& range, const X& what, size_t* ioIndex=null)
{
	size_t index=0;
	while(!range.Empty() && !(range.First()==what))
	{
		range.PopFirst();
		index++;
	}
	if(ioIndex!=null) *ioIndex += index;
	return range;
}

//! Найти первое вхождение элемента, удовлетворяющего некоторому условию, в этот диапазон.
//! Изменяет диапазон в null, если значение не найдено. Иначе оставляет часть этого диапазона, начиная с позиции, на которой начинается первое вхождение искомого элемента.
//! \param pred Условие, которому должен удовлетворять искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
template<typename R, typename P> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Meta::IsCallable<P, typename R::value_type>::_,
R&> FindAdvance(R& range, P pred, size_t* ioIndex=null)
{
	size_t index=0;
	while(!range.Empty() && !pred(range.First()))
	{
		range.PopFirst();
		index++;
	}
	if(ioIndex!=null) *ioIndex += index;
	return range;
}


//! Найти первое вхождение любого элемента из диапазона whats в этот диапазон.
//! Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
//! \param whats Диапазон искомых элементов. Если один из этих элементов найден, начало whats устанавливается на найденный элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
//! \return Возвращает ссылку на себя.
template<typename R, typename Ws> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ &&
	Range::IsFiniteForwardRange<Ws>::_ && Meta::IsConvertible<typename Ws::value_type, typename R::value_type>::_,
R&> FindAdvanceAnyAdvance(R& range, Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	size_t index=0, whatIndex = whats.Count();
	Ws whatsCopy = whats;
	for(; !range.Empty(); range.PopFirst(), index++)
	{
		whats = whatsCopy;
		whatIndex = 0;
		while(!whats.Empty())
		{
			if(range.First()!=whats.First())
			{
				whats.PopFirst();
				whatIndex++;
				continue;
			}
			goto exit;
		}
	}
exit:
	if(ioIndex!=null) *ioIndex += index;
	if(oWhatIndex!=null) *oWhatIndex = whatIndex;
	return range;
}

//! Найти первое вхождение любого элемента из диапазона whats в этот диапазон.
//! Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
//! \param whats Диапазон искомых элементов.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
//! \return Возвращает ссылку на себя.
template<typename R, typename Ws> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Range::IsFiniteForwardRangeOf<Ws, typename R::value_type>::_,
R&> FindAdvanceAny(R& range, const Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	Ws whatCopy = whats;
	return FindAdvanceAnyAdvance(range, whatCopy, ioIndex, oWhatIndex);
}



//! Последовательно удаляет элементы из начала диапазона, до тех пор,
//! пока не встретится элемент, равный x, или диапазон не станет пустым.
//! \return Возвращает количество пройденных элементов.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Meta::IsConvertible<X, typename R::value_type>::_,
size_t> CountUntilAdvance(R& range, const X& x)
{
	size_t index=0;
	FindAdvance(range, x, &index);
	return index;
}
	
//! Последовательно удаляет элементы из начала диапазона до тех пор,
//! пока не выполнится условие predicate или диапазон не станет пустым.
//! \return Возвращает количество пройденных элементов.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Meta::IsCallable<P, typename R::value_type>::_,
size_t> CountUntilAdvance(R& range, P predicate)
{
	size_t index=0;
	FindAdvance(range, predicate, &index);
	return index;
}

//! Последовательно удаляет элементы из начала диапазона до тех пор,
//! пока не встретится элемент, равный любому из элементов диапазона whats.
//! \param whats[inout] Диапазон искомых элементов. Если элемент найден, начало диапазона whats смещается к найденному элементу.
//! \return Возвращает количество пройденных элементов.
template<typename R, typename Ws> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Range::IsForwardRangeOf<Ws, typename R::value_type>::_,
size_t> CountUntilAdvanceAnyAdvance(R& range, Ws& whats)
{
	size_t index=0;
	FindAdvanceAnyAdvance(range, whats, &index);
	return index;
}

//! Последовательно удаляет элементы из начала диапазона до тех пор,
//! пока не встретится элемент, равный любому из элементов диапазона whats.
//! \param whats[inout] Диапазон искомых элементов.
//! \return Возвращает количество пройденных элементов.
template<typename R, typename Ws> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Range::IsForwardRangeOf<Ws, typename R::value_type>::_,
size_t> CountUntilAdvanceAny(R& range, const Ws& whats)
{
	size_t index=0;
	FindAdvanceAny(range, whats, &index);
	return index;
}



//! Последовательно просматривает элементы диапазона до тех пор,
//! пока не встретится элемент, равный x, или не будет достигнут конец диапазона.
//! Возвращает количество пройденных элементов.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
size_t> CountUntil(const R& range, const X& x)
{
	R r = range;
	return CountUntilAdvance(r, x);
}
	
//! Последовательно просматривает элементы диапазона до тех пор, пока для элемента
//! не выполнится условие predicate или не будет достигнут конец диапазона.
//! \return Возвращает количество пройденных элементов.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
size_t> CountUntil(const R& range, P predicate)
{
	R r = range;
	return CountUntilAdvance(r, predicate);
}


//! Последовательно удаляет элементы диапазона до тех пор, пока не встретится элемент,
//! равный x, или не будет достигнут конец диапазона.
//! \return Возвращает диапазон пройденных элементов.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
R> ReadUntilAdvance(R& range, const X& x, size_t* ioIndex=null)
{
	auto r = range;
	size_t index = CountUntilAdvance(range, x);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(r, index);
}
	
//! Последовательно удаляет элементы диапазона до тех пор, пока для элемента
//! не выполнится условие predicate или не будет достигнут конец диапазона.
//! Возвращает диапазон пройденных элементов.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
R> ReadUntilAdvance(R& range, P predicate, size_t* ioIndex=null)
{
	auto r = range;
	size_t index = CountUntilAdvance(range, predicate);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(r, index);
}

//! Последовательно просматривает элементы диапазона до тех пор,
//! пока не встретится элемент, равный x, или не будет достигнут конец диапазона.
//! Возвращает диапазон пройденных элементов.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntil(const R& range, const X& x, size_t* ioIndex=null)
{
	const size_t index = CountUntil(range, x);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}
	
//! Последовательно просматривает элементы диапазона до тех пор,
//! пока для элемента не выполнится условие predicate или не будет достигнут конец диапазона.
//! Возвращает диапазон пройденных элементов.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntil(const R& range, P predicate, size_t* ioIndex=null)
{
	size_t index = CountUntil(range, predicate);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}


//! Найти первое вхождение элемента what в этот диапазон.
//! \param what Искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \returns null, если значение не найдено. Часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
template<typename R, typename X> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
R> Find(const R& range, const X& what, size_t* ioIndex=null)
{
	auto result = range;
	FindAdvance(result, what, ioIndex);
	return result;
}

template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
bool> Contains(const R& range, const X& what) {return !Find(range, what).Empty();}

//! Найти первое вхождение элемента, удовлетворяющего некоторому условию, в этот диапазон.
//! \param pred Условие, которому должен удовлетворять искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
R> Find(const R& range, P pred, size_t* ioIndex=null)
{
	R r = range;
	return FindAdvance(r, pred, ioIndex);
}

template<typename R> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_,
bool> Contains(const R& range, const typename R::value_type& what)
{return !Find(range, what).Empty();}
	

template<typename R, typename Ws> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOf<Ws, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntilAdvanceAny(const R& range, const Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	R r = range;
	size_t index = CountUntilAdvanceAny(r, whats, oWhatIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}

template<typename R, typename Ws> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRangeOf<Ws, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntilAny(const R& range, const Ws& whats,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	size_t index = CountUntilAny(range, whats, oWhatIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}

//! Найти первое вхождение диапазона what в этот диапазон.
//! Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
//! \param what Искомый диапазон.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \returns Возвращает ссылку на себя.
template<typename R, typename RW> Meta::EnableIf<
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
R&> FindAdvance(R& range, const RW& what, size_t* ioIndex=null)
{
	while(!range.Empty() && !range.StartsWith(what))
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
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
R> Find(const R& range, const RW& what, size_t* ioIndex=null)
{
	auto result = range;
	return FindAdvance(result, what, ioIndex);
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
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
R&> FindAdvanceAnyAdvance(R& range, RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
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
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
size_t> CountUntilAdvanceAnyAdvance(RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAdvanceAnyAdvance(subranges, &index, oSubrangeIndex);
	return index;
}

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
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
Range::ResultOfTake<R>> ReadUntilAdvanceAnyAdvance(R& range,
	RWs& subranges, size_t* ioIndex, size_t* oSubrangeIndex=null)
{
	auto r = range;
	size_t index = CountUntilAdvanceAnyAdvance(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(r, index);
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
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
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
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
size_t> CountUntilAdvanceAny(R& range, const RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAdvanceAny(range, subranges, &index, oSubrangeIndex);
	return index;
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
template<typename R, typename RWs> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
Range::ResultOfTake<R>> ReadUntilAdvanceAny(R& range,
	const RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	auto r = range;
	size_t index = CountUntilAdvanceAny(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(r, index);
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
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
R> FindAnyAdvance(const R& range, RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	R r = range;
	return FindAdvanceAnyAdvance(r, subranges, ioIndex, oSubrangeIndex);
}

//! Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс
//! найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает количество пройденных элементов.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
size_t> CountUntilAnyAdvance(const R& range, RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAnyAdvance(range, subranges, &index, oSubrangeIndex);
	return index;
}

//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
//! из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<typename R, typename RWs> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
Range::ResultOfTake<R>> ReadUntilAnyAdvance(const R& range,
	RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	size_t index = CountUntilAnyAdvance(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(range, index);
}



//! Найти первое вхождение любого поддиапазона из диапазона subranges в этот диапазон.
//! \param subranges Искомые поддиапазоны.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного поддиапазона
//! в диапазоне subranges. Если элемент не был найден, будет записано значение whats.Count().
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
R> FindAny(const R& range, const RWs& subranges,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	R result = range;
	return FindAdvanceAny(result, subranges, ioIndex, oWhatIndex);
}

//! Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс
//! найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает количество пройденных элементов.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
size_t> CountUntilAny(const R& range,
	const RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAny(range, subranges, &index, oSubrangeIndex);
	return index;
}

//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
//! из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
Range::ResultOfTake<R>> ReadUntilAny(const R& range, const RWs& subranges,
	size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	const size_t index = CountUntilAny(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}


//! Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
//! Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
//! \param what Искомый диапазон.
//! \returns Возвращает количество пройденных элементов.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
size_t> CountUntilAdvance(R& range, const RW& what)
{
	size_t index=0;
	FindAdvance(range, what, &index);
	return index;
}

//! Прочитать элементы из начала диапазона, предшествующие первому вхождению диапазона what в этот диапазон.
//! \param what Искомый диапазон.
//! \returns Возвращает диапазон пройденных элементов.
template<typename R, typename RW> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntilAdvance(R& range, const RW& what, size_t* ioIndex=null)
{
	R r = range;
	size_t index = CountUntilAdvance(range, what);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(r, index);
}

//! Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
//! \param what Искомый диапазон.
//! \returns Возвращает количество пройденных элементов.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
size_t> CountUntil(const R& range, const RW& what)
{
	R r = range;
	return CountUntilAdvance(r, what);
}

//! Прочитать элементы из начала диапазона, предшествующие первому вхождению диапазона what в этот диапазон.
//! \param what Искомый диапазон.
//! \returns Возвращает диапазон пройденных элементов.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntil(const R& range,
	const RW& what, size_t* ioIndex=null)
{
	size_t index = CountUntil(range, what);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}



	

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	(Range::IsFiniteForwardRange<RW>::_ || Range::HasAsRange<RW>::_),
bool> Contains(const R& range, const RW& what)
{return !Find(range, Range::AsRange(what)).Empty();}

template<typename R, size_t N, typename T = typename R::value_type> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_,
bool> Contains(const R& range, const T(&what)[N])
{
	const size_t len = N-size_t(Meta::IsCharType<typename R::value_type>::_);
	return !Contains(range, Range::ArrayRange<const T>(what, len));
}



}}

