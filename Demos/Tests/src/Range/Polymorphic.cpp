#include "IO/Stream.h"
#include "Range/Stream.h"
#include "Algo/Reduction.h"
#include "Range/Generators/ArrayRange.h"
#include "Range.hh"
#include "Math/MathRanges.h"
#include "Math/Random.h"
#include "Container/Sequential/List.h"
#include "Platform/Time.h"
#include "Test/PerfSummary.h"

using namespace Intra;
using namespace Intra::IO;

struct IRange
{
	virtual ~IRange() {}
	virtual int First() = 0;
	virtual void PopFirst() = 0;
};

class CycledRange: public IRange
{
	int* mBegin;
	int* mEnd;
	int* mPtr;
public:
	CycledRange(int* arr, size_t count): mBegin(arr), mEnd(arr+count), mPtr(arr) {}
	int First() override final {return *mPtr;}
	void PopFirst() override final {mPtr++; if(mPtr>=mEnd) mPtr=mBegin;}
};

int TestPolymorphicRange(IRange* range, size_t totalCount)
{
	int sum = 0;
	for(size_t i=0; i<totalCount; i++)
	{
		sum += range->First();
		range->PopFirst();
	}
	return sum;
}

int TestPolymorphicRange2(InputRange<int> range, size_t totalCount)
{
	int sum = 0;
	for(size_t i=0; i<totalCount; i++)
	{
		sum += range.First();
		range.PopFirst();
	}
	return sum;
}

int TestPolymorphicRange3(InputRange<int> range, size_t totalCount)
{
	int sum = 0;
	for(size_t i=0; i<totalCount; i++)
		sum += range.GetNext();
	return sum;
}

int TestInlinedRange(int* arr, size_t count, size_t totalCount)
{
	int* mBegin = arr;
	int* mEnd = arr+count;
	int* mPtr = arr;
	int sum = 0;
	for(size_t i=0; i<totalCount; i++)
	{
		if(mPtr>=mEnd) mPtr=mBegin;
		sum += *mPtr++;
	}
	return sum;
}

int TestStaticRange(int* arr, size_t count, size_t totalCount)
{
	auto cycle = Range::Cycle(ArrayRange<int>(arr, count));
	int sum = 0;
	for(size_t i=0; i<totalCount; i++)
	{
		sum += cycle.First();
		cycle.PopFirst();
	}
	return sum;
}

void RunPolymorphicRangePerfTests(IO::IFormattedWriter& output)
{
	Array<int> arr;
	arr.SetCountUninitialized(1000);
	IRange* range = new CycledRange(arr.Data(), arr.Count());

	Timer tim;
	int sum1 = TestPolymorphicRange(range, 100000000);
	double time1 = tim.GetTimeAndReset();

	int sum2 = TestPolymorphicRange2(Range::Cycle(arr), 100000000);
	double time2 = tim.GetTimeAndReset();

	int sum3 = TestPolymorphicRange3(Range::Cycle(arr), 100000000);
	double time3 = tim.GetTimeAndReset();

	int sum4 = TestInlinedRange(arr.Data(), 1000, 100000000);
	double time4 = tim.GetTimeAndReset();
		
	int sum5 = TestStaticRange(arr.Data(), 1000, 100000000);
	double time5 = tim.GetTimeAndReset();

	Console.PrintLine(sum1, " ", sum2, " ", sum3, " ", sum4, " ", sum5);

	PrintPerformanceResults(output, "CycledRange 100000000 times",
		{"CycledRange*", "InputRange<int>", "InputRange<int>::GetNext", "manually inlined loop", "ArrayRange.Cycle"},
		{time1, time2, time3},
		{time4, time5});
}
