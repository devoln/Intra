#include "PerfTestRanges.h"
#include "IO/Stream.h"
#include "Range/ArrayRange.h"
#include "Range/Construction/Construction.h"
#include "Range/Iteration/Iteration.h"
#include "Math/MathRanges.h"
#include "Math/Random.h"
#include "Range/Polymorphic.h"

using namespace Intra;
using namespace Intra::IO;

template<typename T> void PrintPolymorphicRange(FiniteInputRange<T> range)
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

void RunRangeTests()
{
	char bufOnStack[100];
	ArrayRange<char> buf = ArrayRange<char>(bufOnStack);
	buf.AppendAdvance(StringView("2.1415926").ParseAdvance<float>()+1.0f);
	Console.PrintLine(StringView(bufOnStack, buf.Begin));

	Console.PrintLine("Есть нормальная поддержка Юникода в консоли, даже на винде.");
	Console.PrintLine("Тестируется текст с кириллицей, греческим алфавитом αβγδεζηθικλμνξο, а также с иероглифами ㈇㌤㈳㌛㉨.");
	Console.PrintLine("Иероглифы не отображаются в консоли, потому что консольный шрифт их не содержит.");

 	StringView strs[] = {"hello", "world"};
	StringView strs1[]  = {"range", "testing", "program"};
	StringView strs2[] = {"C++", "крут"};

	Console.PrintLine("В тесте используются три массива:", endl, strs, endl, strs1, endl, strs2);
	Console.PrintLine(endl, "Пример вывода initializer list:");
	Console.PrintLine(AsRange<double>({4353.435, 3243.23, 21.421, 12355.5, 64532}));

	auto fib = Range::Recurrence(Op::Add<int>, 1, 1);

	Array<int> fibArr;
	fibArr.SetCountUninitialized(15);
	fib.Take(15).CopyTo(fibArr);
	Console.PrintLine(endl, "Последовательность Фибоначчи в массиве: ", endl, fibArr);
	Console.PrintLine(endl, "Вторая половина того же массива задом наперёд: ", endl, fibArr($/2, $).Retro());

	fib.Drop(5).Take(15).CopyTo(fibArr.Insert($));
	Console.PrintLine(endl, "Добавляем 15 чисел Фибоначчи, начиная с пятого, в конец. Новое содержимое массива: ");
	Console.PrintLine(fibArr());

	auto chain = Chain(AsRange(strs), AsRange(strs1), AsRange(strs2)).Take(50);
	auto someRecurrence = Range::Recurrence([](int a, int b){return a*2+b;}, 1, 1).Take(17).Cycle().Drop(3).Take(22);
	auto megaZip = Zip(
				fib.Take(30),
				chain.Take(40).Stride(2).Retro(),
				someRecurrence,
				fib.Take(19).Cycle().Drop(5).Take(50).Stride(3),
				Range::Recurrence(Op::Mul<ulong64>, 2ull, 3ull).Take(9)
			);

	Console.PrintLine(endl, "Полиморфные диапазоны:");
	PrintPolymorphicRange<int>(someRecurrence);
	PrintPolymorphicRange<StringView>(chain.Cycle().Take(100));
	PrintPolymorphicRange<String>(someRecurrence.Map([](int x){return ToString(x);}));
	PrintPolymorphicRange<StringView>(strs1);

	Console.PrintLine(endl, "Объединяем элементы различных диапазонов в диапазоне кортежей: ", endl,
		ToString(
			megaZip,
			",\n  ", "[\n  ", "\n]"
		)
	);

	Console.PrintLine("4-й элемент цепочки массивов: ", chain[4]);
	Console.PrintLine("Первые 20 элементов зацикленной цепочки массивов: ", endl, chain.Cycle().Take(20));

	Console.PrintLine(endl, "Поменяем сразу три массива одним вызовом FillPattern для цепочки:");
	static const StringView pattern[] = {"pattern", "fills", "range"};
	chain.FillPattern(AsRange(pattern));
	Console.PrintLine(strs, endl, strs1, endl, strs2, endl);

	Console.PrintLine("11-й элемент зацикленного массива строк: ", endl,
		AsRange(strs).Cycle().Take(11).Tail(1), endl);
	Console.PrintLine("Перевёрнутый массив строк: ", endl, AsRange(strs).Retro(), endl);
	Console.PrintLine("Зациклили первые два элемента массива и взяли 10 из них:");
	Console.PrintLine(AsRange(strs1).Take(2).Cycle().Take(10) );
	Console.PrintLine("Между массивом строк и 5 числами Фибоначчи выбрали второе в рантайме: ");
	Console.PrintLine(Choose(
		AsRange(strs1).Map([](StringView str) {return String(str);}),
		fib.Take(5).Map([](int x){return ToString(x);}),
		true) );

	static const size_t indices[] = {1,1,1,2,2,0,2,1,0};
	Console.PrintLine(
			RoundRobin(
				AsRange(strs1).Indexed(AsRange(indices)),
				Repeat(StringView("Test"), 5),
				AsRange(strs1),
				AsRange(strs2)
			));
	

	/*Console.PrintLine(endl, "Введите строки, которые войдут в диапазон строк. В конце введите \"end\".");

	Console.PrintLine("Вы ввели следующие строки:", endl);
	PrintPolymorphicRange<String>(Console.ByLine("end"));*/

	int arr[]={1, 4, 11, 6, 8};
	Console.PrintLine("max of ", arr, " = ", AsRange(arr).Reduce(Op::Max<int>));
	Console.PrintLine("Генерация 100 случайных чисел от 0 до 999 и вывод квадратов тех из них, которые делятся на 7: ");

	FiniteInputRange<uint> seq = Range::Generate([](){return Math::Random<uint>::Global(1000);}).Take(500)
		.Filter([](uint x) {return x%7==0;})
		.Map(Math::Sqr<uint>);
	PrintPolymorphicRange(core::move(seq));

	Console.PrintLine(endl, "Присвоили той же переменной диапазон другого типа:");
	
	seq = Range::Generate([](){return Math::Random<uint>::Global(1000);}).Take(50);
	PrintPolymorphicRange(core::move(seq));
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
	auto cycle = ArrayRange<int>(arr, count).Cycle();
	int sum = 0;
	for(size_t i=0; i<totalCount; i++)
	{
		sum += cycle.First();
		cycle.PopFirst();
	}
	return sum;
}


#include "Core/Time.h"
#include "Test/PerformanceTest.h"

void RunRangePerfTests(IO::Logger& logger)
{
	Array<int> arr;
	arr.SetCountUninitialized(1000);
	IRange* range = new CycledRange(arr.Data(), arr.Count());

	Timer tim;
	int sum1 = TestPolymorphicRange(range, 100000000);
	double time1 = tim.GetTimeAndReset();

	int sum2 = TestPolymorphicRange2(arr.AsRange().Cycle(), 100000000);
	double time2 = tim.GetTimeAndReset();

	int sum3 = TestPolymorphicRange3(arr.AsRange().Cycle(), 100000000);
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

