#include "Intra/Range/Composition.h"
#include "Intra/Range/Comparison.h"
#include "Intra/Range/Generators.h"
#include "Intra/Range/Decorators.h"
#include "IntraX/System/Debug.h"

namespace Intra { INTRA_BEGIN

//INTRA_CONSTEXPR_MODULE_UNITTEST(Chain)
INTRA_MODULE_UNITTEST
{
	{
		int arr1[] = {1, 2, 3, 4};
		int arr2[] = {5, 6};
		int arr3[] = {7};
		auto s1 = Chain(arr1);
		static_assert(CRandomAccessRange<decltype(s1)>);
		auto s2 = Chain(arr1, arr2);
		static_assert(CBidirectionalRange<decltype(s2)>);
		static_assert(CRandomAccessRange<decltype(s2)>);
		static_assert(CReference<decltype(Chain(arr1).First())>);
		static_assert(CSame<int&, decltype(Chain(arr1).First())>);
		static_assert(CSame<int&, decltype(Chain(arr1, arr2).First())>);
		s2.First() = 1;
		auto s = Chain(arr1, arr2, arr3);
		INTRA_ASSERT(s[5] == 6);
		INTRA_ASSERT(s|MatchesWith(Array{1, 2, 3, 4, 5, 6, 7}));
		INTRA_ASSERT(s[5] == 6);
	}
	{
		int arr1[] = {1, 2, 3, 4};
		int witness[] = {1, 2, 3, 4};
		INTRA_ASSERT(Chain(arr1)|MatchesWith(witness));
	}
	{
		unsigned foo[] = {1,2,3,4,5};
		unsigned bar[] = {1,2,3,4,5};
		auto c = Chain(foo, bar);
		c[3] = 42;
		INTRA_ASSERT(c[3] == 42);
	}

	INTRA_ASSERT(Chain(Iota(0, 3), Iota(1, 4))|MatchesWith(Array{0, 1, 2, 1, 2, 3}));

	// Test the case where infinite ranges are present.
	auto inf = Chain(Array{0, 1, 2}, Array{7, 8, 9}, Array{4, 5, 6}|Cycle);
	INTRA_ASSERT(inf[0] == 0);
	INTRA_ASSERT(inf[3] == 4);
	INTRA_ASSERT(inf[6] == 4);
	INTRA_ASSERT(inf[7] == 5);
	static_assert(CInfiniteRange<decltype(inf)>);
}

//INTRA_CONSTEXPR_TEST_RUN(Chain);
} INTRA_END
