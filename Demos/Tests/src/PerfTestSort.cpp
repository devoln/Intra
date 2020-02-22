
INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include "PerfTestSort.h"
#include "Test/PerfSummary.h"
#include "Test/TestGroup.h"
#include "Test/TestData.h"
#include "Core/Range/Sort.hh"
#include "System/Stopwatch.h"
#include "IO/FormattedWriter.h"

using namespace Intra;

#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <algorithm>
INTRA_WARNING_POP


template<typename T, typename Comparer = Funal::TLess>
double TestInsertionSorting(size_t size, Comparer comparer = FLess)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Stopwatch tim;
	InsertionSort(arr, comparer);
	const double result = tim.ElapsedSeconds();
	INTRA_DEBUG_ASSERT(IsSorted(arr));
	return result;
}

template<typename T, typename Comparer = Funal::TLess>
double TestShellSorting(size_t size, Comparer comparer = FLess)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Stopwatch tim;
	ShellSort(arr, comparer);
	double result = tim.ElapsedSeconds();
	INTRA_DEBUG_ASSERT(IsSorted(arr));
	return result;
}

template<typename T, typename Comparer = Funal::TLess>
double TestQuickSorting(size_t size, Comparer comparer = FLess)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Stopwatch tim;
	QuickSort(arr, comparer);
	double result = tim.ElapsedSeconds();
	INTRA_DEBUG_ASSERT(IsSorted(arr));
	return result;
}

template<typename T> double TestRadixSorting(size_t size)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Stopwatch tim;
	RadixSort(arr.AsRange());
	double result = tim.ElapsedSeconds();
	INTRA_DEBUG_ASSERT(IsSorted(arr));
	return result;
}

template<typename T, typename Comparer = Funal::TLess>
double TestMergeSorting(size_t size, Comparer comparer = FLess)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Stopwatch tim;
	MergeSort(arr, comparer);
	double result = tim.ElapsedSeconds();
	INTRA_DEBUG_ASSERT(IsSorted(arr));
	return result;
}

template<typename T, typename Comparer = Funal::TLess>
double TestSelectionSorting(size_t size, Comparer comparer = FLess)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Stopwatch tim;
	SelectionSort(arr, comparer);
	const double result = tim.ElapsedSeconds();
	INTRA_DEBUG_ASSERT(IsSorted(arr));
	return result;
}

template<typename T, typename Comparer = Funal::TLess>
double TestHeapSorting(size_t size, Comparer comparer = FLess)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Stopwatch tim;
	HeapSort(arr, comparer);
	double result = tim.ElapsedSeconds();
	INTRA_DEBUG_ASSERT(IsSorted(arr));
	return result;
}

template<typename T, typename Comparer = Funal::TLess>
double TestStdSorting(size_t size, Comparer comparer = FLess)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Stopwatch tim;
	std::sort(arr.begin(), arr.end(), comparer);
	double result = tim.ElapsedSeconds();
	INTRA_DEBUG_ASSERT(IsSorted(arr));
	return result;
}

static const StringView comparedSorts[] = {
	"std::sort", "InsertionSort", "ShellSort", "QuickSort",
	"MergeSort", "SelectionSort", "HeapSort", "RadixSort"
};
static const StringView comparedSortsWithoutSlow[] = {
	"std::sort", "ShellSort", "QuickSort",
	"MergeSort", "HeapSort", "RadixSort"
};

template<typename T> void TestAndPrintIntegralTypeSorts(IO::FormattedWriter& output, StringView typeName)
{
	PrintPerformanceResults(output, typeName + " array size: 100",
		comparedSorts, {TestStdSorting<T>(100)},
		{
			TestInsertionSorting<T>(100),
			TestShellSorting<T>(100),
			TestQuickSorting<T>(100),
			TestMergeSorting<T>(100),
			TestSelectionSorting<T>(100),
			TestHeapSorting<T>(100),
			TestRadixSorting<T>(100)
		});

	PrintPerformanceResults(output, typeName + " array size: 1000",
		comparedSorts, {TestStdSorting<T>(1000)},
		{
			TestInsertionSorting<T>(1000),
			TestShellSorting<T>(1000),
			TestQuickSorting<T>(1000),
			TestMergeSorting<T>(1000),
			TestSelectionSorting<T>(1000),
			TestHeapSorting<T>(1000),
			TestRadixSorting<T>(1000)
		});

	PrintPerformanceResults(output, typeName + " array size: 10000",
		comparedSorts, {TestStdSorting<T>(10000)},
		{
			TestInsertionSorting<T>(10000),
			TestShellSorting<T>(10000),
			TestQuickSorting<T>(10000),
			TestMergeSorting<T>(10000),
			TestSelectionSorting<T>(10000),
			TestHeapSorting<T>(10000),
			TestRadixSorting<T>(10000)
		});

	PrintPerformanceResults(output, typeName + " array size: 100000",
		comparedSorts, {TestStdSorting<T>(100000)},
		{
			TestInsertionSorting<T>(100000),
			TestShellSorting<T>(100000),
			TestQuickSorting<T>(100000),
			TestMergeSorting<T>(100000),
			TestSelectionSorting<T>(100000),
			TestHeapSorting<T>(100000),
			TestRadixSorting<T>(100000)
		});

	PrintPerformanceResults(output, typeName + " array size: 1000000",
		comparedSortsWithoutSlow, {TestStdSorting<T>(1000000)},
		{
			TestShellSorting<T>(1000000),
			TestQuickSorting<T>(1000000),
			TestMergeSorting<T>(1000000),
			TestHeapSorting<T>(1000000),
			TestRadixSorting<T>(1000000)
		});

	PrintPerformanceResults(output, typeName + " array size: 10000000",
		comparedSortsWithoutSlow, {TestStdSorting<T>(10000000)},
		{
			TestShellSorting<T>(10000000),
			TestQuickSorting<T>(10000000),
			TestMergeSorting<T>(10000000),
			TestHeapSorting<T>(10000000),
			TestRadixSorting<T>(10000000)
		});
}

void RunSortPerfTests(FormattedWriter& output)
{
	if(TestGroup gr{"Sorting of random generated arrays of short"})
		TestAndPrintIntegralTypeSorts<short>(output, "short");

	if(TestGroup gr{"Sorting of random generated arrays of int"})
		TestAndPrintIntegralTypeSorts<int>(output, "int");

	if(TestGroup gr{"Sorting of random generated arrays of uint"})
		TestAndPrintIntegralTypeSorts<uint>(output, "uint");

	if(TestGroup gr{"Sorting of random generated arrays of int64"})
		TestAndPrintIntegralTypeSorts<int64>(output, "int64");
}

INTRA_WARNING_POP
