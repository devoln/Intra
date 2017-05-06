#pragma once

#include "Cpp/Warnings.h"
#include "Meta/Operators.h"
#include "Concepts/Range.h"
#include "Range/Decorators/Take.h"
#include "Range/Operations.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

//! Найти первое вхождение элемента what в этот диапазон.
//! Изменяет диапазон в null, если значение не найдено.
//! Иначе оставляет часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
//! \param what Искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
template<class R, typename X> Meta::EnableIf<
	Concepts::IsFiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	!Meta::IsCallable<X, Concepts::ValueTypeOf<R>>::_ &&
	!(Concepts::IsFiniteForwardRange<X>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<X>, Concepts::ValueTypeOf<R>>::_),
R&&> FindAdvance(R&& range, const X& what, size_t* ioIndex=null)
{
	size_t index = 0;
	while(!range.Empty() && !(range.First()==what))
	{
		range.PopFirst();
		index++;
	}
	if(ioIndex!=null) *ioIndex += index;
	return Cpp::Forward<R>(range);
}

//! Найти первое вхождение элемента, удовлетворяющего некоторому условию, в этот диапазон.
//! Изменяет диапазон в null, если значение не найдено.
//! Иначе оставляет часть этого диапазона, начиная с позиции, на которой начинается первое вхождение искомого элемента.
//! \param pred Условие, которому должен удовлетворять искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
template<class R, typename P> Meta::EnableIf<
	Concepts::IsFiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<R>>::_,
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
//! \param whats Диапазон искомых элементов. Если один из этих элементов найден,
//! начало whats устанавливается на найденный элемент. Иначе whats становится пустым
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
//! \return Возвращает ссылку на себя.
template<class R, class Ws> Meta::EnableIf<
	Concepts::IsNonInfiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsNonInfiniteForwardRange<Ws>::_ &&
	!Meta::IsConst<Ws>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<Ws>, Concepts::ValueTypeOf<R>>::_,
R&> FindAdvanceAnyAdvance(R& range, Ws& whats,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	size_t index=0, whatIndex = Range::Count(whats);
	Ws whatsCopy = Cpp::Forward<Ws>(whats);
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
template<class R, class Ws,
	typename AsWs = Concepts::RangeOfType<Ws>
> forceinline Meta::EnableIf<
	Concepts::IsNonInfiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsNonInfiniteForwardRange<AsWs>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<AsWs>, Concepts::ValueTypeOf<R>>::_,
R&> FindAdvanceAny(R& range, Ws&& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	auto whatsCopy = Range::Forward<Ws>(whats);
	return FindAdvanceAnyAdvance(range, whatsCopy, ioIndex, oWhatIndex);
}


//! Последовательно удаляет все элементы из диапазона, подсчитывая количество элементов, равных x.
template<class R, typename X> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	!Meta::IsCallable<X, Concepts::ValueTypeOf<R>>::_ &&
	Meta::HasOpEquals<Concepts::ReturnValueTypeOf<R>, const X&>::_,
size_t> CountAdvance(R& range, const X& x)
{
	size_t result=0;
	while(!range.Empty())
	{
		if(range.First()==x) result++;
		range.PopFirst();
	}
	return result;
}


//! Последовательно удаляет все элементы из диапазона, подсчитывая количество элементов, для которых выполнено условие pred.
template<class R, typename P> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<R>>::_,
size_t> CountAdvance(R& range, P pred)
{
	size_t result=0;
	while(!range.Empty())
	{
		if(pred(range.First())) result++;
		range.PopFirst();
	}
	return result;
}

//! Подсчитать количество элементов, равных x или для которых выполнен предикат x.
template<class R, typename X,
	typename AsR = Concepts::RangeOfType<R>
> Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_ &&
	(Meta::IsCallable<X, Concepts::ValueTypeOf<AsR>>::_ ||
		Meta::HasOpEquals<Concepts::ReturnValueTypeOf<AsR>, const X&>::_),
size_t> Count(R&& range, const X& x)
{
	auto rangeCopy = Range::Forward<R>(range);
	return CountAdvance(rangeCopy, x);
}


//! Последовательно удаляет элементы из начала диапазона, до тех пор, пока не:
//! 1) встретится элемент, для которого выполнится предикат valOrPred, если это предикат;
//! 2) встретится элемент, равный valOrPred, если это не предикат;
//! 3) будет достигнут конец диапазона.
//! \return Возвращает количество пройденных элементов.
template<class R, typename X> forceinline Meta::EnableIf<
	Concepts::IsNonInfiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	!(Concepts::IsNonInfiniteForwardRange<X>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<X>, Concepts::ValueTypeOf<R>>::_),
size_t> CountUntilAdvance(R&& range, const X& valOrPred)
{
	size_t index=0;
	FindAdvance(range, valOrPred, &index);
	return index;
}

//! Последовательно удаляет элементы из начала диапазона до тех пор,
//! пока не встретится элемент, равный любому из элементов диапазона whats.
//! \param whats[inout] Диапазон искомых элементов. Если элемент найден, начало диапазона whats смещается к найденному элементу.
//! \return Возвращает количество пройденных элементов.
template<class R, class Ws> forceinline Meta::EnableIf<
	Concepts::IsFiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsForwardRange<Ws>::_ &&
	!Meta::IsConst<Ws>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<Ws>, Concepts::ValueTypeOf<R>>::_,
size_t> CountUntilAdvanceAnyAdvance(R&& range, Ws&& whats)
{
	size_t index=0;
	FindAdvanceAnyAdvance(range, whats, &index);
	return index;
}

//! Последовательно удаляет элементы из начала диапазона до тех пор,
//! пока не встретится элемент, равный любому из элементов диапазона whats.
//! \param whats[inout] Диапазон искомых элементов.
//! \return Возвращает количество пройденных элементов.
template<class R, class Ws,
	typename AsWs = Concepts::RangeOfType<Ws>
> forceinline Meta::EnableIf<
	Concepts::IsNonInfiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsForwardRange<AsWs>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<AsWs>, Concepts::ValueTypeOf<R>>::_,
size_t> CountUntilAdvanceAny(R&& range, Ws&& whats)
{
	size_t index = 0;
	FindAdvanceAny(range, Range::Forward<Ws>(whats), &index);
	return index;
}



//! Последовательно просматривает элементы диапазона до тех пор,
//! пока не встретится элемент, для которого либо выполнен предикат valueOrPred,
//! либо равный valueOrPred, если это не предикат, или не будет достигнут конец диапазона.
//! Возвращает количество пройденных элементов.
template<class R, typename X,
	typename AsR = Concepts::RangeOfType<R>,
	typename T = Concepts::ValueTypeOf<AsR>
> forceinline Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<AsR>::_ &&
	(Meta::IsConvertible<X, T>::_ ||
		Meta::IsCallable<X, T>::_),
size_t> CountUntil(R&& range, const X& valueOrPred)
{
	auto rangeCopy = Range::Forward<R>(range);
	return CountUntilAdvance(rangeCopy, valueOrPred);
}


//! Найти первое вхождение элемента what в этот диапазон.
//! \param what Искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \returns null, если значение не найдено. Часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
template<class R, typename X,
	typename AsR = Concepts::RangeOfTypeNoCRef<R>
> Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<AsR>::_ &&
	Meta::IsConvertible<X, Concepts::ValueTypeOf<AsR>>::_,
AsR> Find(R&& range, const X& what, size_t* ioIndex=null)
{
	auto result = Range::Forward<R>(range);
	FindAdvance(result, what, ioIndex);
	return result;
}

template<class R, typename X,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<AsR>::_ &&
	Meta::IsConvertible<X, Concepts::ValueTypeOf<AsR>>::_,
bool> Contains(R&& range, const X& what)
{return !Find(Range::Forward<R>(range), what).Empty();}

//! Найти первое вхождение элемента, удовлетворяющего некоторому условию, в этот диапазон.
//! \param pred Условие, которому должен удовлетворять искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
template<class R, typename P,
	typename AsR = Concepts::RangeOfTypeNoCRef<R>
> forceinline Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<AsR>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<AsR>>::_,
AsR> Find(R&& range, P pred, size_t* ioIndex=null)
{
	auto rangeCopy = Range::Forward<R>(range);
	return FindAdvance(rangeCopy, pred, ioIndex);
}

}}

INTRA_WARNING_POP
