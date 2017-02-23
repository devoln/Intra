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


void RunComposedRangeTests()
{
	Console.PrintLine(endl, "Есть нормальная поддержка Юникода в консоли, даже на винде.");
	Console.PrintLine("Тестируется текст с кириллицей, греческим алфавитом αβγδεζηθικλμνξοπρστυφχψω, а также с иероглифами ㈇㌤㈳㌛㉨.");
	Console.PrintLine("Иероглифы не отображаются в консоли, потому что консольный шрифт их не содержит.");

	Console.PrintLine("Выведем английский алфавит: ", Iota('A', char('Z'+1), 1), endl);

	StringView strs[] = {"hello", "world"};
	StringView strs1[]  = {"range", "testing", "program"};
	StringView strs2[] = {"C++", "крут"};

	Console.PrintLine("В тесте используются три массива:");
	Console.PrintLine(strs);
	Console.PrintLine(strs1, endl, strs2);
	Console.PrintLine(endl, "Пример вывода initializer list:");
	Console.PrintLine(AsRange<double>({4353.435, 3243.23, 21.421, 12355.5, 64532}));

	//Бесконечная последовательность Фибоначчи вместе с диапазоном
	auto fib = Recurrence(Op::Add<int>, 1, 1);

	//Создаём массив из 15 неинициализированных элементов
	Array<int> fibArr;
	fibArr.SetCountUninitialized(15);

	//Копируем на их место 15 элементов из последовательности fib
	Algo::CopyTo(fib, 15, fibArr);
	Console.PrintLine(endl, "Последовательность Фибоначчи в массиве: ", endl, fibArr);
	Console.PrintLine(endl, "Вторая половина того же массива задом наперёд: ", endl, Retro(fibArr($/2, $)));

	//Вставляем в массив 15 чисел Фибонначчи, начиная с 6-го
	Algo::CopyTo(Take(Drop(fib, 5), 15), fibArr.Insert($));
	Console.PrintLine(endl, "Добавляем 15 чисел Фибоначчи, начиная с пятого, в конец. Новое содержимое массива: ");
	Console.PrintLine(fibArr);

	auto chain = Chain(strs, strs1, strs2);

	auto someRecurrence = Take(Drop(Cycle(Take(Recurrence(
		[](int a, int b) {return a*2+b;}, 1, 1
	), 17)), 3), 22);

	Console.PrintLine(endl, "Объединяем элементы различных диапазонов в диапазоне кортежей:");
	auto megaZip = Zip(
		Take(fib, 30),
		Retro(Stride(Take(chain, 40), 2)),
		someRecurrence,
		Stride(Take(Drop(Cycle(Take(fib, 19)), 5), 50), 3),
		Take(Recurrence(Op::Mul<ulong64>, 2ull, 3ull), 9)
	);

	Console.PrintLine(ToString(megaZip, ",\n  ", "[\n  ", "\n]"));

	Console.PrintLine("4-й элемент цепочки массивов: ", chain[4]);
	Console.PrintLine("Первые 20 элементов зацикленной цепочки массивов: ", endl, Take(Cycle(chain), 20));

	Console.PrintLine(endl, "Поменяем сразу три массива одним вызовом FillPattern для цепочки:");
	static const StringView pattern[] = {"pattern", "fills", "range"};
	Algo::FillPattern(chain, pattern);
	Console.PrintLine(strs, endl, strs1, endl, strs2, endl);

	Console.PrintLine("11-й элемент зацикленного массива строк: ", endl, Tail(Take(Cycle(strs), 11), 1), endl);
	Console.PrintLine("Перевёрнутый массив строк: ", endl, Retro(strs), endl);
	Console.PrintLine("Зациклили первые два элемента массива и взяли 10 из них:");
	Console.PrintLine(Take(Cycle(Take(strs1, 2)), 10));
	Console.PrintLine("Между массивом строк и 5 числами Фибоначчи выбрали второе в рантайме: ");
	Console.PrintLine(Choose(
		strs1,
		Map(Take(fib, 5), ToString<int>),
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

	StringView helloWorldCode = "int main()\n{\n\tprintf(\"HelloWorld!\");\n}";
	Console.PrintLine("Разобьём программу helloworld на токены. Её код:");
	Console.PrintLine(helloWorldCode);
	auto tokens = Split(helloWorldCode, AsciiSet::Spaces, AsciiSet("(){},;"));
	Console.PrintLine("Токены: ", ToString(tokens, "\", \"", "[\"", "\"]"));

	int arr[]={1, 4, 11, 6, 8};
	Console.PrintLine("max of ", arr, " = ", Algo::Reduce(arr, Op::Max<int>));

	Console.PrintLine(endl, "Код в 4 строки, эквивалентный примеру из http://ru.cppreference.com/w/cpp/algorithm/copy:");
	Array<int> fromVector = Iota(10);
	Array<int> toVector = Repeat(0, 10);
	Algo::CopyTo(fromVector, toVector);
	Console.PrintLine("toVector содержит: ", toVector);
}
