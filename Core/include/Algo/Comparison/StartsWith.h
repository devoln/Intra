#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/Operations.h"

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
	Range::IsAccessibleRange<R>::_ &&
	Range::IsConsumableRange<RW>::_ &&
	Meta::IsConvertible<Range::ValueTypeOf<RW>, Range::ValueTypeOf<R>>::_ &&
	!(Range::HasLength<R>::_ &&
		Range::HasLength<RW>::_),
bool> StartsWith(R&& range, RW&& what)
{
	auto rangeCopy = Meta::Forward<R>(range);
	auto whatCopy = Meta::Forward<RW>(what);
	return D::StartsAdvanceWithAdvance(rangeCopy, whatCopy);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsAccessibleRange<R>::_ &&
	Range::IsConsumableRange<RW>::_ &&
	Meta::IsConvertible<Range::ValueTypeOf<RW>, Range::ValueTypeOf<R>>::_ &&
	!(Range::HasData<R>::_ && Range::HasData<RW>::_ && Meta::IsAlmostPod<Range::ValueTypeOf<R>>::_) &&
	Range::HasLength<R>::_ && Range::HasLength<RW>::_,
bool> StartsWith(R&& range, RW&& what)
{
	if(range.Length()<what.Length()) return false;
	auto rangeCopy = Meta::Forward<R>(range);
	auto whatCopy = Meta::Forward<RW>(what);
	return D::StartsAdvanceWithAdvance(rangeCopy, whatCopy);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsArrayRange<R>::_ &&
	Range::IsArrayRangeOfExactly<RW, Range::ValueTypeOf<R>>::_ &&
	Meta::IsAlmostPod<Range::ValueTypeOf<R>>::_,
bool> StartsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	return C::memcmp(range.Data(), what.Data(), what.Length()*sizeof(what.First()))==0;
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	(!Range::IsInputRange<R>::_ && Range::IsAsForwardRange<R>::_) ||
	(!Range::IsInputRange<RW>::_ && Range::IsAsForwardRange<RW>::_),
bool> StartsWith(R&& range, RW&& what)
{return StartsWith(Range::Forward<R>(range), Range::Forward<RW>(what));}


//! Проверяет, начинается ли range с what.
//! Если да, то начало range смещается к первому элементу, идущему сразу после вхождения what.
//! Иначе range остаётся без изменений.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Range::IsAsNonInfiniteForwardRange<RW>::_,
bool> StartsAdvanceWith(R& range, RW&& what)
{
	auto whatCopy = Range::Forward<RW>(what);
	bool result = Algo::StartsWith(range, whatCopy);
	if(result) Range::PopFirstExactly(range, Range::Count(whatCopy));
	return result;
}

template<typename R, typename RWs> Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Range::IsInputRange<RWs>::_ && !Range::IsInfiniteRange<RWs>::_ &&
	Range::IsNonInfiniteForwardRange<Range::ValueTypeOf<RWs>>::_ &&
	Meta::IsConvertible<Range::ValueTypeOf<Range::ValueTypeOf<RWs>>, Range::ValueTypeOf<R>>::_,
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
	Range::IsAsForwardRange<R>::_ &&
	Range::IsAsConsumableRange<RWs>::_ &&
	Range::IsAsNonInfiniteForwardRange<Range::ValueTypeOfAs<RWs>>::_ &&
	Meta::IsConvertible<Range::ValueTypeOfAs<Range::ValueTypeOfAs<RWs>>, Range::ValueTypeOfAs<R>>::_,
bool> StartsWithAny(R&& range, RWs&& subranges, size_t* oSubrangeIndex = null)
{
	auto subrangesCopy = Range::Forward<RWs>(subranges);
	return StartsWithAnyAdvance(Range::Forward<R>(range), subrangesCopy, oSubrangeIndex);
}

template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Range::IsAsConsumableRange<RWs>::_ &&
	Range::IsAsNonInfiniteForwardRange<Range::ValueTypeOfAs<RWs>>::_ &&
	Meta::IsConvertible<Range::ValueTypeOfAs<Range::ValueTypeOfAs<RWs>>, Range::ValueTypeOf<R>>::_,
bool> StartsAdvanceWithAny(R& range, RWs&& subranges, size_t* oSubrangeIndex = null)
{
	auto subrangesCopy = Range::Forward<RWs>(subranges);
	bool result = Algo::StartsWithAnyAdvance(range, subrangesCopy, oSubrangeIndex);
	if(result) Range::PopFirstExactly(range, Range::Count(subrangesCopy.First()));
	return result;
}

INTRA_WARNING_POP

}}
