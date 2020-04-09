#include "Span.h"

INTRA_BEGIN

#if INTRA_CONSTEXPR_TEST
static_assert(CHasLength<Span<float>>);
static_assert(CHasData<Span<int>>);
static_assert(CArrayClass<Span<double>>);
static_assert(CInputRange<Span<float>>);
static_assert(CForwardRange<Span<float>>);
static_assert(CInputRange<CSpan<int>>);
static_assert(CForwardRange<CSpan<int>>);
static_assert(CRandomAccessRange<CSpan<unsigned>>);
static_assert(CFiniteRandomAccessRange<CSpan<int>>);
static_assert(CRandomAccessRange<Span<float>>);
static_assert(CFiniteRandomAccessRange<Span<float>>);
static_assert(CSame<TValueTypeOf<Span<float>>, float>);
static_assert(CArrayRange<Span<float>>);
static_assert(CArrayRange<CSpan<char>>);

namespace CompileTimeTest_Span {
static constexpr int arr[] = {54, 21, 83, 64};
static constexpr auto span = SpanOf(arr);
static_assert(span.Data() == RangeOf(arr).Data() && span.Length() == RangeOf(arr).Length());
static_assert(Take(arr, 2).Length() == 2);
static_assert(Take(arr, 12).Length() == 4);
static_assert(Take(arr, 2)[0] == 54);
//static_assert(Take(arr, 2)[2] == 83); //compiles in release, in debug the range check fails
static_assert(Take(arr, 2).Get(2, 123) == 123);
static_assert(!span.Drop().Empty());
static_assert(SpanOfPtr(&arr[0], 3)[0] == 54);
static_assert(span.Drop(100) == null);
static_assert(span.Get(2, 123) == 83);
static_assert(Take(arr, 3).Drop(2).Data() == SpanOf(arr).Drop(2).Take(1).Data());
static_assert(span.Drop(1).Length() == span.Tail(3).Length());
static_assert(span.Find(83).Data() == span.Drop(2).Data());
static_assert(span.Find(12) == null);
static_assert(span.FindBefore(83).Data() == span.Take(2).Data());
}
#endif

INTRA_END
