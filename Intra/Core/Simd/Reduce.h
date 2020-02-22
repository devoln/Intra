#pragma once

#include "Simd.h"
#include "Core/Range/Span.h"
#include "Core/Range/Reduce.h"

INTRA_BEGIN
namespace Simd {

template<typename T, typename F> INTRA_NODISCARD forceinline Requires<
	CCallable<F, TScalarOf<T>, TScalarOf<T>> &&
	LengthOf(T()) == 4,
TScalarOf<T>> INTRA_VECTORCALL HorReduce(T x, F&& f)
{
	TScalarOf<T> v[4];
	Store(x, v);
	return f(f(v[0], v[1]), f(v[2], v[3]));
}

template<typename T, typename F> INTRA_NODISCARD forceinline Requires<
	CCallable<F, TScalarOf<T>, TScalarOf<T>> &&
	LengthOf(T()) == 8,
TScalarOf<T>> INTRA_VECTORCALL HorReduce(T x, F&& f)
{
	TScalarOf<T> v[8];
	Store(x, v);
	return f(
		f(f(v[0], v[1]), f(v[2], v[3])),
		f(f(v[4], v[5]), f(v[6], v[7])));
}

namespace D {
template<index_t N> struct RecursiveReducer
{
	template<typename T, typename F> static INTRA_NODISCARD forceinline Requires<
		CCallable<F, T, T>,
	T> INTRA_VECTORCALL Call(T v[N], F&& f)
	{
		return f(
			RecursiveReducer<N/2>::Call(v, f),
			RecursiveReducer<N - N/2>::Call(v + N/2, f)
		);
	}
};

template<> struct RecursiveReducer<1>
{
	template<typename T, typename F> static INTRA_NODISCARD forceinline Requires<
		CCallable<F, T, T>,
	T> INTRA_VECTORCALL Call(T v[1], F&&) {return v[0];}
};
}

// Add, Mul, Min, Max are ~12 times faster than naive loop for float4 and ~24 times faster for float8.
//http://quick-bench.com/ZQ6L9WjXuu1qt2g2Xe1_tNGhG4k
// TODO: test on AMD processors, on ARM NEON, and with int4 and int8 types.
template<typename F, typename T, index_t NumAccumulators = 4, bool ProcessLeftOver = true> INTRA_NODISCARD Requires<
	!CScalar<T> &&
	CCallable<F, TScalarOf<T>, TScalarOf<T>> &&
	CCallable<F, T, T>,
TScalarOf<T>> INTRA_VECTORCALL Reduce(CSpan<TScalarOf<T>> arr, F&& f)
{
	static_assert(NumAccumulators > 0 && (NumAccumulators & (NumAccumulators - 1)) == 0, "NumAccumulators must be a power of 2!");
	enum: index_t {
		VectorSize = LengthOf(T()),
		ElementsPerIteration = VectorSize*NumAccumulators,
		ArraySizeMask = ElementsPerIteration - 1
	};
	const index_t n = arr.Length();
	T accum[NumAccumulators];
	for(index_t i = 0; i < NumAccumulators; i++)
		accum[i] = Load<T>(arr.Begin + i*VectorSize);
	for(index_t i = ElementsPerIteration; i < (n & ~ArraySizeMask); i += ElementsPerIteration)
	{
		for(index_t j = 0; j < NumAccumulators; j++)
			accum[j] = f(accum[j], Load<T>(arr.Begin + i + j*VectorSize));
	}
	auto res = HorReduce(D::RecursiveReducer<NumAccumulators>::Call(accum, f), f);
	if(ProcessLeftOver)
	{
		for(index_t i = (n & ~ArraySizeMask); i < n; i++)
			res = f(res, arr.Begin[i]);
	}
	return res;
}

template<typename F, typename T> INTRA_NODISCARD Requires<
	CScalar<T> &&
	CCallable<F, T, T>,
TResultOf<F, T, T>> INTRA_VECTORCALL Reduce(CSpan<T> arr, F&& f)
{
	Reduce(arr, f);
}


}
INTRA_END
