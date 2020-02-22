#include "Range.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include "Core/Range/Mutation/Heap.h"
#include "IO/FormattedWriter.h"

using namespace Intra;

void TestHeap(FormattedWriter& output)
{
	int arr[] = {4, 7, 2, 6, 9, 2};
	Span<int> span = arr;
	output.PrintLine("Building min-heap on array: ", span);
	HeapBuild(span, FGreater);
	INTRA_ASSERT_EQUALS(span.First(), 2);
	output.PrintLine("Result: ", span);
	output.Print("Min elements: ");
	while(!span.Empty())
	{
		output.Print(HeapPop(span, FGreater), ' ');
		span.PopLast();
	}
	output.LineBreak();
}

INTRA_WARNING_POP
