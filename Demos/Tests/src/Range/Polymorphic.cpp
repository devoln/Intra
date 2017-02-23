#include "Platform/CppWarnings.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include "Header.h"
#include "IO/Stream.h"
#include "Range/Stream.h"
#include "Algo/Reduction.h"
#include "Range/Generators/ArrayRange.h"
#include "Range.hh"
#include "Math/MathRanges.h"
#include "Math/Random.h"
#include "Container/Sequential/List.h"

#include <stdlib.h>

using namespace Intra;
using namespace Intra::IO;
using namespace Intra::Range;
using namespace Intra::Algo;

template<typename T> void PrintPolymorphicRange(InputRange<T> range)
{
	Console.Print("[");
	bool firstIteration = true;
	while(!range.Empty())
	{
		if(!firstIteration) Console.Print(", ");
		else firstIteration = false;
		Console.Print(range.First());
		range.PopFirst();
	}
	Console.PrintLine("]");
}

int SumPolymorphicRange(InputRange<int> ints)
{
	int sum = 0;
	while(!ints.Empty())
		sum += ints.GetNext();
	return sum;
}


struct ivec3
{
	int x, y, z;
	INTRA_ADD_REFLECTION(ivec3, x, y, z);
};

void TestSumRange()
{
	int ints[] = {3, 53, 63, 3, 321, 34253, 35434, 2};
	int sum = SumPolymorphicRange(ints);
	Console.PrintLine("sum of ", ints, " = ", sum);

	InputRange<const char> myRange = StringView("Диапазон");
	String myRange2Str = "Супер Диапазон";
	//myRange = myRange2Str();
	char c[40];
	auto r = ArrayRange<char>(c);
	r << Meta::Move(myRange);

	ivec3 vectors[] = {{1, 2, 3}, {1, 64, 7}, {43, 5, 342}, {5, 45, 4}};
	RandomAccessRange<ivec3&> vectors1;
	vectors1 = vectors;
	vectors1[1] = {2, 3, 4};
	InputRange<int> xvectors = Map(vectors, [](const ivec3& v) {return v.x;});
	int xsum = SumPolymorphicRange(Meta::Move(xvectors));
	Console.PrintLine("x sum of ", vectors, " = ", xsum);
}

void TestComposedPolymorphicRange()
{
	auto someRecurrence = Take(Drop(Cycle(Take(Recurrence(
		[](int a, int b) {return a*2+b; }, 1, 1

	), 17)), 3), 22);
	Console.PrintLine("Представляем сложную последовательность в виде полиморфного input-диапазона:");
	InputRange<int> someRecurrencePolymorphic = someRecurrence;
	PrintPolymorphicRange(Meta::Move(someRecurrencePolymorphic));

	Console.PrintLine("Полиморфный диапазон seq содержит генератор 100 случайных чисел от 0 до 999 с отбором квадратов тех из них, которые делятся на 7: ");
	InputRange<uint> seq = Map(
		Filter(
			Take(Generate([]() {return Math::Random<uint>::Global(1000); }), 500),
			[](uint x) {return x%7==0; }),
		Math::Sqr<uint>);
	PrintPolymorphicRange(Meta::Move(seq));

	Console.PrintLine(endl, "Присвоили той же переменной seq диапазон другого типа и выведем его снова:");
	seq = Take(Generate(rand), 50);
	PrintPolymorphicRange(Meta::Move(seq));
}

void RunPolymorphicRangeTests()
{
	TestSumRange();
	TestComposedPolymorphicRange();
}


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


#include "Platform/Time.h"
#include "Test/PerformanceTest.h"

void RunPolymorphicRangePerfTests(IO::Logger& logger)
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

	PrintPerformanceResults(logger, "CycledRange 100000000 раз",
		{"CycledRange*", "InputRange<int>", "InputRange<int>::GetNext", "manually inlined loop", "ArrayRange.Cycle"},
		{time1, time2, time3},
		{time4, time5});
}
