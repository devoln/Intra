#include "Range/Stream.hh"
#include "Range/Reduction.h"

#include "Utils/Span.h"

#include "Range.hh"

#include "Random/FastUniform.h"

#include "Container/Sequential/List.h"

#include "System/Stopwatch.h"

#include "Test/PerfSummary.h"

#include "IO/Std.h"

using namespace Intra;
using namespace IO;

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
	int First() final {return *mPtr;}
	void PopFirst() final {mPtr++; if(mPtr>=mEnd) mPtr=mBegin;}
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
		sum += range.Next();
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
	auto cycle = Range::Cycle(Span<int>(arr, count));
	int sum = 0;
	for(size_t i=0; i<totalCount; i++)
	{
		sum += cycle.First();
		cycle.PopFirst();
	}
	return sum;
}

void RunPolymorphicRangePerfTests(FormattedWriter& output)
{
	Array<int> arr;
	arr.SetCountUninitialized(1000);
	IRange* range = new CycledRange(arr.Data(), arr.Count());

	Stopwatch tim;
	int sum1 = TestPolymorphicRange(range, 100000000);
	double time1 = tim.GetElapsedSecondsAndReset();

	int sum2 = TestPolymorphicRange2(Range::Cycle(arr), 100000000);
	double time2 = tim.GetElapsedSecondsAndReset();

	int sum3 = TestPolymorphicRange3(Range::Cycle(arr), 100000000);
	double time3 = tim.GetElapsedSecondsAndReset();

	int sum4 = TestInlinedRange(arr.Data(), 1000, 100000000);
	double time4 = tim.GetElapsedSecondsAndReset();
		
	int sum5 = TestStaticRange(arr.Data(), 1000, 100000000);
	double time5 = tim.GetElapsedSecondsAndReset();

	Std.PrintLine(sum1, " ", sum2, " ", sum3, " ", sum4, " ", sum5);

	PrintPerformanceResults(output, "CycledRange 100000000 times",
		{"CycledRange*", "InputRange<int>", "InputRange<int>::GetNext", "manually inlined loop", "Span.Cycle"},
		{time1, time2, time3},
		{time4, time5});
}
