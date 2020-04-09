#pragma once

#include "Simd.h"

INTRA_BEGIN
namespace Simd {

template<typename T, typename F, bool ProcessLeftOver = true> [[nodiscard]] Requires<
	!CScalar<T> &&
	CCallable<F, TScalarOf<T>, TScalarOf<T>> &&
	CCallable<F, T, T>
> INTRA_VECTORCALL Transform(CSpan<TScalarOf<T>> arr, F&& f)
{
}

}
INTRA_END
