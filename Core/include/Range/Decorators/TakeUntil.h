#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Algo/Search/Single.h"
#include "Algo/Search/Subrange.h"
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
template<typename R, typename X> forceinline Meta::EnableIf<
	IsNonInfiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	((Meta::IsConvertible<X, ValueTypeOf<R>>::_ ||
		Meta::IsCallable<X, ValueTypeOf<R>>::_) ||
	(IsForwardRange<X>::_ && !IsInfiniteRange<X>::_ &&
		Meta::IsConvertible<ValueTypeOf<X>, ValueTypeOf<R>>::_)),
ResultOfTake<R>> TakeUntilAdvance(R&& range, const X& valueOrPredOrSubrange, size_t* ioIndex=null)
{
	auto rangeCopy = range;
	size_t index = Algo::CountUntilAdvance(range, valueOrPredOrSubrange);
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
template<typename R, typename X> forceinline Meta::EnableIf<
	IsForwardRange<R>::_ && !IsInfiniteRange<R>::_ &&
	((Meta::IsConvertible<X, ValueTypeOf<R>>::_ ||
		Meta::IsCallable<X, ValueTypeOf<R>>::_) ||
	(IsForwardRange<X>::_ && !IsInfiniteRange<X>::_ &&
		Meta::IsConvertible<ValueTypeOf<X>, ValueTypeOf<R>>::_)),
ResultOfTake<R>> TakeUntil(const R& range, const X& valueOrPredOrSubrange, size_t* ioIndex=null)
{return TakeUntilAdvance(R(range), valueOrPredOrSubrange, ioIndex);}

INTRA_WARNING_POP

}}
