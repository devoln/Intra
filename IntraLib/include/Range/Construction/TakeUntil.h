#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Algo/Search/Single.h"
#include "Algo/Search/Subrange.h"
#include "Take.h"

namespace Intra { namespace Range {

//TODO: –еализовать класс TakeUntilResult дл€ InputRange


//! ѕоследовательно удал€ет элементы диапазона до тех пор, пока не:
//! 1) встретитс€ элемент, дл€ которого выполнен предикат valueOrPredOrSubrange;
//! 2) встретитс€ элемент, равный valueOrPredOrSubrange;
//! 3) встретитс€ поддиапазон valueOrPredOrSubrange;
//! 4) будет достигнут конец диапазона.
//! ¬озвращает диапазон пройденных элементов.
template<typename R, typename X> forceinline Meta::EnableIf<
	IsFiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	((Meta::IsConvertible<X, ValueTypeOf<R>>::_ ||
		Meta::IsCallable<X, ValueTypeOf<R>>::_) ||
	(IsForwardRange<X>::_ && !IsInfiniteRange<X>::_ &&
		ValueTypeIsConvertible<X, ValueTypeOf<R>>::_)),
ResultOfTake<R>> TakeUntilAdvance(R&& range, const X& valueOrPredOrSubrange, size_t* ioIndex=null)
{
	auto rangeCopy = range;
	size_t index = Algo::CountUntilAdvance(range, valueOrPredOrSubrange);
	if(ioIndex!=null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! ѕоследовательно просматривает элементы диапазона до тех пор, пока не:
//! 1) встретитс€ элемент, дл€ которого выполнен предикат valueOrPredOrSubrange;
//! 2) встретитс€ элемент, равный valueOrPredOrSubrange;
//! 3) встретитс€ поддиапазон valueOrPredOrSubrange;
//! 4) будет достигнут конец диапазона.
//! \param valueOrPredOrSubrange »скомое значение, предикат или диапазон.
//! ¬озвращает диапазон пройденных элементов.
template<typename R, typename X> forceinline Meta::EnableIf<
	IsForwardRange<R>::_ && !IsInfiniteRange<R>::_ &&
	((Meta::IsConvertible<X, ValueTypeOf<R>>::_ ||
		Meta::IsCallable<X, ValueTypeOf<R>>::_) ||
	(IsForwardRange<X>::_ && !IsInfiniteRange<X>::_ &&
		ValueTypeIsConvertible<X, ValueTypeOf<R>>::_)),
ResultOfTake<R>> TakeUntil(const R& range, const X& valueOrPredOrSubrange, size_t* ioIndex=null)
{return TakeUntilAdvance(R(range), valueOrPredOrSubrange, ioIndex);}

//! ѕоследовательно просматривает элементы массива до тех пор, пока не:
//! 1) встретитс€ элемент, дл€ которого выполнен предикат valOrPredOrRange
//! 2) встретитс€ элемент, равный valOrPredOrRange
//! 3) встретитс€ поддиапазон valOrPredOrRange
//! 4) будет достигнут конец диапазона.
//! ¬озвращает диапазон пройденных элементов.
template<typename T, size_t N, typename X> forceinline Meta::EnableIf<
	(Meta::IsConvertible<X, T>::_ ||
		Meta::IsCallable<X, T>::_) ||
	(IsForwardRange<X>::_ && !IsInfiniteRange<X>::_ &&
		ValueTypeIsConvertible<X, T>::_),
ArrayRange<T>> TakeUntil(T(&arr)[N], const X& valOrPredOrRange, size_t* ioIndex=null)
{return TakeUntil(AsRange(arr), valOrPredOrRange, ioIndex);}

}}
