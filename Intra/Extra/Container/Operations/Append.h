#pragma once

#include "Core/CContainer.h"
#include "Core/Range/Inserter.h"
#include "Core/Range/Operations.h"
#include "Core/Range/Mutation/Copy.h"

INTRA_BEGIN
template<typename SC, typename R> inline Requires<
	CHas_resize<SC> && CTrivCopyCompatibleArrayWith<SC, R>,
SC&> operator+=(SC& lhs, R&& rhs)
{
	const size_t oldLen = LengthOf(lhs);
	SetCountTryNotInit(lhs, oldLen+LengthOf(rhs));
	Misc::C::memcpy(DataOf(lhs)+oldLen, DataOf(rhs), LengthOf(rhs));
	return lhs;
}

template<typename SC, typename R,
	typename AsR=TRangeOfType<R>
> Requires<
	CHas_resize<SC> && !CTrivCopyCompatibleArrayWith<SC, R> &&
	CArrayClass<SC> &&
	CConsumableRangeOf<AsR, TValueTypeOf<SC>>,
SC&> operator+=(SC& lhs, R&& rhs)
{
	auto r = ForwardAsRange<R>(rhs);
	const size_t oldLen = LengthOf(lhs);
	SetCountTryNotInit(lhs, oldLen+Count(r));
	ReadTo(r, Drop(lhs, oldLen));
	return lhs;
}

template<typename SC, typename R> inline Requires<
	!(CHas_resize<SC> && CTrivCopyCompatibleArrayWith<SC, R>) &&
	CHas_push_back<SC> &&
	CAsConsumableRangeOf<R, TValueTypeOf<SC>>,
SC&> operator+=(SC& lhs, R&& rhs)
{
	LastAppender(lhs) << rhs;
	return lhs;
}
INTRA_END
