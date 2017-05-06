#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

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
	Concepts::IsArrayClass<R>::_ &&
	Concepts::IsArrayClass<RW>::_ &&
	Meta::TypeEquals<Concepts::ElementTypeOfArray<R>, Concepts::ElementTypeOfArray<RW>>::_,
bool> EndsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	return C::memcmp(range.Data()+range.Length()-what.Length(),
		what.Data(), what.Length()*sizeof(what.First()))==0;
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Concepts::IsBidirectionalRange<R>::_ &&
	Concepts::IsBidirectionalRange<RW>::_ &&
	!(Concepts::HasLength<R>::_ && Concepts::HasLength<RW>::_),
bool> EndsWith(R&& range, RW&& what)
{
	auto rangeCopy = Cpp::Forward<R>(range);
	auto whatCopy = Cpp::Forward<RW>(what);
	return D::EndsAdvanceWithAdvance(range, what);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Concepts::IsBidirectionalRangeWithLength<R>::_ &&
	Concepts::IsBidirectionalRangeWithLength<RW>::_ &&
	!(Concepts::HasData<R>::_ && Concepts::HasData<RW>::_),
bool> EndsWith(R&& range, RW&& what)
{
	if(range.Length()<what.Length()) return false;
	auto rangeCopy = Cpp::Forward<R>(range);
	auto whatCopy = Cpp::Forward<RW>(what);
	return D::EndsAdvanceWithAdvance(rangeCopy, whatCopy);
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	(!Concepts::IsInputRange<R>::_ || !Concepts::IsInputRange<RW>::_) &&
	Concepts::IsAsBidirectionalRange<R>::_ &&
	Concepts::IsAsBidirectionalRange<RW>::_,
bool> EndsWith(R&& range, RW&& what)
{return EndsWith(Range::Forward<R>(range), Range::Forward<RW>(what));}

template<typename R, typename W> forceinline Meta::EnableIf<
	Concepts::IsBidirectionalRange<R>::_ &&
	!Concepts::IsAsInputRange<W>::_,
bool> EndsWith(const R& range, const W& what)
{return !range.Empty() && range.Last()==what;}

template<typename R, typename W> forceinline Meta::EnableIf<
	!Concepts::IsInputRange<R>::_ &&
	Concepts::IsAsBidirectionalRange<R>::_ &&
	!Concepts::IsAsInputRange<W>::_,
bool> EndsWith(R&& range, const W& what)
{return EndsWith(Range::Forward<R>(range), what);}

}}

INTRA_WARNING_POP
