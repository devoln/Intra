#include "Range.h"

#include "Cpp/Warnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include "Range/Mutation/Heap.h"
#include "IO/FormattedWriter.h"

#include <algorithm>

using namespace Intra;
using namespace IO;
using namespace Range;


void TestHeap(FormattedWriter& output)
{
	int arr[] = {4, 7, 2, 6, 9, 2};
	Span<int> span = arr;
	output.PrintLine("Building min-heap on array: ", span);
	HeapBuild(span, Funal::Greater);
	INTRA_ASSERT_EQUALS(span.First(), 2);
	output.PrintLine("Result: ", span);
	output.Print("Min elements: ");
	while(!span.Empty())
	{
		output.Print(HeapPop(span, Funal::Greater), ' ');
		span.PopLast();
	}
	output.LineBreak();
}


INTRA_WARNING_POP
