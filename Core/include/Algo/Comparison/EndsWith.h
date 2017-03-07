#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

using namespace Range::Concepts;

namespace D {

template<typename R, typename RW> bool EndsAdvanceWithAdvance(R& range, RW& what)
{
	while(!what.Empty())
	{
		if(range.Empty()) return false;
		if(range.Last()!=what.Last()) return false;
		range.PopLast();
		what.PopLast();
	}
	return true;
}

}

template<typename R, typename RW> forceinline Meta::EnableIf<
	IsArrayClass<R>::_ && IsArrayClass<RW>::_ &&
	Meta::TypeEqualsIgnoreCV<ValueTypeOf<R>, ValueTypeOf<RW>>::_,
bool> EndsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	return C::memcmp(range.Data()+range.Length()-what.Length(),
		what.Data(), what.Length()*sizeof(what.First()))==0;
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	IsBidirectionalRange<R>::_ && IsBidirectionalRange<RW>::_ &&
	!(HasLength<R>::_ && HasLength<RW>::_),
bool> EndsWith(R&& range, RW&& what)
{
	auto rangeCopy = Meta::Forward<R>(range);
	auto whatCopy = Meta::Forward<RW>(what);
	return D::EndsAdvanceWithAdvance(range, what);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	IsBidirectionalRangeWithLength<R>::_ && IsBidirectionalRangeWithLength<RW>::_ &&
	!(HasData<R>::_ && HasData<RW>::_),
bool> EndsWith(R&& range, RW&& what)
{
	if(range.Length()<what.Length()) return false;
	auto rangeCopy = Meta::Forward<R>(range);
	auto whatCopy = Meta::Forward<RW>(what);
	return D::EndsAdvanceWithAdvance(rangeCopy, whatCopy);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	(!IsInputRange<R>::_ || !IsInputRange<RW>::_) &&
	IsAsBidirectionalRange<R>::_ && IsAsBidirectionalRange<RW>::_,
bool> EndsWith(R&& range, RW&& what)
{return EndsWith(Range::Forward<R>(range), Range::Forward<RW>(what));}

template<typename R, typename W> forceinline Meta::EnableIf<
	IsBidirectionalRange<R>::_ && !IsAsInputRange<W>::_,
bool> EndsWith(const R& range, const W& what)
{return !range.Empty() && range.Last()==what;}

template<typename R, typename W> forceinline Meta::EnableIf<
	!IsInputRange<R>::_ && IsAsBidirectionalRange<R>::_ && !IsAsInputRange<W>::_,
bool> EndsWith(R&& range, const W& what)
{return EndsWith(Range::Forward<R>(range), what);}

INTRA_WARNING_POP

}}
