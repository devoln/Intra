#include "Core/Core.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include "Range.h"
#include "Core/Range/Reduce.h"
#include "Core/Range/Span.h"
#include "Core/Range/Chain.h"
#include "Core/Range/Iota.h"
#include "Core/Range/Cycle.h"
#include "Core/Range/Stride.h"
#include "Core/Range/Map.h"
#include "Core/Range/Recurrence.h"
#include "Core/Range/RoundRobin.h"
#include "Core/Range/Split.h"
#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Comparison.h"
#include "Core/Range/Stream/ToString.h"
#include "Math/Random/FastUniform.h"
#include "Container/Sequential/List.h"
#include "Utils/AsciiSet.h"

#include <stdlib.h>

using namespace Intra;


void TestComposedRange(FormattedWriter& output)
{
	auto latinAlphabet = Iota('A', char('Z'+1), 1);
	INTRA_ASSERT1(Equals(latinAlphabet, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"), latinAlphabet);
	output.PrintLine("Printing English alphabet: ", latinAlphabet);
	output.LineBreak();

	//Infinite Fibonacci sequence
	auto fib = Recurrence(FAdd, 1, 1);
	INTRA_ASSERT1(StartsWith(fib, CSpan<int>{1, 1, 2, 3, 5, 8, 13, 21}), Take(fib, 8));

	Array<int> fibArr;
	fibArr.SetCountUninitialized(15);

	//Replace 15 unitialized array elements with fib elements
	CopyTo(fib, fibArr);
	output.LineBreak();
	output.PrintLine("Fibonacci numbers in the array: ");
	INTRA_ASSERT_EQUALS(fibArr, Take(fib, 15));
	output.PrintLine(fibArr);
	output.LineBreak();
	output.PrintLine("Вторая половина того же массива задом наперёд: ");
	INTRA_ASSERT1(Equals(
		Retro(Drop(fibArr, fibArr.Count()/2)),
		CSpan<int>({610, 377, 233, 144, 89, 55, 34, 21})
	), Retro(Drop(fibArr, fibArr.Count()/2)));
	output.PrintLine(Retro(Drop(fibArr, fibArr.Count()/2)));

	//Вставляем в массив 15 чисел Фибонначчи, начиная с 6-го
	CopyTo(Take(Drop(fib, 5), 15), LastAppender(fibArr));
	output.LineBreak();
	output.PrintLine("Добавляем 15 чисел Фибоначчи, начиная с шестого, в конец. Новое содержимое массива: ");
	output.PrintLine(fibArr);

	StringView strs0[] = {"hello", "world"};
	StringView strs1[] = {"range", "testing", "program"};
	StringView strs2[] = {"C++", "is cool"};

	output.PrintLine("This test uses the folowing string arrays:");
	output.PrintLine("strs0 = ", strs0);
	output.PrintLine("strs1 = ", strs1);
	output.PrintLine("strs2 = ", strs2);
	output.LineBreak();
	auto chain = Chain(strs0, strs1, strs2);

	auto someRecurrence = Take(Drop(Cycle(Take(Recurrence(
		[](int a, int b) {return a*2+b;}, 1, 1
	), 17)), 3), 22);

	output.LineBreak();
	output.PrintLine("Объединяем элементы различных диапазонов в диапазоне кортежей:");
	auto megaZip = Zip(
		Take(fib, 30),
		Retro(Stride(Take(chain, 40), 2)),
		someRecurrence,
		Stride(Take(Drop(Cycle(Take(fib, 19)), 5), 50), 3),
		Take(Recurrence(FMul, 2ull, 3ull), 9)
	);

	output.PrintLine(StringOf(megaZip, ",\n  ", "[\n  ", "\n]"));

	output.PrintLine("4-й элемент цепочки массивов: ", chain[4]);
	output.PrintLine("Первые 20 элементов зацикленной цепочки массивов: ");
	output.PrintLine(Take(Cycle(chain), 20));

	output.LineBreak();
	output.PrintLine("Поменяем сразу три массива одним вызовом FillPattern для цепочки:");
	static const StringView pattern[] = {"pattern", "fills", "range"};
	FillPattern(chain, pattern);
	output.PrintLine("strs0 = ", strs0);
	output.PrintLine("strs1 = ", strs1);
	output.PrintLine("strs2 = ", strs2);
	output.LineBreak();

	output.PrintLine("11-й элемент зацикленного массива строк: ");
	output.PrintLine(Tail(Take(Cycle(strs0), 11), 1));
	output.LineBreak();

	output.PrintLine("Перевёрнутый массив строк: ");
	output.PrintLine(Retro(strs0));
	output.LineBreak();

	output.PrintLine("Зациклили первые два элемента массива и взяли 10 из них:");
	output.PrintLine(Take(Cycle(Take(strs1, 2)), 10));
#if INTRA_DISABLED
	output.PrintLine("Между массивом строк и 5 числами Фибоначчи выбрали второе в рантайме: ");
	output.PrintLine(Choose(
		strs1,
		Map(Take(fib, 5), StringOf<int>),
		true) );
#endif
	output.LineBreak();
	

	//Выводим чередующиеся элементы из четырёх разных диапазонов
	static const index_t indices[] = {1,1,1,2,2,0,2,1,0};
	output.PrintLine(
		RoundRobin(
			Map(indices, Bind(FIndex, CSpanOf(strs1))),
			Repeat("Test", 5),
			SpanOf(strs1),
			SpanOf(strs2)
		)
	);

	StringView helloWorldCode = "int main()\n{\n\tprintf(\"HelloWorld!\");\n}";
	output.PrintLine("Разобьём программу helloworld на токены. Её код:");
	output.PrintLine(helloWorldCode);
	auto tokens = Split(helloWorldCode, AsciiSets.Spaces, AsciiSet("(){},;"));
	output.PrintLine("Токены: ", StringOf(tokens, "\", \"", "[\"", "\"]"));

	int arr[]={1, 4, 11, 6, 8};
	output.PrintLine("max of ", arr, " = ", Reduce(arr, FMax));

	output.LineBreak();
	output.PrintLine("Код в 4 строки, эквивалентный примеру из "
		"http://ru.cppreference.com/w/cpp/algorithm/copy"":");
	Array<int> fromVector = Iota(10);
	Array<int> toVector = Repeat(0, 10);
	CopyTo(fromVector, toVector);
	output.PrintLine("toVector содержит: ", toVector);
}
