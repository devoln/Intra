#include "Cpp/Warnings.h"

#include "Sort.h"

#include "Range/Sort.hh"
#include "Container/Sequential/Array.h"
#include "IO/FormattedWriter.h"
#include "Utils/Debug.h"


INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <algorithm>
INTRA_WARNING_POP

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
using namespace Intra;

static const short arrayForSortTesting[] = {
	2, 4234, -9788, 23, 5, 245, 2, 24, 5, -9890,
	2, 5, 4552, 54, 3, -932, 123, 342, 24321, -234
};

void TestSort(IO::FormattedWriter& output)
{
	Array<short> arrUnsorted = arrayForSortTesting;
	output.PrintLine("Not sorted array: ", arrUnsorted);

	Array<short> arrStdSort = arrUnsorted;
	std::sort(arrStdSort.begin(), arrStdSort.end());
	output.PrintLine("std::sort'ed array: ", arrStdSort);
	INTRA_ASSERT1(Range::IsSorted(arrStdSort), arrStdSort);

	Array<short> arrInsertion = arrUnsorted;
	Range::InsertionSort(arrInsertion);
	output.PrintLine("InsertionSort'ed array: ", arrInsertion);
	INTRA_ASSERT_EQUALS(arrInsertion, arrStdSort);
	INTRA_ASSERT1(Range::IsSorted(arrInsertion), arrInsertion);

	Array<short> arrShell = arrUnsorted;
	Range::ShellSort(arrShell);
	output.PrintLine("ShellSort'ed array: ", arrShell);
	INTRA_ASSERT_EQUALS(arrShell, arrStdSort);
	INTRA_ASSERT1(Range::IsSorted(arrShell), arrShell);
	
	Array<short> arrQuick = arrUnsorted;
	Range::QuickSort(arrQuick);
	output.PrintLine("QuickSort'ed array: ", arrQuick);
	INTRA_ASSERT_EQUALS(arrQuick, arrStdSort);
	INTRA_ASSERT1(Range::IsSorted(arrQuick), arrQuick);
	
	Array<short> arrRadix = arrUnsorted;
	Range::RadixSort(arrRadix.AsRange());
	output.PrintLine("RadixSort'ed array: ", arrRadix);
	INTRA_ASSERT_EQUALS(arrRadix, arrStdSort);
	INTRA_ASSERT1(Range::IsSorted(arrRadix), arrRadix);
	
	Array<short> arrMerge = arrUnsorted;
	Range::MergeSort(arrMerge);
	output.PrintLine("MergeSort'ed array: ", arrMerge);
	INTRA_ASSERT_EQUALS(arrMerge, arrStdSort);
	INTRA_ASSERT1(Range::IsSorted(arrMerge), arrMerge);
	
	Array<short> arrHeap = arrUnsorted;
	Range::HeapSort(arrHeap);
	output.PrintLine("HeapSort'ed array: ", arrHeap);
	INTRA_ASSERT_EQUALS(arrHeap, arrStdSort);
	INTRA_ASSERT1(Range::IsSorted(arrHeap), arrHeap);
	
	Array<short> arrSelection = arrUnsorted;
	Range::SelectionSort(arrSelection);
	output.PrintLine("SelectionSort'ed array: ", arrSelection);
	INTRA_ASSERT_EQUALS(arrSelection, arrStdSort);
	INTRA_ASSERT1(Range::IsSorted(arrSelection), arrSelection);
};

INTRA_WARNING_POP
