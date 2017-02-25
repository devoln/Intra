#include "Sort.h"

#include "Algo/Sort.hh"
#include "Container/Sequential/Array.h"
#include "IO/FormattedWriter.h"
#include "Platform/Debug.h"
#include "Platform/CppWarnings.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

#ifdef _MSC_VER
#pragma warning(disable: 4350 4548 4702)
#endif

#include <algorithm>

using namespace Intra;

static const short arrayForSortTesting[] = {
	2, 4234, -9788, 23, 5, 245, 2, 24, 5, -9890,
	2, 5, 4552, 54, 3, -932, 123, 342, 24321, -234
};

void TestSort(IO::IFormattedWriter& output)
{
	Array<short> arrUnsorted = arrayForSortTesting;
	output.PrintLine("Not sorted array: ", arrUnsorted);

	Array<short> arrStdSort = arrUnsorted;
	std::sort(arrStdSort.begin(), arrStdSort.end());
	output.PrintLine("std::sort'ed array: ", arrStdSort);
	INTRA_ASSERT1(Algo::IsSorted(arrStdSort), arrStdSort);

	Array<short> arrInsertion = arrUnsorted;
	Algo::InsertionSort(arrInsertion);
	output.PrintLine("InsertionSort'ed array: ", arrInsertion);
	INTRA_ASSERT_EQUALS(arrInsertion, arrStdSort);
	INTRA_ASSERT1(Algo::IsSorted(arrInsertion), arrInsertion);

	Array<short> arrShell = arrUnsorted;
	Algo::ShellSort(arrShell);
	output.PrintLine("ShellSort'ed array: ", arrShell);
	INTRA_ASSERT_EQUALS(arrShell, arrStdSort);
	INTRA_ASSERT1(Algo::IsSorted(arrShell), arrShell);
	
	Array<short> arrQuick = arrUnsorted;
	Algo::QuickSort(arrQuick);
	output.PrintLine("QuickSort'ed array: ", arrQuick);
	INTRA_ASSERT_EQUALS(arrQuick, arrStdSort);
	INTRA_ASSERT1(Algo::IsSorted(arrQuick), arrQuick);
	
	Array<short> arrRadix = arrUnsorted;
	Algo::RadixSort(arrRadix.AsRange());
	output.PrintLine("RadixSort'ed array: ", arrRadix);
	INTRA_ASSERT_EQUALS(arrRadix, arrStdSort);
	INTRA_ASSERT1(Algo::IsSorted(arrRadix), arrRadix);
	
	Array<short> arrMerge = arrUnsorted;
	Algo::MergeSort(arrMerge);
	output.PrintLine("MergeSort'ed array: ", arrMerge);
	INTRA_ASSERT_EQUALS(arrMerge, arrStdSort);
	INTRA_ASSERT1(Algo::IsSorted(arrMerge), arrMerge);
	
	Array<short> arrHeap = arrUnsorted;
	Algo::HeapSort(arrHeap);
	output.PrintLine("HeapSort'ed array: ", arrHeap);
	INTRA_ASSERT_EQUALS(arrHeap, arrStdSort);
	INTRA_ASSERT1(Algo::IsSorted(arrHeap), arrHeap);
	
	Array<short> arrSelection = arrUnsorted;
	Algo::SelectionSort(arrSelection);
	output.PrintLine("SelectionSort'ed array: ", arrSelection);
	INTRA_ASSERT_EQUALS(arrSelection, arrStdSort);
	INTRA_ASSERT1(Algo::IsSorted(arrSelection), arrSelection);
};
