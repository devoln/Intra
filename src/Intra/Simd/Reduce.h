#pragma once

#include "Simd.h"
#include <Intra/Range.h>

namespace Intra { INTRA_BEGIN
template<typename T, typename F> requires CCallable<F, TScalarOf<T>, TScalarOf<T>> && LengthOf(T()) == 4
[[nodiscard]] INTRA_FORCEINLINE TScalarOf<T> INTRA_VECTORCALL HorReduce(T x, F&& f)
{
	TScalarOf<T> v[4];
	SimdStore(x, v);
	return f(f(v[0], v[1]), f(v[2], v[3]));
}

template<typename T, typename F> requires CCallable<F, TScalarOf<T>, TScalarOf<T>> && LengthOf(T()) == 8
[[nodiscard]] INTRA_FORCEINLINE TScalarOf<T> INTRA_VECTORCALL HorReduce(T x, F&& f)
{
	TScalarOf<T> v[8];
	SimdStore(x, v);
	return f(
		f(f(v[0], v[1]), f(v[2], v[3])),
		f(f(v[4], v[5]), f(v[6], v[7])));
}

namespace z_D {
template<index_t N> constexpr T MergeAccums(T v[N], F&& f)
{
	if constexpr(N == 1) return v[0];
	else return f(MergeAccums<N/2>(v, f), MergeAccums<N - N/2>(v + N/2, f));
}
}

// Add, Mul, Min, Max are ~12 times faster than naive loop for float4 and ~24 times faster for float8.
//http://quick-bench.com/ZQ6L9WjXuu1qt2g2Xe1_tNGhG4k
// TODO: test on AMD processors, on ARM NEON, and with int4 and int8 types.
template<typename F, typename T, index_t NumAccumulators = 4, bool ProcessLeftOver = true> requires
	!CScalar<T> &&
	CCallable<F, TScalarOf<T>, TScalarOf<T>> &&
	CCallable<F, T, T>
[[nodiscard]] TScalarOf<T> INTRA_VECTORCALL Reduce(Span<const TScalarOf<T>> arr, F&& f)
{
	static_assert(IsPow2(NumAccumulators));
	constexpr index_t VectorSize = StaticLengthOf<T>,
		ElementsPerIteration = VectorSize*NumAccumulators,
		ArraySizeMask = ElementsPerIteration - 1;
	const index_t n = arr.Length();
	const index_t nMasked = n & ~ArraySizeMask;
	T accum[NumAccumulators];
	for(index_t i = 0; i < NumAccumulators; i++)
		accum[i] = SimdLoad<VectorSize>(arr.Data() + i*VectorSize);
	for(index_t i = ElementsPerIteration; i < nMasked; i += ElementsPerIteration)
	{
		for(index_t j = 0; j < NumAccumulators; j++)
			accum[j] = f(accum[j], SimdLoad<VectorSize>(arr.Data() + (i + j*VectorSize)));
	}
	accum[0] = z_D::MergeAccums<NumAccumulators>(accum, f);
	TScalarOf<T> res = 0;
	if constexpr(CSame<TRemoveConstRef<F>, decltype(Add)>) res = SimdHorSum(accum[0]);
	if constexpr(CSame<TRemoveConstRef<F>, decltype(Or)>) res = SimdHorOr(accum[0]);
	else
	{
		TScalarOf<T> v[VectorSize];
		SimdStore(x, v);
		if constexpr(VectorSize == 4) res = f(f(v[0], v[1]), f(v[2], v[3]));
		else if constexpr(VectorSize == 8) res = f(
			f(f(v[0], v[1]), f(v[2], v[3])),
			f(f(v[4], v[5]), f(v[6], v[7])));
	}
	if constexpr(ProcessLeftOver)
	{
		for(index_t i = nMasked; i < n; i++)
			res = f(res, arr[i]);
	}
	return res;
}

template<typename F, typename T> requires CScalar<T> && CCallable<F, T, T>
[[nodiscard]] TResultOf<F, T, T> INTRA_VECTORCALL Reduce(Span<const T> arr, F&& f)
{
	Reduce(arr, f);
}
} INTRA_END
