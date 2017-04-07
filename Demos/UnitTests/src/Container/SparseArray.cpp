#include "SparseArray.h"
#include "Range/Special/SparseRange.h"
#include "Container/Utility/SparseArray.h"
#include "Utils/Finally.h"

using namespace Intra;
using namespace IO;
using namespace Range;

void TestSparseArray(FormattedWriter& output)
{
	SparseArray<int, int> arr;
	INTRA_ASSERT(arr.Empty());
	INTRA_ASSERT(arr.IsFull());
	output.PrintLine("Appending 7, 5 and 2 to array.");
	arr.Add(7);
	INTRA_ASSERT(!arr.Empty());
	arr.Add(5);
	arr.Add(2);
	output.PrintLine("Removing element at index 1.");
	arr.Remove(1);
	INTRA_ASSERT(!arr.IsFull());
	INTRA_ASSERT(!arr.Empty());

	output.PrintLine("arr[0] = ", arr[0]);
	INTRA_ASSERT_EQUALS(arr[0], 7);
	output.PrintLine("arr[2] = ", arr[2]);
	INTRA_ASSERT_EQUALS(arr[2], 2);
}

void TestSparseRange(FormattedWriter& output)
{
	SparseRange<int, int> range;
	INTRA_ASSERT(range.Empty());
	INTRA_ASSERT(range.IsFull());
	size_t count = 3;
	range = Memory::AllocateRangeUninitialized<int>(Memory::GlobalHeap, count, INTRA_SOURCE_INFO);
	auto cleanupRange = Finally([&range]() {
		range.Clear();
		Memory::FreeRangeUninitialized(Memory::GlobalHeap, range.GetInternalDataBuffer());
	});
	INTRA_ASSERT(!range.IsFull());
	output.PrintLine("Appending 7, 5 and 2 to a range.");
	range.Add(7);
	range.Add(5);
	range.Add(2);
	INTRA_ASSERT(range.IsFull());
	output.PrintLine("Removing element at index 1.");
	range.Remove(1);
	INTRA_ASSERT(!range.IsFull());
	INTRA_ASSERT(!range.Empty());
	output.PrintLine("range[0] = ", range[0]);
	INTRA_ASSERT_EQUALS(range[0], 7);
	output.PrintLine("range[2] = ", range[2]);
	INTRA_ASSERT_EQUALS(range[2], 2);
}
