#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/Operations.h"
#include "Platform/InitializerList.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace D {

template<typename R, typename RW> bool StartsAdvanceWithAdvance(R& range, RW& what)
{
	while(!what.Empty())
	{
		if(range.Empty()) return false;
		if(range.First()!=what.First()) return false;
		range.PopFirst();
		what.PopFirst();
	}
	return true;
}

}

template<typename R, typename RW> forceinline Meta::EnableIf<
	IsAccessibleRange<R>::_ &&
	IsConsumableRange<RW>::_ &&
	Meta::IsConvertible<ValueTypeOf<RW>, ValueTypeOf<R>>::_ &&
	!(HasLength<R>::_ && HasLength<RW>::_),
bool> StartsWith(R&& range, RW&& what)
{
	auto rangeCopy = Meta::Forward<R>(range);
	auto whatCopy = Meta::Forward<RW>(what);
	return D::StartsAdvanceWithAdvance(rangeCopy, whatCopy);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	IsAccessibleRangeWithLength<R>::_ &&
	IsAccessibleRangeWithLength<RW>::_ &&
	Meta::IsConvertible<ValueTypeOf<RW>, ValueTypeOf<R>>::_ &&
	!(HasData<R>::_ && HasData<RW>::_ && Meta::IsAlmostPod<ValueTypeOf<R>>::_),
bool> StartsWith(R&& range, RW&& what)
{
	if(range.Length()<what.Length()) return false;
	auto rangeCopy = Meta::Forward<R>(range);
	auto whatCopy = Meta::Forward<RW>(what);
	return D::StartsAdvanceWithAdvance(rangeCopy, whatCopy);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	IsArrayRange<R>::_ &&
	IsArrayRangeOfExactly<RW, ValueTypeOf<R>>::_ &&
	Meta::IsAlmostPod<ValueTypeOf<R>>::_,
bool> StartsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	return C::memcmp(range.Data(), what.Data(), what.Length()*sizeof(what.First()))==0;
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	(!IsInputRange<R>::_ || !IsInputRange<RW>::_) &&
	IsAsForwardRange<R>::_ && IsAsForwardRange<RW>::_,
bool> StartsWith(R&& range, RW&& what)
{return StartsWith(Range::Forward<R>(range), Range::Forward<RW>(what));}

template<typename R, typename W> forceinline Meta::EnableIf<
	IsForwardRange<R>::_ && !IsAsInputRange<W>::_,
bool> StartsWith(const R& range, const W& what)
{return !range.Empty() && range.First()==what;}

template<typename R, typename W> forceinline Meta::EnableIf<
	!IsInputRange<R>::_ && IsAsForwardRange<R>::_ && !IsAsInputRange<W>::_,
bool> StartsWith(R&& range, const W& what)
{return StartsWith(Range::Forward<R>(range), what);}


//! Проверяет, начинается ли range с what.
//! Если да, то начало range смещается к первому элементу, идущему сразу после вхождения what.
//! Иначе range остаётся без изменений.
template<typename R, typename RW> forceinline Meta::EnableIf<
	IsForwardRange<R>::_ &&
	IsAsConsumableRange<RW>::_,
bool> SkipIfStartsWith(R& range, RW&& what)
{
	auto whatCopy = Range::Forward<RW>(what);
	bool result = Algo::StartsWith(range, whatCopy);
	if(result) Range::PopFirstExactly(range, Range::Count(whatCopy));
	return result;
}

//! Проверяет, начинается ли range с what.
//! Если да, то начало range смещается к первому элементу, идущему сразу после вхождения what.
//! Иначе range остаётся без изменений.
template<typename R, typename RW> forceinline Meta::EnableIf<
	IsInputRange<R>::_ &&
	IsAsConsumableRange<RW>::_,
bool> StartsAdvanceWith(R& range, RW&& what)
{
	auto whatCopy = Range::Forward<RW>(what);
	return D::StartsAdvanceWithAdvance(range, whatCopy);
}

template<typename R, typename RWs> Meta::EnableIf<
	IsForwardRange<R>::_ &&
	IsNonInfiniteInputRange<RWs>::_ &&
	IsNonInfiniteForwardRange<ValueTypeOf<RWs>>::_ &&
	Meta::IsConvertible<ValueTypeOf<ValueTypeOf<RWs>>, ValueTypeOf<R>>::_,
bool> StartsWithAnyAdvance(const R& range, RWs& subranges, size_t* oSubrangeIndex = null)
{
	if(oSubrangeIndex!=null) *oSubrangeIndex = 0;
	while(!subranges.Empty())
	{
		if(StartsWith(range, subranges.First())) return true;
		if(oSubrangeIndex!=null) ++*oSubrangeIndex;
		subranges.PopFirst();
	}
	return false;
}

template<typename R, typename RWs> forceinline Meta::EnableIf<
	IsAsForwardRange<R>::_ &&
	IsAsConsumableRange<RWs>::_ &&
	IsAsNonInfiniteForwardRange<ValueTypeOfAs<RWs>>::_ &&
	Meta::IsConvertible<ValueTypeOfAs<ValueTypeOfAs<RWs>>, ValueTypeOfAs<R>>::_,
bool> StartsWithAny(R&& range, RWs&& subranges, size_t* oSubrangeIndex = null)
{
	auto subrangesCopy = Range::Forward<RWs>(subranges);
	return StartsWithAnyAdvance(Range::Forward<R>(range), subrangesCopy, oSubrangeIndex);
}

template<typename R, typename RWs> forceinline Meta::EnableIf<
	IsForwardRange<R>::_ &&
	IsAsConsumableRange<RWs>::_ &&
	IsAsNonInfiniteForwardRange<ValueTypeOfAs<RWs>>::_ &&
	Meta::IsConvertible<ValueTypeOfAs<ValueTypeOfAs<RWs>>, ValueTypeOf<R>>::_,
bool> StartsAdvanceWithAny(R& range, RWs&& subranges, size_t* oSubrangeIndex = null)
{
	auto subrangesCopy = Range::Forward<RWs>(subranges);
	bool result = Algo::StartsWithAnyAdvance(range, subrangesCopy, oSubrangeIndex);
	if(result) Range::PopFirstExactly(range, Range::Count(subrangesCopy.First()));
	return result;
}

INTRA_WARNING_POP

}}
