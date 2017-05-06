#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Concepts/Container.h"
#include "Cpp/Intrinsics.h"

#include "Range/Output/Inserter.h"
#include "Range/Operations.h"
#include "Range/Mutation/Copy.h"

namespace Intra { namespace Container {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename SC, typename R> inline Meta::EnableIf<
	(Concepts::Has_resize<SC>::_ &&
		Concepts::IsArrayClass<SC>::_) &&
	Meta::TypeEquals<Concepts::ElementTypeOfArray<SC>, Concepts::ElementTypeOfArray<R>>::_,
SC&> operator+=(SC& lhs, R&& rhs)
{
	using namespace Concepts;
	const size_t oldLen = LengthOf(lhs);
	SetCountTryNotInit(lhs, oldLen+LengthOf(rhs));
	C::memcpy(DataOf(lhs)+oldLen, DataOf(rhs), LengthOf(rhs));
	return lhs;
}

template<typename SC, typename R,
	typename AsR=Concepts::RangeOfType<R>
> Meta::EnableIf<
	Concepts::Has_resize<SC>::_ &&
	Concepts::IsArrayClass<SC>::_ &&
	!Meta::TypeEquals<Concepts::ElementTypeOfArray<SC>, Concepts::ElementTypeOfArray<R>>::_ &&
	Concepts::IsConsumableRangeOf<AsR, Concepts::ValueTypeOf<SC>>::_,
SC&> operator+=(SC& lhs, R&& rhs)
{
	using namespace Range;
	auto r = Range::Forward<R>(rhs);
	const size_t oldLen = Concepts::LengthOf(lhs);
	Concepts::SetCountTryNotInit(lhs, oldLen+Count(r));
	CopyAdvanceTo(r, Drop(lhs, oldLen));
	return lhs;
}

template<typename SC, typename R> inline Meta::EnableIf<
	!(Concepts::Has_resize<SC>::_ && Concepts::IsArrayClass<SC>::_) &&
	Concepts::Has_push_back<SC>::_ &&
	Concepts::IsAsConsumableRangeOf<R, Concepts::ValueTypeOf<SC>>::_,
SC&> operator+=(SC& lhs, R&& rhs)
{
	Range::LastAppender(lhs) << rhs;
	return lhs;
}


INTRA_WARNING_POP

}}
