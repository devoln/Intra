#include "Span.h"

INTRA_BEGIN

#if INTRA_CONSTEXPR_TEST
static_assert(CHasLength<Span<float>>, "HasLength error.");
static_assert(CHasData<Span<int>>, "HasData error.");
static_assert(CArrayClass<Span<double>>, "CArrayClass error.");
static_assert(CInputRange<Span<float>>, "Not input range???");
static_assert(CForwardRange<Span<float>>, "Not forward range???");
static_assert(CInputRange<CSpan<int>>, "Not input range???");
static_assert(CForwardRange<CSpan<int>>, "Not forward range???");
static_assert(CRandomAccessRange<CSpan<uint>>, "Not random access range???");
static_assert(CFiniteRandomAccessRange<CSpan<int>>, "Not finite random access range???");
static_assert(CRandomAccessRange<Span<float>>, "IsRandomAccessRange error.");
static_assert(CFiniteRandomAccessRange<Span<float>>, "IsFiniteRandomAccessRange error.");
static_assert(CSame<TValueTypeOf<Span<float>>, float>, "IsArrayRange error.");
static_assert(CArrayRange<Span<float>>, "IsArrayRange error.");
static_assert(CArrayRange<CSpan<char>>, "IsArrayRange error.");

namespace CompileTimeTest_Span {
static constexpr int arr[] = {54, 21, 83, 64};
static constexpr auto span = SpanOf(arr);
static_assert(span == RangeOf(arr), "TEST FAILED!");
static_assert(Take(arr, 2).Length() == 2, "TEST FAILED!");
static_assert(Take(arr, 12).Length() == 4, "TEST FAILED!");
static_assert(Take(arr, 2)[0] == 54, "TEST FAILED!");
//static_assert(Take(arr, 2)[2] == 83, "TEST FAILED!"); //compiles in release, in debug the range check fails
static_assert(Take(arr, 2).Get(2, 123) == 123, "TEST FAILED!");
static_assert(!span.Drop().Empty(), "TEST FAILED!");
static_assert(SpanOfPtr(&arr[0], 3)[0] == 54, "TEST FAILED!");
static_assert(span.Drop(100) == null, "TEST FAILED!");
static_assert(span.Get(2, 123) == 83, "TEST FAILED!");
static_assert(Take(arr, 3).Drop(2) == SpanOf(arr).Drop(2).Take(1), "TEST FAILED!");
static_assert(span.Drop(1) == span.Tail(3), "TEST FAILED!");
static_assert(span(1, 3) == span.Drop().Take(2), "TEST FAILED!");
static_assert(span.Find(83) == span.Drop(2), "TEST FAILED!");
static_assert(span.Find(12) == null, "TEST FAILED!");
static_assert(span.FindBefore(83) == span.Take(2), "TEST FAILED!");
}
#endif

INTRA_END
