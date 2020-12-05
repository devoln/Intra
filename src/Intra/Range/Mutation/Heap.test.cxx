#include "Intra/Range/Mutation/Heap.h"
#include "Intra/Container/Array.h"
#include "IntraX/System/Debug.h"

INTRA_BEGIN
INTRA_MODULE_UNITTEST
{
	int arr[] = {4, 7, 2, 6, 9, 2};
	Span<int> span = arr;
	HeapBuild(span, FGreater);
	INTRA_ASSERT_EQUALS(span.First(), 2);
	//Std.PrintLine("HeapBuild result: ", span); //TODO: replace with an assert
	int mins[6];
	auto minsOut = SpanOutput<int>(mins);
	while(!span.Empty())
	{
		minsOut.Put(HeapPop(span, FGreater));
		span.PopLast();
	}
	INTRA_ASSERT(Equals(
		minsOut.WrittenRange(),
		Array{2, 2, 4, 6, 7, 9}
	));
}
INTRA_WARNING_POP
