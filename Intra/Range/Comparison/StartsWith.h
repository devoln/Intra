#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Cpp/InitializerList.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Range/Operations.h"

namespace Intra { namespace Range {

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
	Concepts::IsAccessibleRange<R>::_ &&
	Concepts::IsConsumableRange<RW>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<RW>, Concepts::ValueTypeOf<R>>::_ &&
	!(Concepts::HasLength<R>::_ && Concepts::HasLength<RW>::_),
bool> StartsWith(R&& range, RW&& what)
{
	auto rangeCopy = Cpp::Forward<R>(range);
	auto whatCopy = Cpp::Forward<RW>(what);
	return D::StartsAdvanceWithAdvance(rangeCopy, whatCopy);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Concepts::IsAccessibleRangeWithLength<R>::_ &&
	Concepts::IsAccessibleRangeWithLength<RW>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<RW>, Concepts::ValueTypeOf<R>>::_ &&
	!(Concepts::HasData<R>::_ && Concepts::HasData<RW>::_ && Meta::IsAlmostPod<Concepts::ValueTypeOf<R>>::_),
bool> StartsWith(R&& range, RW&& what)
{
	if(range.Length()<what.Length()) return false;
	auto rangeCopy = Cpp::Forward<R>(range);
	auto whatCopy = Cpp::Forward<RW>(what);
	return D::StartsAdvanceWithAdvance(rangeCopy, whatCopy);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Concepts::IsArrayClass<R>::_ && Concepts::IsArrayClass<RW>::_ &&
	Meta::TypeEquals<Concepts::ElementTypeOfArray<RW>, Concepts::ElementTypeOfArray<R>>::_ &&
	Meta::IsAlmostPod<Concepts::ElementTypeOfArray<R>>::_,
bool> StartsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	return C::memcmp(range.Data(), what.Data(), what.Length()*sizeof(what.First()))==0;
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	(!Concepts::IsInputRange<R>::_ ||
		!Concepts::IsInputRange<RW>::_) &&
	Concepts::IsAsForwardRange<R>::_ && Concepts::IsAsForwardRange<RW>::_,
bool> StartsWith(R&& range, RW&& what)
{return StartsWith(Range::Forward<R>(range), Range::Forward<RW>(what));}

template<typename R, typename W> forceinline Meta::EnableIf<
	Concepts::IsForwardRange<R>::_ &&
	!Concepts::IsAsInputRange<W>::_,
bool> StartsWith(const R& range, const W& what)
{return !range.Empty() && range.First()==what;}

template<typename R, typename W> forceinline Meta::EnableIf<
	!Concepts::IsInputRange<R>::_ &&
	Concepts::IsAsForwardRange<R>::_ &&
	!Concepts::IsAsInputRange<W>::_,
bool> StartsWith(R&& range, const W& what)
{return StartsWith(Range::Forward<R>(range), what);}


//! Проверяет, начинается ли range с what.
//! Если да, то начало range смещается к первому элементу, идущему сразу после вхождения what.
//! Иначе range остаётся без изменений.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Concepts::IsForwardRange<R>::_ &&
	Concepts::IsAsConsumableRange<RW>::_,
bool> SkipIfStartsWith(R& range, RW&& what)
{
	auto whatCopy = Range::Forward<RW>(what);
	bool result = StartsWith(range, whatCopy);
	if(result) PopFirstExactly(range, Count(whatCopy));
	return result;
}

//! Проверяет, начинается ли range с what.
//! Если да, то начало range смещается к первому элементу, идущему сразу после вхождения what.
//! Иначе range остаётся без изменений.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	Concepts::IsAsConsumableRange<RW>::_,
bool> StartsAdvanceWith(R& range, RW&& what)
{
	auto whatCopy = Range::Forward<RW>(what);
	return D::StartsAdvanceWithAdvance(range, whatCopy);
}

template<typename R, typename RWs> Meta::EnableIf<
	Concepts::IsForwardRange<R>::_ &&
	Concepts::IsNonInfiniteInputRange<RWs>::_ &&
	Concepts::IsNonInfiniteForwardRange<Concepts::ValueTypeOf<RWs>>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<Concepts::ValueTypeOf<RWs>>, Concepts::ValueTypeOf<R>>::_,
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
	Concepts::IsAsForwardRange<R>::_ &&
	Concepts::IsAsConsumableRange<RWs>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<Concepts::ValueTypeOfAs<RWs>>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOfAs<Concepts::ValueTypeOfAs<RWs>>, Concepts::ValueTypeOfAs<R>>::_,
bool> StartsWithAny(R&& range, RWs&& subranges, size_t* oSubrangeIndex = null)
{
	auto subrangesCopy = Range::Forward<RWs>(subranges);
	return StartsWithAnyAdvance(Range::Forward<R>(range), subrangesCopy, oSubrangeIndex);
}

template<typename R, typename RWs,
	typename W = Concepts::ValueTypeOfAs<RWs>
> forceinline Meta::EnableIf<
	Concepts::IsForwardRange<R>::_ &&
	Concepts::IsAsConsumableRange<RWs>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<W>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOfAs<W>, Concepts::ValueTypeOf<R>>::_,
bool> StartsAdvanceWithAny(R& range, RWs&& subranges, size_t* oSubrangeIndex = null)
{
	auto subrangesCopy = Range::Forward<RWs>(subranges);
	bool result = StartsWithAnyAdvance(range, subrangesCopy, oSubrangeIndex);
	if(result) PopFirstExactly(range, Count(subrangesCopy.First()));
	return result;
}

INTRA_WARNING_POP

}}
