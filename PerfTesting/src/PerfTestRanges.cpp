#include "PerfTestRanges.h"
#include "IO/Stream.h"
#include "Algorithms/Range.h"
#include "Algorithms/RangeConstruct.h"
#include "Algorithms/RangeIteration.h"
#include "Math/MathRanges.h"
#include "Math/Random.h"

using namespace Intra;
using namespace Intra::IO;

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

	auto fib = Math::Recurrence(Op::Add<int>, 1, 1);

	Array<int> fibArr;
	fibArr.SetCountUninitialized(15);
	fib.Take(15).CopyTo(fibArr);
	Console.PrintLine(endl, "Последовательность Фибоначчи в массиве: ", endl, fibArr);
	Console.PrintLine(endl, "Вторая половина того же массива задом наперёд: ", endl, fibArr($/2, $).Retro());

	fib.Drop(5).Take(15).CopyTo(fibArr.Insert($));
	Console.PrintLine(endl, "Добавляем 15 чисел Фибоначчи, начиная с пятого, в конец. Новое содержимое массива: ");
	Console.PrintLine(fibArr());

	auto chain = Chain(AsRange(strs), AsRange(strs1), AsRange(strs2)).Take(50);
	auto someRecurrence = Math::Recurrence([](int a, int b){return a*2+b;}, 1, 1).Take(17).Cycle().Drop(3).Take(22);
	auto megaZip = Zip(
				fib.Take(30),
				chain.Take(40).Stride(2).Retro(),
				someRecurrence,
				fib.Take(19).Cycle().Drop(5).Take(50).Stride(3),
				Math::Recurrence(Op::Mul<ulong64>, 2ull, 3ull).Take(9)
			);

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
	

	Console.PrintLine(endl, "Введите строки, которые войдут в диапазон строк. В конце введите \"end\".");

	Console.PrintLine("Вы ввели следующие строки:", endl, "[", String::Join(Console.ByLine("end"), ", ", "\"", "\""), "]");

	int arr[]={1, 4, 11, 6, 8};
	Console.PrintLine("max of ", arr, " = ", AsRange(arr).Reduce(Op::Max<int>));
	Console.PrintLine("Генерация 100 случайных чисел от 0 до 999 и вывод квадратов тех из них, которые делятся на 7: ");

	auto seq = Range::Generate([](){return Math::Random<uint>::Global(1000);}).Take(500)
		.Filter([](uint x) {return x%7==0;})
		.Map(Math::Sqr<uint>);
	Console.PrintLine("[", String::Join(seq, ", "), "]");
}



void RunRangePerfTests(IO::Logger& logger)
{
	(void)logger;
}

