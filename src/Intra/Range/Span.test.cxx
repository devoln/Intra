#include "Span.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Take.h"
#include "Intra/Range/Search/Single.h"

INTRA_BEGIN

#if INTRA_CONSTEXPR_TEST
static_assert(CHasLength<Span<float>>);
static_assert(CHasData<Span<int>>);
static_assert(CArrayList<Span<double>>);
static_assert(CRange<Span<float>>);
static_assert(CForwardRange<Span<float>>);
static_assert(CRange<CSpan<int>>);
static_assert(CForwardRange<CSpan<int>>);
static_assert(CRandomAccessRange<CSpan<unsigned>>);
static_assert(CFiniteRandomAccessRange<CSpan<int>>);
static_assert(CRandomAccessRange<Span<float>>);
static_assert(CFiniteRandomAccessRange<Span<float>>);
static_assert(CSame<TRangeValue<Span<float>>, float>);
static_assert(CHasTakeMethod<CSpan<int>>);
static_assert(CHasPopFirstCount<CSpan<int>>);

namespace CompileTimeTest_Span {
static constexpr int arr[] = {54, 21, 83, 64};
static constexpr auto span = Span(arr);
static_assert(span.Data() == RangeOf(arr).Data() && span.Length() == RangeOf(arr).Length());
static_assert(Take(2)(arr).Length() == 2);
static_assert((arr|Take(2)|Count) == 2);
static_assert((arr|Take(12)|Count) == 4);
static_assert((arr|Take(2))[0] == 54);
//static_assert(Take(arr, 2)[2] == 83); //compiles in release, in debug the range check fails
//static_assert(Take(arr, 2).Get(2, 123) == 123);
static_assert(!(Drop(1)(arr)).Empty());
static_assert(!(span|Drop(1)).Empty());
static_assert(Span(Unsafe, &arr[0], 3)[0] == 54);
static_assert(span|Drop(100)|IsEmpty);
//static_assert(span.Get(2, 123) == 83);
static_assert((arr|Take(3)|Drop(2)).Data() == (Span(arr)|Drop(2)|Take(1)).Data());
static_assert((span|Drop(1)).Length() == (span|Tail(3)).Length());
static_assert((span|DropUntil(Bind(Equal, 83))).Data() == (span|Drop(2)).Data());
static_assert(span|DropUntil(Bind(Equal, 12))|IsEmpty);
//static_assert(span.FindBefore(83).Data() == span.Take(2).Data());
}
#endif

INTRA_END
