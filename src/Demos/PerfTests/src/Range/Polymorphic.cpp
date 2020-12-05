#include "Intra/Range/Stream.hh"
#include "Intra/Range/Reduction.h"

#include "Intra/Range/Span.h"

#include "Range.hh"

#include "Random/FastUniform.h"

#include "IntraX/Container/Sequential/List.h"

#include "IntraX/System/Stopwatch.h"

#include "Test/PerfSummary.h"

#include "IntraX/IO/Std.h"

using namespace Intra;

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

static int TestPolymorphicRange(IRange* range, size_t totalCount)
{
	int sum = 0;
	for(size_t i=0; i<totalCount; i++)
	{
		sum += range->First();
		range->PopFirst();
	}
	return sum;
}

static int TestPolymorphicRange2(InputRange<int> range, size_t totalCount)
{
	int sum = 0;
	for(size_t i=0; i<totalCount; i++)
	{
		sum += range.First();
		range.PopFirst();
	}
	return sum;
}

static int TestPolymorphicRange3(InputRange<int> range, size_t totalCount)
{
	int sum = 0;
	for(size_t i=0; i<totalCount; i++)
		sum += range.Next();
	return sum;
}

static int TestInlinedRange(int* arr, size_t count, size_t totalCount)
{
	int* mBegin = arr;
	int* mEnd = arr+count;
	int* mPtr = arr;
	int sum = 0;
	for(size_t i=0; i<totalCount; i++)
	{
		if(mPtr >= mEnd) mPtr = mBegin;
		sum += *mPtr++;
	}
	return sum;
}

static int TestStaticRange(int* arr, size_t count, size_t totalCount)
{
	auto cycle = Cycle(Span<int>(arr, count));
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

	int sum2 = TestPolymorphicRange2(Cycle(arr), 100000000);
	double time2 = tim.GetElapsedSecondsAndReset();

	int sum3 = TestPolymorphicRange3(Cycle(arr), 100000000);
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
