#pragma once

#include "Platform/CppWarnings.h"
#include "Meta/Operators.h"
#include "Range/Concepts.h"
#include "Range/Decorators/Take.h"
#include "Range/Operations.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Найти первое вхождение элемента what в этот диапазон.
//! Изменяет диапазон в null, если значение не найдено.
//! Иначе оставляет часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
//! \param what Искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
template<class R, typename X> Meta::EnableIf<
	IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	!Meta::IsCallable<X, ValueTypeOf<R>>::_ &&
	!(IsFiniteForwardRange<X>::_ &&
	Meta::IsConvertible<ValueTypeOf<X>, ValueTypeOf<R>>::_),
R&&> FindAdvance(R&& range, const X& what, size_t* ioIndex=null)
{
	size_t index = 0;
	while(!range.Empty() && !(range.First()==what))
	{
		range.PopFirst();
		index++;
	}
	if(ioIndex!=null) *ioIndex += index;
	return Meta::Forward<R>(range);
}

//! Найти первое вхождение элемента, удовлетворяющего некоторому условию, в этот диапазон.
//! Изменяет диапазон в null, если значение не найдено.
//! Иначе оставляет часть этого диапазона, начиная с позиции, на которой начинается первое вхождение искомого элемента.
//! \param pred Условие, которому должен удовлетворять искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
template<class R, typename P> Meta::EnableIf<
	IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, ValueTypeOf<R>>::_,
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
	IsNonInfiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsNonInfiniteForwardRange<Ws>::_ && !Meta::IsConst<Ws>::_ &&
	Meta::IsConvertible<ValueTypeOf<Ws>, ValueTypeOf<R>>::_,
R&> FindAdvanceAnyAdvance(R& range, Ws& whats,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	size_t index=0, whatIndex = Range::Count(whats);
	Ws whatsCopy = Meta::Forward<Ws>(whats);
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
template<class R, class Ws> forceinline Meta::EnableIf<
	IsNonInfiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsNonInfiniteForwardRange<Ws>::_ &&
	Meta::IsConvertible<ValueTypeOfAs<Ws>, ValueTypeOf<R>>::_,
R&> FindAdvanceAny(R& range, Ws&& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	auto whatsCopy = Range::Forward<Ws>(whats);
	return FindAdvanceAnyAdvance(range, whatsCopy, ioIndex, oWhatIndex);
}


//! Последовательно удаляет все элементы из диапазона, подсчитывая количество элементов, равных x.
template<class R, typename X> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	!Meta::IsCallable<X, ValueTypeOf<R>>::_ &&
	Meta::HasOpEquals<ReturnValueTypeOf<R>, const X&>::_,
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
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, ValueTypeOf<R>>::_,
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
template<class R, typename X> Meta::EnableIf<
	IsAsConsumableRange<R>::_ &&
	(Meta::IsCallable<X, ValueTypeOf<R>>::_ ||
		Meta::HasOpEquals<ReturnValueTypeOf<R>, const X&>::_),
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
	IsNonInfiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	!(IsNonInfiniteForwardRange<X>::_ &&
	Meta::IsConvertible<ValueTypeOf<X>, ValueTypeOf<R>>::_),
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
	IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsForwardRange<Ws>::_ && !Meta::IsConst<Ws>::_ &&
	Meta::IsConvertible<ValueTypeOf<Ws>, ValueTypeOf<R>>::_,
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
template<class R, class Ws> forceinline Meta::EnableIf<
	IsNonInfiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsForwardRange<Ws>::_ &&
	Meta::IsConvertible<ValueTypeOfAs<Ws>, ValueTypeOf<R>>::_,
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
template<class R, typename X> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	(Meta::IsConvertible<X, ValueTypeOfAs<R>>::_ ||
		Meta::IsCallable<X, ValueTypeOfAs<R>>::_),
size_t> CountUntil(R&& range, const X& valueOrPred)
{
	auto rangeCopy = Range::Forward<R>(range);
	return CountUntilAdvance(rangeCopy, valueOrPred);
}


//! Найти первое вхождение элемента what в этот диапазон.
//! \param what Искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
//! \returns null, если значение не найдено. Часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
template<class R, typename X> Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, ValueTypeOfAs<R>>::_,
Meta::RemoveConstRef<AsRangeResult<R>>> Find(R&& range, const X& what, size_t* ioIndex=null)
{
	auto result = Range::Forward<R>(range);
	FindAdvance(result, what, ioIndex);
	return result;
}

template<class R, typename X> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, ValueTypeOfAs<R>>::_,
bool> Contains(R&& range, const X& what)
{return !Find(Range::Forward<R>(range), what).Empty();}

//! Найти первое вхождение элемента, удовлетворяющего некоторому условию, в этот диапазон.
//! \param pred Условие, которому должен удовлетворять искомый элемент.
//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
template<class R, typename P> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	Meta::IsCallable<P, ValueTypeOfAs<R>>::_,
Meta::RemoveConstRef<AsRangeResult<R>>> Find(R&& range, P pred, size_t* ioIndex=null)
{
	auto rangeCopy = Range::Forward<R>(range);
	return FindAdvance(rangeCopy, pred, ioIndex);
}

INTRA_WARNING_POP

}}
