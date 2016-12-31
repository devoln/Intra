#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include "PerfTestRanges.h"
#include "IO/Stream.h"
#include "Algo/String/ToString.h"
#include "Algo/String/Parse.h"
#include "Algo/Reduction.h"
#include "Range/ArrayRange.h"
#include "Range/Construction.h"
#include "Range/Iteration.h"
#include "Range/Operations.h"
#include "Math/MathRanges.h"
#include "Math/Random.h"
#include "Range/Polymorphic.h"

using namespace Intra;
using namespace Intra::IO;
using namespace Intra::Range;

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

struct ivec3
{
	int x, y, z;
	INTRA_ADD_REFLECTION(ivec3, x, y, z);
};

int SumPolymorphicRange(InputRange<int> ints)
{
	int sum=0;
	while(!ints.Empty())
		sum += ints.GetNext();
	return sum;
}

void TestSumRange()
{
	int ints[] = {3, 53, 63, 3, 321, 34253, 35434, 2};
	int sum = SumPolymorphicRange(ints);
	Console.PrintLine("sum of ", ints, " = ", sum);

	ivec3 vectors[] = {{1, 2, 3}, {1, 64, 7}, {43, 5, 342}, {5, 45, 4}};
	//static_assert(Range::RD::AsRangeCompiles<ivec3(&)[4]>::_, "ERROR!");
	int xsum = SumPolymorphicRange(Map(vectors, [](const ivec3& v){return v.x;}));
	static_assert(Meta::HasForEachField<ivec3>::_, "ERROR!");
	Console.PrintLine("x sum of ", vectors, " = ", xsum);
}

void RunRangeTests()
{
	Console.PrintLine("Можно работать со строками прямо в буфере на стеке:");
	char bufOnStack[100];
	ArrayRange<char> buf = ArrayRange<char>(bufOnStack);
	Algo::ToString(buf, 1.0f+Algo::ParseAdvance<float>(StringView("2.1415926")));
	Console.PrintLine(StringView(bufOnStack, buf.Begin), endl);

	TestSumRange();

	Console.PrintLine(endl, "Есть нормальная поддержка Юникода в консоли, даже на винде.");
	Console.PrintLine("Тестируется текст с кириллицей, греческим алфавитом αβγδεζηθικλμνξο, а также с иероглифами ㈇㌤㈳㌛㉨.");
	Console.PrintLine("Иероглифы не отображаются в консоли, потому что консольный шрифт их не содержит.");

	String alphabet = Iota('A', char('Z'+1), 1);
	Console.PrintLine("Выведем английский алфавит: ", alphabet, endl);

	StringView strs[] = {"hello", "world"};
	StringView strs1[]  = {"range", "testing", "program"};
	StringView strs2[] = {"C++", "крут"};

	Console.PrintLine("В тесте используются три массива:", endl, strs, endl, strs1, endl, strs2);
	Console.PrintLine(endl, "Пример вывода initializer list:");
	Console.PrintLine(AsRange<double>({4353.435, 3243.23, 21.421, 12355.5, 64532}));

	//Бесконечная последовательность Фибоначчи вместе с диапазоном
	auto fib = Recurrence(Op::Add<int>, 1, 1);

	//Создаём массив из 15 неинициализированных элементов
	Array<int> fibArr;
	fibArr.SetCountUninitialized(15);

	//Копируем на их место 15 элементов из последовательности fib
	Algo::CopyTo(Take(fib, 15), fibArr());
	Console.PrintLine(endl, "Последовательность Фибоначчи в массиве: ", endl, fibArr);
	Console.PrintLine(endl, "Вторая половина того же массива задом наперёд: ", endl, Retro(fibArr($/2, $)));

	//Вставляем в массив 15 чисел Фибонначчи, начиная с 6-го
	Algo::CopyTo(Take(Drop(fib, 5), 15), fibArr.Insert($));
	Console.PrintLine(endl, "Добавляем 15 чисел Фибоначчи, начиная с пятого, в конец. Новое содержимое массива: ");
	Console.PrintLine(fibArr());

	auto chain = Chain(AsRange(strs), AsRange(strs1), AsRange(strs2));
	auto someRecurrence = Take(Drop(Cycle(Take(Recurrence(
		[](int a, int b) {return a*2+b;}, 1, 1
	), 17)), 3), 22);
	auto megaZip = Zip(
		Take(fib, 30),
		Retro(Stride(Take(chain, 40), 2)),
		someRecurrence,
		Stride(Take(Drop(Cycle(Take(fib, 19)), 5), 50), 3),
		Take(Recurrence(Op::Mul<ulong64>, 2ull, 3ull), 9)
	);

	Console.PrintLine(endl, "Полиморфные диапазоны:");
	PrintPolymorphicRange<int>(someRecurrence);
	PrintPolymorphicRange<StringView>(Take(Cycle(chain), 100));
	PrintPolymorphicRange<String>(Map(someRecurrence, [](int x){return ToString(x);}));
	PrintPolymorphicRange<StringView>(strs1);

	Console.PrintLine(endl, "Объединяем элементы различных диапазонов в диапазоне кортежей: ", endl,
		ToString(
			megaZip,
			",\n  ", "[\n  ", "\n]"
		)
	);

	Console.PrintLine("4-й элемент цепочки массивов: ", chain[4]);
	Console.PrintLine("Первые 20 элементов зацикленной цепочки массивов: ", endl, Take(Cycle(chain), 20));

	Console.PrintLine(endl, "Поменяем сразу три массива одним вызовом FillPattern для цепочки:");
	static const StringView pattern[] = {"pattern", "fills", "range"};
	Algo::FillPattern(chain, pattern);
	Console.PrintLine(strs, endl, strs1, endl, strs2, endl);

	Console.PrintLine("11-й элемент зацикленного массива строк: ", endl,
		Tail(Take(Cycle(strs), 11), 1), endl);
	Console.PrintLine("Перевёрнутый массив строк: ", endl, Retro(strs), endl);
	Console.PrintLine("Зациклили первые два элемента массива и взяли 10 из них:");
	Console.PrintLine(Take(Cycle(Take(strs1, 2)), 10));
	Console.PrintLine("Между массивом строк и 5 числами Фибоначчи выбрали второе в рантайме: ");
	Console.PrintLine(Choose(
		Map(AsRange(strs1), [](StringView str) {return String(str);}),
		Map(Take(fib, 5), [](int x){return ToString(x);}),
		true) );
	

	//Выводим чередующиеся элементы из четырёх разных диапазонов
	static const size_t indices[] = {1,1,1,2,2,0,2,1,0};
	Console.PrintLine(
		RoundRobin(
			Indexed(strs1, indices),
			Repeat("Test", 5),
			AsRange(strs1),
			AsRange(strs2)
		)
	);
	

	/*Console.PrintLine(endl, "Введите строки, которые войдут в диапазон строк. В конце введите \"end\".");

	Console.PrintLine("Вы ввели следующие строки:", endl);
	PrintPolymorphicRange<String>(Console.ByLine("end"));*/

	int arr[]={1, 4, 11, 6, 8};
	Console.PrintLine("max of ", arr, " = ", Algo::Reduce(arr, Op::Max<int>));
	Console.PrintLine("Генерация 100 случайных чисел от 0 до 999 и вывод квадратов тех из них, которые делятся на 7: ");

	Console.PrintLine("Random value: ", Math::Random<uint>::Global(1000));
	FiniteInputRange<uint> seq = Map(
		Filter(
			Take(Generate([](){return Math::Random<uint>::Global(1000);}), 500),
			[](uint x) {return x%7==0;}),
		Math::Sqr<uint>);
	PrintPolymorphicRange(Meta::Move(seq));

	Console.PrintLine(endl, "Присвоили той же переменной диапазон другого типа:");
	
	seq = Take(Generate([](){return Math::Random<uint>::Global(1000);}), 50);
	PrintPolymorphicRange(Meta::Move(seq));
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

void RunRangePerfTests(IO::Logger& logger)
{
	Array<int> arr;
	arr.SetCountUninitialized(1000);
	IRange* range = new CycledRange(arr.Data(), arr.Count());

	Timer tim;
	int sum1 = TestPolymorphicRange(range, 100000000);
	double time1 = tim.GetTimeAndReset();

	int sum2 = TestPolymorphicRange2(Range::Cycle(arr()), 100000000);
	double time2 = tim.GetTimeAndReset();

	int sum3 = TestPolymorphicRange3(Range::Cycle(arr()), 100000000);
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

