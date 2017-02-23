#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Container/Concepts.h"
#include "Platform/Intrinsics.h"
#include "Extension.h"
#include "Range/Output/Inserter.h"

namespace Intra { namespace Container {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename SC, typename R, typename AsR=Range::AsRangeResult<R>> inline Meta::EnableIf<
	(Has_resize<SC>::_ && Has_data<SC>::_ && Has_size<SC>::_) &&
	Range::IsArrayClass<AsR>::_ && Meta::TypeEquals<ValueTypeOf<SC>, Range::ValueTypeOf<AsR>>::_,
SC&> operator+=(SC& lhs, R&& rhs)
{
	auto r = Range::Forward<R>(rhs);
	size_t oldLen = lhs.size();
	Container::SetCountTryNotInit(lhs, oldLen+r.Length());
	C::memcpy(lhs.data()+oldLen, r.Data(), r.Length());
	return lhs;
}

template<typename SC, typename R, typename AsR=Range::AsRangeResult<R>> inline Meta::EnableIf<
	(Has_resize<SC>::_ && Has_data<SC>::_ && Has_size<SC>::_) &&
	!(Range::IsArrayClass<AsR>::_ && Meta::TypeEquals<ValueTypeOf<SC>, Range::ValueTypeOf<AsR>>::_) &&
	Range::IsConsumableRangeOf<AsR, ValueTypeOf<SC>>::_,
SC&> operator+=(SC& lhs, R&& rhs)
{
	auto r = Range::Forward<R>(rhs);
	size_t oldLen = lhs.size();
	Container::SetCountTryNotInit(lhs, oldLen+Range::Count(r));
	Algo::CopyAdvanceTo(r, Range::Drop(lhs, oldLen));
	return lhs;
}

template<typename SC, typename R> inline Meta::EnableIf<
	!(Has_resize<SC>::_ && Has_data<SC>::_ && Has_size<SC>::_) &&
	Has_push_back<SC>::_ &&
	Range::IsAsConsumableRangeOf<R, ValueTypeOf<SC>>::_,
SC&> operator+=(SC& lhs, R&& rhs)
{
	Range::LastAppender(lhs) << rhs;
	return lhs;
}


INTRA_WARNING_POP

}}
