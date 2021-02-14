#pragma once

#include "Simd.h"

namespace Intra { INTRA_BEGIN
template<typename T, typename F, bool ProcessLeftOver = true> requires !CScalar<T> &&
	CCallable<F, TScalarOf<T>, TScalarOf<T>> &&
	CCallable<F, T, T>
[[nodiscard]] void SimdTransform(Span<const TScalarOf<T>> arr, F&& f)
{
}
} INTRA_END
