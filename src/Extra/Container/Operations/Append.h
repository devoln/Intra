#pragma once

#include "Intra/Container/Concepts.h"
#include "Intra/Range/Inserter.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Operations.h"

INTRA_BEGIN
template<typename SC, typename R> inline Requires<
	CHas_resize<SC> && CTrivCopyCompatibleArrayWith<SC, R>,
SC&> operator+=(SC& lhs, R&& rhs)
{
	const auto oldLen = LengthOf(lhs);
	SetCountTryNotInit(lhs, oldLen + LengthOf(rhs));
	Misc::BitwiseCopyUnsafe(DataOf(lhs) + oldLen, DataOf(rhs), LengthOf(rhs));
	return lhs;
}

template<typename SC, typename R,
	typename AsR=TRangeOfRef<R>
> Requires<
	CHas_resize<SC> && !CTrivCopyCompatibleArrayWith<SC, R> &&
	CArrayClass<SC> &&
	CConsumableRangeOf<AsR, TValueTypeOf<SC>>,
SC&> operator+=(SC& lhs, R&& rhs)
{
	auto r = ForwardAsRange<R>(rhs);
	const auto oldLen = LengthOf(lhs);
	SetCountTryNotInit(lhs, oldLen + Count(r));
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
