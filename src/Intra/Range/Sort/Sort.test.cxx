#include "Intra/Range/Sort/IsSorted.h"
#include "Intra/Range/Sort/Quick.h"
#include "Intra/Range/Sort/Insertion.h"
#include "Intra/Range/Sort/Merge.h"
#include "Intra/Range/Sort/Selection.h"
#include "Intra/Range/Sort/Heap.h"
#include "Intra/Range/Sort/Radix.h"
#include "Intra/Assert.h"
#include "IntraX/Container/Sequential/Array.h"
#include "IntraX/IO/FormattedWriter.h"
#include "IntraX/System/Debug.h"


INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <algorithm>
INTRA_WARNING_POP

INTRA_BEGIN

static const short arrayForSortTesting[] = {
	2, 4234, -9788, 23, 5, 245, 2, 24, 5, -9890,
	2, 5, 4552, 54, 3, -932, 123, 342, 24321, -234
};

INTRA_MODULE_UNITTEST
{
	Array<short> arrStdSort = arrayForSortTesting;
	std::sort(arrStdSort.begin(), arrStdSort.end());
	INTRA_ASSERT1(IsSorted(arrStdSort), arrStdSort);
	INTRA_ASSERT1(Equals(
		arrStdSort,
		CSpan<short>{-9890, -9788, -932, -234, 2, 2, 2, 3, 5, 5, 5, 23, 24, 54, 123, 245, 342, 4234, 4552, 24321}
	), arrStdSort);

	Array<short> arrInsertion = arrayForSortTesting;
	InsertionSort(arrInsertion);
	INTRA_ASSERT_EQUALS(arrInsertion, arrStdSort);

	Array<short> arrShell = arrayForSortTesting;
	ShellSort(arrShell);
	INTRA_ASSERT_EQUALS(arrShell, arrStdSort);
	
	Array<short> arrQuick = arrayForSortTesting;
	QuickSort(arrQuick);
	INTRA_ASSERT_EQUALS(arrQuick, arrStdSort);
	
	Array<short> arrRadix = arrayForSortTesting;
	RadixSort(SpanOf(arrRadix));
	INTRA_ASSERT_EQUALS(arrRadix, arrStdSort);
	
	Array<short> arrMerge = arrayForSortTesting;
	MergeSort(arrMerge);
	INTRA_ASSERT_EQUALS(arrMerge, arrStdSort);
	
	Array<short> arrHeap = arrayForSortTesting;
	HeapSort(arrHeap);
	INTRA_ASSERT_EQUALS(arrHeap, arrStdSort);
	
	Array<short> arrSelection = arrayForSortTesting;
	SelectionSort(arrSelection);
	INTRA_ASSERT_EQUALS(arrSelection, arrStdSort);
}

INTRA_END
