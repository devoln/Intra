#include "Core/Core.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include "Sort.h"

#include "Core/Range/Sort/IsSorted.h"
#include "Core/Range/Sort/Quick.h"
#include "Core/Range/Sort/Insertion.h"
#include "Core/Range/Sort/Merge.h"
#include "Core/Range/Sort/Selection.h"
#include "Core/Range/Sort/Heap.h"
#include "Core/Range/Sort/Radix.h"
#include "Container/Sequential/Array.h"
#include "IO/FormattedWriter.h"
#include "Core/Assert.h"


INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <algorithm>
INTRA_WARNING_POP

using namespace Intra;

static const short arrayForSortTesting[] = {
	2, 4234, -9788, 23, 5, 245, 2, 24, 5, -9890,
	2, 5, 4552, 54, 3, -932, 123, 342, 24321, -234
};

void TestSort(FormattedWriter& output)
{
	Array<short> arrUnsorted = arrayForSortTesting;
	output.PrintLine("Not sorted array: ", arrUnsorted);

	Array<short> arrStdSort = arrUnsorted;
	std::sort(arrStdSort.begin(), arrStdSort.end());
	output.PrintLine("std::sort'ed array: ", arrStdSort);
	INTRA_ASSERT1(IsSorted(arrStdSort), arrStdSort);

	Array<short> arrInsertion = arrUnsorted;
	InsertionSort(arrInsertion);
	output.PrintLine("InsertionSort'ed array: ", arrInsertion);
	INTRA_ASSERT_EQUALS(arrInsertion, arrStdSort);
	INTRA_ASSERT1(IsSorted(arrInsertion), arrInsertion);

	Array<short> arrShell = arrUnsorted;
	ShellSort(arrShell);
	output.PrintLine("ShellSort'ed array: ", arrShell);
	INTRA_ASSERT_EQUALS(arrShell, arrStdSort);
	INTRA_ASSERT1(IsSorted(arrShell), arrShell);
	
	Array<short> arrQuick = arrUnsorted;
	QuickSort(arrQuick);
	output.PrintLine("QuickSort'ed array: ", arrQuick);
	INTRA_ASSERT_EQUALS(arrQuick, arrStdSort);
	INTRA_ASSERT1(IsSorted(arrQuick), arrQuick);
	
	Array<short> arrRadix = arrUnsorted;
	RadixSort(arrRadix.AsRange());
	output.PrintLine("RadixSort'ed array: ", arrRadix);
	INTRA_ASSERT_EQUALS(arrRadix, arrStdSort);
	INTRA_ASSERT1(IsSorted(arrRadix), arrRadix);
	
	Array<short> arrMerge = arrUnsorted;
	MergeSort(arrMerge);
	output.PrintLine("MergeSort'ed array: ", arrMerge);
	INTRA_ASSERT_EQUALS(arrMerge, arrStdSort);
	INTRA_ASSERT1(IsSorted(arrMerge), arrMerge);
	
	Array<short> arrHeap = arrUnsorted;
	HeapSort(arrHeap);
	output.PrintLine("HeapSort'ed array: ", arrHeap);
	INTRA_ASSERT_EQUALS(arrHeap, arrStdSort);
	INTRA_ASSERT1(IsSorted(arrHeap), arrHeap);
	
	Array<short> arrSelection = arrUnsorted;
	SelectionSort(arrSelection);
	output.PrintLine("SelectionSort'ed array: ", arrSelection);
	INTRA_ASSERT_EQUALS(arrSelection, arrStdSort);
	INTRA_ASSERT1(IsSorted(arrSelection), arrSelection);
}

INTRA_WARNING_POP
