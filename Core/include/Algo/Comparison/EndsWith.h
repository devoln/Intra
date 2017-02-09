#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

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
	Range::IsArrayRange<R>::_ &&
	Range::IsArrayRange<RW>::_ && Meta::IsConvertible<Range::ValueTypeOf<RW>, Range::ValueTypeOf<R>>::_,
bool> EndsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	return C::memcmp(range.Data()+range.Length()-what.Length(),
		what.Data(), what.Length()*sizeof(what.First()))==0;
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_ &&
	Range::IsBidirectionalRange<RW>::_ &&
	Meta::IsConvertible<Range::ValueTypeOf<RW>, Range::ValueTypeOf<R>>::_ &&
	!(Range::HasLength<R>::_ && Range::HasLength<RW>::_),
bool> EndsWith(R&& range, RW&& what)
{
	auto rangeCopy = Meta::Forward<R>(range);
	auto whatCopy = Meta::Forward<RW>(what);
	return D::EndsAdvanceWithAdvance(range, what);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_ &&
	Range::IsBidirectionalRange<RW>::_ &&
	Meta::IsConvertible<Range::ValueTypeOf<RW>, Range::ValueTypeOf<R>>::_ &&
	Range::HasLength<R>::_ && Range::HasLength<RW>::_ &&
	!(Range::HasData<R>::_ && Range::HasData<RW>::_),
bool> EndsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	R rangeCopy = range;
	RW whatCopy = what;
	return D::EndsAdvanceWithAdvance(rangeCopy, whatCopy);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	(!Range::IsInputRange<R>::_ || !Range::IsInputRange<RW>::_) &&
	Range::IsAsBidirectionalRange<R>::_ &&
	Range::IsAsBidirectionalRange<RW>::_,
bool> EndsWith(R&& range, RW&& what)
{return EndsWith(Range::Forward<R>(range), Range::Forward<RW>(what));}

INTRA_WARNING_POP

}}
