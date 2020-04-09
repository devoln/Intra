#include "Intra/Container/AsciiSet.h"
#include "Intra/Range/Chain.h"
#include "Intra/Range/Comparison.h"
#include "Intra/Range/Cycle.h"
#include "Intra/Range/Iota.h"
#include "Intra/Range/Map.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Recurrence.h"
#include "Intra/Range/Reduce.h"
#include "Intra/Range/RoundRobin.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Split.h"
#include "Intra/Range/Stream/ToString.h"
#include "Intra/Range/Stride.h"
#include "Extra/Container/Sequential/List.h"
#include "Extra/Unstable/Random/FastUniform.h"


INTRA_BEGIN

INTRA_MODULE_UNITTEST
{
	auto latinAlphabet = Iota('A', char('Z'+1), 1);
	INTRA_ASSERT(Equals(latinAlphabet, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
	INTRA_ASSERT(StringOf(latinAlphabet) == "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

	auto fib = Recurrence(FAdd, 1, 1);
	INTRA_ASSERT1(StartsWith(fib, CSpan<int>{1, 1, 2, 3, 5, 8, 13, 21, 34}));

	Array<int> arr = {1, 2, 3, 4, 5, 6, 7, 8};
	CopyTo(fib, arr);
	INTRA_ASSERT(Equals(
		arr,
		CSpan<int>{1, 1, 2, 3, 5, 8, 13, 21}
	));

	Span<int> arrSecondHalf = Drop(fibArr, fibArr.Length() / 2);
	INTRA_ASSERT(Equals(arrSecondHalf, CSpan<int>{5, 8, 13, 21}));
	auto reversedArrSecondHalf = Retro(arrSecondHalf);
	INTRA_ASSERT(Equals(reversedArrSecondHalf, CSpan<int>{21, 13, 8, 5}));

	CopyTo(Take(Drop(fib, 5), 7), LastAppender(arr));
	INTRA_ASSERT(Equals(
		arr,
		CSpan<int>{1, 1, 2, 3, 5, 8, 13, 21, 8, 13, 21, 34, 55, 89, 144}
	));

	auto someRecurrencePart = Take(Recurrence([](int a, int b) {return a*2+b;}, 1, 1), 6);
	INTRA_ASSERT(Equals(
		someRecurrencePart,
		CSpan<int>{1, 1, 3, 5, 11, 21}
	));
	auto shiftedCycledRecurrencePart = Take(Drop(Cycle(someRecurrencePart), 9), 7);
	INTRA_ASSERT(Equals(
		complexRecurrencePart,
		CSpan<int>{5, 11, 21, 1, 1, 3, 5}
	));

	StringView strs0[] = {"hello", "world"};
	INTRA_ASSERT(Equals(
		Retro(strs0),
		CSpan<StringView>{"world", "hello"}
	));

	StringView strs1[] = {"range", "testing", "program"};
	StringView strs2[] = {"C++", "is cool"};
	auto chain = Chain(strs0, strs1, strs2);
	INTRA_ASSERT(Equals(
		chain,
		CSpan<StringView>{"hello", "world", "range", "testing", "program", "C++", "is cool"}
	));

	INTRA_ASSERT(Equals(
		chain,
		CSpan<StringView>{
			"hello", "world", "range", "testing", "program", "C++", "is cool",
			"hello", "world", "range", "testing", "program", "C++", "is cool",
			"hello", "world", "range", "testing", "program", "C++"
		}
	));

	auto megaZip = Zip(
		Take(fib, 30),
		Retro(Stride(Take(chain, 40), 2)),
		someRecurrence,
		Stride(Take(Drop(Cycle(Take(fib, 19)), 5), 50), 3),
		Take(Recurrence(FMul, 2ull, 3ull), 9)
	);
	Std.PrintLine(StringOf(megaZip, ",\n  ", "[\n  ", "\n]")); //TODO: replace with assert

	static const StringView pattern[] = {"pattern", "fills", "range"};
	FillPattern(chain, pattern);
	INTRA_ASSERT(Equals(
		chain,
		CSpan<StringView>{"pattern", "fills", "range", "pattern", "fills", "range", "pattern"}
	));
	INTRA_ASSERT(Equals(
		strs1,
		CSpan<StringView>{"range", "pattern", "fills"}
	));

	INTRA_ASSERT(Equals(
		Tail(Take(Cycle(strs1), 11), 2),
		CSpan<StringView>{"range", "pattern"}
	));
	

	static const index_t indices[] = {1,1,1,2,2,0,2,1,0};
	auto indexedStrs1 = Map(indices, Bind(FIndex, CSpanOf(strs1)));
	INTRA_ASSERT(Equals(
		indexedStrs1,
		CSpan<StringView>{"pattern", "pattern", "pattern", "fills", "fills", "range", "fills", "pattern", "range"}
	));

	INTRA_ASSERT(Equals(
		Repeat("Test", 5),
		CSpan<StringView>{"Test", "Test", "Test", "Test", "Test"}
	));

	auto roundRobin = RoundRobin(
		indexedStrs1,
		Repeat("Test", 5),
		SpanOf(strs1),
		SpanOf(strs2)
	);
	INTRA_ASSERT(Equals(
		roundRobin,
		CSpan<StringView>{
			"pattern", "Test", "range", "range", //[0]
			"pattern", "Test", "pattern", "pattern", //[1]
			"pattern", "Test", "fills", //[2]
			"fills", "Test", //[3]
			"fills", "Test", //[4]
			"range", //[5]
			"fills", //[6]
			"pattern", //[7]
			"range", //[8]
		}
	));

	StringView helloWorldCode = "int main()\n{\n\tprintf(\"HelloWorld!\");\n}";
	auto helloWorldTokens = Split(helloWorldCode, IsSpace, AsciiSet("(){},;"));
	INTRA_ASSERT(Equals(
		helloWorldTokens,
		CSpan<StringView>{"int", "main", "(", ")", "{", "printf", "(", "\"HelloWorld!\"", ")", ";", "}"}
	));

	int arr[] = {1, 4, 11, 6, 8};
	INTRA_ASSERT(Reduce(arr, FMax) == 11);

	//An example equivalent to an example of STL: http://en.cppreference.com/w/cpp/algorithm/copy
	Array<int> fromVector = Iota(10);
	Array<int> toVector;
	CopyTo(fromVector, LastAppender(toVector));
	INTRA_ASSERT(Equals(fromVector, toVector));
	INTRA_ASSERT(Equals(
		toVector,
		CSpan<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
	));
}

INTRA_END
