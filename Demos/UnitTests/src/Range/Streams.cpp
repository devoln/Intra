﻿#include "Cpp/Warnings.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include "Range.h"
#include "IO/FormattedWriter.h"
#include "Range/Stream.hh"
#include "Range/Reduction.h"
#include "Utils/Span.h"
#include "Range.hh"
#include "Random/FastUniform.h"
#include "Container/Sequential/List.h"

using namespace Intra;
using namespace IO;
using namespace Range;


void TestArrayRangeStreams(FormattedWriter& output)
{
	output.PrintLine("Любой диапазон символов есть поток.");
	output.PrintLine("Можно работать со строками прямо в буфере на стеке как с потоками.");
	char bufOnStack[512];
	Array<int> arrToFormat = {54, 3, 45, 56, 24};
	int rawArr[] = {6, 3, 8, 3};
	Span<char> buf = bufOnStack;
	buf << "Мы пишем в сишный массив, как в поток." << '\r' << '\n' <<
		"Парсим pi и прибавляем к нему 1: " <<
		1.0f+Range::Parse<float>("3.1415926") << "\r\n" <<
		"Далее записан массив: " << arrToFormat <<
		"\r\nКвадраты элементов этого массива: " << Map(arrToFormat, Math::Sqr<int>) <<
		"\r\nКвадраты элементов другого массива: " << Map(rawArr, Math::Sqr<int>);
	output.PrintLine("Результат:");
	output.PrintLine(StringView(bufOnStack, buf.Begin));
	output.LineBreak();

	StringView strPiE = "3.1415926, 2.178281828, 1, 2";
	output.PrintLine("Распарсим строку \"", strPiE, "\" как поток:");
	float pi, e;
	int i1, i2;
	strPiE >> pi >> "," >> e >> "," >> i1 >> "," >> i2;
	output.PrintLine("π = ", pi, ", e = ", e, ", i1 = ", i1, ", i2 = ", i2);
	output.LineBreak();
}

void TestCustomOutputStream(FormattedWriter& output)
{
	INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
	struct OutputCaesarCipher
	{
		void Put(char ch)
		{
			if(ch >= 'A' && ch <= 'Z')
			{
				if(ch == 'Z') ch = 'A';
				else ch++;
			}
			if(ch >= 'a' && ch <= 'z')
			{
				if(ch == 'z') ch = 'a';
				else ch++;
			}
			Output << ch;
		}
		OutputStream Output;
	};

	StringView strings[] = {"Hello", ", ", "World", "!"};
	output.PrintLine("Воспользуемся шифрующим потоком, применив его к строке \"",
		"Printing array: ", "\" и к массиву ", strings, ":");
	String dst;
	OutputCaesarCipher{LastAppender(dst)} << "Printing array: " << strings;
	output << dst;
	output.LineBreak();
}

void TestStreamRange(FormattedWriter& output)
{
	TestArrayRangeStreams(output);
	TestCustomOutputStream(output);
}
