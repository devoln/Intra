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


void TestComposedRange(IO::IFormattedWriter& output)
{
	auto latinAlphabet = Iota('A', char('Z'+1), 1);
	INTRA_ASSERT1(Algo::Equals(latinAlphabet, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"), latinAlphabet);
	output.PrintLine("Выведем английский алфавит: ", latinAlphabet, endl);

	//Бесконечная последовательность Фибоначчи вместе с диапазоном
	auto fib = Recurrence(Op::Add<int>, 1, 1);
	INTRA_ASSERT1(Algo::StartsWith(fib, ArrayRange<const int>{1, 1, 2, 3, 5, 8, 13, 21}), Take(fib, 8));

	Array<int> fibArr;
	fibArr.SetCountUninitialized(15);

	//Копируем на их место 15 элементов из последовательности fib
	Algo::CopyTo(fib, 15, fibArr);
	output.PrintLine(endl, "Последовательность Фибоначчи в массиве: ");
	INTRA_ASSERT_EQUALS(fibArr, Take(fib, 15));
	output.PrintLine(fibArr);
	output.PrintLine(endl, "Вторая половина того же массива задом наперёд: ");
	INTRA_ASSERT1(Algo::Equals(Retro(Drop(fibArr, fibArr.Count()/2)), ArrayRange<const int>({610, 377, 233, 144, 89, 55, 34, 21})), Retro(Drop(fibArr, fibArr.Count()/2)));
	output.PrintLine(Retro(Drop(fibArr, fibArr.Count()/2)));

	//Вставляем в массив 15 чисел Фибонначчи, начиная с 6-го
	Algo::CopyTo(Take(Drop(fib, 5), 15), LastAppender(fibArr));
	output.PrintLine(endl, "Добавляем 15 чисел Фибоначчи, начиная с шестого, в конец. Новое содержимое массива: ");
	output.PrintLine(fibArr);

	StringView strs0[] = {"hello", "world"};
	StringView strs1[] = {"range", "testing", "program"};
	StringView strs2[] = {"C++", "is cool"};

	output.PrintLine("This test uses the folowing string arrays:");
	output.PrintLine("strs0 = ", strs0);
	output.PrintLine("strs1 = ", strs1);
	output.PrintLine("strs2 = ", strs2);
	output.PrintLine();
	auto chain = Chain(strs0, strs1, strs2);

	auto someRecurrence = Take(Drop(Cycle(Take(Recurrence(
		[](int a, int b) {return a*2+b;}, 1, 1
	), 17)), 3), 22);

	output.PrintLine(endl, "Объединяем элементы различных диапазонов в диапазоне кортежей:");
	auto megaZip = Zip(
		Take(fib, 30),
		Retro(Stride(Take(chain, 40), 2)),
		someRecurrence,
		Stride(Take(Drop(Cycle(Take(fib, 19)), 5), 50), 3),
		Take(Recurrence(Op::Mul<ulong64>, 2ull, 3ull), 9)
	);

	output.PrintLine(ToString(megaZip, ",\n  ", "[\n  ", "\n]"));

	output.PrintLine("4-й элемент цепочки массивов: ", chain[4]);
	output.PrintLine("Первые 20 элементов зацикленной цепочки массивов: ", endl, Take(Cycle(chain), 20));

	output.PrintLine(endl, "Поменяем сразу три массива одним вызовом FillPattern для цепочки:");
	static const StringView pattern[] = {"pattern", "fills", "range"};
	Algo::FillPattern(chain, pattern);
	output.PrintLine(strs0, endl, strs1, endl, strs2, endl);

	output.PrintLine("11-й элемент зацикленного массива строк: ", endl, Tail(Take(Cycle(strs0), 11), 1), endl);
	output.PrintLine("Перевёрнутый массив строк: ", endl, Retro(strs0), endl);
	output.PrintLine("Зациклили первые два элемента массива и взяли 10 из них:");
	output.PrintLine(Take(Cycle(Take(strs1, 2)), 10));
	output.PrintLine("Между массивом строк и 5 числами Фибоначчи выбрали второе в рантайме: ");
	output.PrintLine(Choose(
		strs1,
		Map(Take(fib, 5), ToString<int>),
		true) );
	

	//Выводим чередующиеся элементы из четырёх разных диапазонов
	static const size_t indices[] = {1,1,1,2,2,0,2,1,0};
	output.PrintLine(
		RoundRobin(
			Indexed(strs1, indices),
			Repeat("Test", 5),
			AsRange(strs1),
			AsRange(strs2)
		)
	);

	StringView helloWorldCode = "int main()\n{\n\tprintf(\"HelloWorld!\");\n}";
	output.PrintLine("Разобьём программу helloworld на токены. Её код:");
	output.PrintLine(helloWorldCode);
	auto tokens = Split(helloWorldCode, AsciiSet::Spaces, AsciiSet("(){},;"));
	output.PrintLine("Токены: ", ToString(tokens, "\", \"", "[\"", "\"]"));

	int arr[]={1, 4, 11, 6, 8};
	output.PrintLine("max of ", arr, " = ", Algo::Reduce(arr, Op::Max<int>));

	output.PrintLine(endl, "Код в 4 строки, эквивалентный примеру из http://ru.cppreference.com/w/cpp/algorithm/copy:");
	Array<int> fromVector = Iota(10);
	Array<int> toVector = Repeat(0, 10);
	Algo::CopyTo(fromVector, toVector);
	output.PrintLine("toVector содержит: ", toVector);
}
