#pragma once

#include "Core/Core.h"
#include "Core/CContainer.h"
#include "Core/Range/Inserter.h"
#include "Core/Range/Operations.h"
#include "Core/Range/Mutation/Copy.h"

INTRA_BEGIN
namespace Container {

template<typename SC, typename R> inline Requires<
	CHas_resize<SC> && CTrivCopyCompatibleArrayWith<SC, R>,
SC&> operator+=(SC& lhs, R&& rhs)
{
	using namespace Core;
	const size_t oldLen = LengthOf(lhs);
	SetCountTryNotInit(lhs, oldLen+LengthOf(rhs));
	C::memcpy(DataOf(lhs)+oldLen, DataOf(rhs), LengthOf(rhs));
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
	using namespace Range;
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

}
INTRA_END
