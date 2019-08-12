#include "Core/Range/Chain.h"
#include "Core/Range/Comparison/Equals.h"
#include "Core/Range/Generators/Iota.h"
#include "Core/Range/Cycle.h"
#include "Core/Assert.h"

INTRA_BEGIN


INTRA_MODULE_UNITTEST
{
	{
		int arr1[] = {1, 2, 3, 4};
		int arr2[] = {5, 6};
		int arr3[] = {7};
		int witness[] = {1, 2, 3, 4, 5, 6, 7};
		auto s1 = Chain(arr1);
		static_assert(CRandomAccessRange<decltype(s1)>, "TEST FAILED!");
		auto s2 = Chain(arr1, arr2);
		static_assert(CBidirectionalRange<decltype(s2)>, "TEST FAILED!");
		static_assert(CRandomAccessRange<decltype(s2)>, "TEST FAILED!");
		s2.First() = 1;
		auto s = Chain(arr1, arr2, arr3);
		INTRA_ASSERT(s[5] == 6);
		INTRA_ASSERT(Equals(s, witness));
		INTRA_ASSERT(s[5] == 6);
	}
	{
		int arr1[] = {1, 2, 3, 4};
		int witness[] = {1, 2, 3, 4};
		INTRA_ASSERT(Equals(Chain(arr1), witness));
	}
	{
		uint foo[] = {1,2,3,4,5};
		uint bar[] = {1,2,3,4,5};
		auto c = Chain(foo, bar);
		c[3] = 42;
		INTRA_ASSERT(c[3] == 42);
	}

	// Make sure bug 3311 is fixed.  ChainImpl should compile even if not all
	// elements are mutable.
	INTRA_ASSERT(Equals(Chain(Iota(0, 3), Iota(0, 3)),SpanOf({0, 1, 2, 0, 1, 2})));

	// Test the case where infinite ranges are present.
	auto inf = Chain(CSpanOf({0,1,2}), Cycle(CSpanOf({4, 5, 6})), CSpanOf({7, 8, 9})); // infinite range
	INTRA_ASSERT(inf[0] == 0);
	INTRA_ASSERT(inf[3] == 4);
	INTRA_ASSERT(inf[6] == 4);
	INTRA_ASSERT(inf[7] == 5);
	static_assert(CInfiniteRange<decltype(inf)>, "TEST FAILED!");

	const int immi[] = {1, 2, 3};
	const float immf[] = {1, 2, 3};
	Chain(immi, immf);
}

}