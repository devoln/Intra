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
#include "Containers/List.h"

using namespace Intra;
using namespace Intra::IO;
using namespace Intra::Range;
using namespace Intra::Algo;


void TestArrayRangeStreams()
{
	Console.PrintLine("Любой диапазон символов есть поток.");
	Console.PrintLine("Можно работать со строками прямо в буфере на стеке как с потоками.");
	char bufOnStack[512];
	Array<int> arrToFormat = {54, 3, 45, 56, 24};
	int rawArr[] = {6, 3, 8, 3};
	ArrayRange<char> buf = bufOnStack;
	buf << "Мы пишем в сишный массив, как в поток." << '\r' << '\n' <<
		"Парсим pi и прибавляем к нему 1: " <<
		1.0f+Algo::Parse<float>("3.1415926") << "\r\n" <<
		"Далее записан массив: " << arrToFormat <<
		"\r\nКвадраты элементов этого массива: " << Map(arrToFormat, Math::Sqr<int>) <<
		"\r\nКвадраты элементов другого массива: " << Map(rawArr, Math::Sqr<int>);
	Console.PrintLine("Результат:");
	Console.PrintLine(StringView(bufOnStack, buf.Begin), endl);

	StringView strPiE = "3.1415926, 2.178281828, 1, 2";
	Console.PrintLine("Распарсим строку \"", strPiE, "\" как поток:");
	float pi, e;
	int i1, i2;
	strPiE >> pi >> "," >> e >> "," >> i1 >> "," >> i2;
	Console.PrintLine("π = ", pi, ", e = ", e, ", i1 = ", i1, ", i2 = ", i2, endl);
}

void TestCustomOutputStream()
{
	struct ConsoleCaesarCipher
	{
		void Put(char ch)
		{
			if(ch>='A' && ch<='Z')
			{
				if(ch=='Z') ch = 'A';
				else ch++;
			}
			if(ch>='a' && ch<='z')
			{
				if(ch=='z') ch = 'a';
				else ch++;
			}
			Console << ch;
		}
	};

	StringView strings[] = {"Hello", ", ", "World", "!"};
	Console.PrintLine("Воспользуемся шифрующим потоком, применив его к строке \"", "Printing array: ", "\" и к массиву ", strings, ":");
	ConsoleCaesarCipher() << "Printing array: " << strings;
	Console.PrintLine();
}

void RunStreamRangeTests()
{
	TestArrayRangeStreams();
	TestCustomOutputStream();
}
