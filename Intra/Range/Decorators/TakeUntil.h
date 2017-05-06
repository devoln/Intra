#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Range/ForwardDecls.h"
#include "Concepts/Range.h"
#include "Range/Search/Single.h"
#include "Range/Search/Subrange.h"
#include "Take.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//TODO: Реализовать класс TakeUntilResult для InputRange


//! Последовательно удаляет элементы диапазона до тех пор, пока не:
//! 1) встретится элемент, для которого выполнен предикат valueOrPredOrSubrange;
//! 2) встретится элемент, равный valueOrPredOrSubrange;
//! 3) встретится поддиапазон valueOrPredOrSubrange;
//! 4) будет достигнут конец диапазона.
//! Возвращает диапазон пройденных элементов.
template<typename R, typename X,
	typename T = Concepts::ValueTypeOf<R>
> forceinline Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	((Meta::IsConvertible<X, T>::_ ||
		Meta::IsCallable<X, T>::_) ||
	(Concepts::IsForwardRange<X>::_ && !Concepts::IsInfiniteRange<X>::_ &&
		Meta::IsConvertible<Concepts::ValueTypeOf<X>, T>::_)),
TakeResult<R>> TakeUntilAdvance(R&& range, const X& valueOrPredOrSubrange, size_t* ioIndex=null)
{
	auto rangeCopy = range;
	size_t index = CountUntilAdvance(range, valueOrPredOrSubrange);
	if(ioIndex!=null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! Последовательно просматривает элементы диапазона до тех пор, пока не:
//! 1) встретится элемент, для которого выполнен предикат valueOrPredOrSubrange;
//! 2) встретится элемент, равный valueOrPredOrSubrange;
//! 3) встретится поддиапазон valueOrPredOrSubrange;
//! 4) будет достигнут конец диапазона.
//! \param valueOrPredOrSubrange Искомое значение, предикат или диапазон.
//! Возвращает диапазон пройденных элементов.
template<typename R, typename X,
	typename T = Concepts::ValueTypeOf<R>
> forceinline Meta::EnableIf<
	Concepts::IsForwardRange<R>::_ &&
	!Concepts::IsInfiniteRange<R>::_ &&
	((Meta::IsConvertible<X, T>::_ ||
		Meta::IsCallable<X, T>::_) ||
	(Concepts::IsForwardRange<X>::_ &&
		!Concepts::IsInfiniteRange<X>::_ &&
		Meta::IsConvertible<Concepts::ValueTypeOf<X>, T>::_)),
TakeResult<R>> TakeUntil(const R& range, const X& valueOrPredOrSubrange, size_t* ioIndex=null)
{return TakeUntilAdvance(R(range), valueOrPredOrSubrange, ioIndex);}

INTRA_WARNING_POP

}}
