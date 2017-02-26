#include "Range.hh"
#include "IO/Stream.h"
#include "IO/FormattedWriter.h"

INTRA_DISABLE_REDUNDANT_WARNINGS
#define _HAS_EXCEPTIONS 0

#ifdef _MSC_VER
#pragma warning(disable: 4548)
#endif

#include <vector>
#include <string>
#include <list>
#include <deque>
#include <map>
#include <unordered_map>

using namespace Intra;
using namespace Range;
using namespace IO;
using namespace Algo;

struct Point
{
	float x, y;
	template<typename V> void ForEachField(V&& v) const {v(x); v(y);}
};

//TODO: перенести это
template<typename Char> void TestStrings(IO::IFormattedWriter& output)
{
	(void)output;
	GenericString<Char> emptyStr;
	GenericString<Char> shortStr = L"ShortString";
	auto shortCStr = shortStr.CStr();
	GenericString<Char> longerStr = u"LongerString";
	auto longerCStr = longerStr.CStr();
	GenericString<Char> veryShortStr = "VSS";
	auto veryShortCStr = veryShortStr.CStr();
	GenericString<Char> longStr = "This is a long string!";
	auto longCStr = longStr.CStr();
	GenericString<Char> veryLongStr = "This is a very long string!";
	auto veryLongCStr = veryLongStr.CStr();
}

void TestRangeStlInterop(IO::IFormattedWriter& output)
{
	std::vector<std::string> stringVec = {"Hello", "Intra", "Ranges"};
	output.PrintLine("Выводим std::vector<std::string> stringVec:");
	INTRA_ASSERT_EQUALS(ToString(stringVec), "[Hello, Intra, Ranges]");
	output.PrintLine(stringVec, endl);

	std::unordered_map<std::string, std::vector<Point>> figureMap = {
		{"Triangle", {{2.7f, 1.21f}, {3, 2.718f}, {4.321f, 3.212f}}},
		{"Line", {{1.1112f, 5.234f}, {6.22f, 5.45f}}},
		{"Point", {{65, 242}}}
	};
	output.PrintLine("Переведём std::unordered_map<std::string, std::vector<int>> в строку на стеке и выведем её:");
	char figuresTextBuf[200];
	StringView figuresText = (OutputArrayRange<char>(figuresTextBuf) << figureMap).GetWrittenData();
	INTRA_ASSERT_EQUALS(Algo::Count(figuresText, '['), Algo::Count(figuresText, ']'));
	output.PrintLine(figuresText, endl);
	
	std::map<std::string, std::vector<Point>> orderedFigureMap = {
		{"Triangle", {{2.7f, 1.21f}, {3, 2.718f}, {4.321f, 3.212f}}},
		{"Line", {{1.1112f, 5.234f}, {6.22f, 5.45f}}},
		{"Point", {{65, 242}}}
	};
	output.PrintLine("Теперь проделаем то же самое с std::map<std::string, std::vector<int>>:");
	figuresText = (OutputArrayRange<char>(figuresTextBuf) << orderedFigureMap).GetWrittenData();
	INTRA_ASSERT_EQUALS(figuresText, "[[Line, [[1.1112, 5.234], [6.22, 5.45]]], [Point, [[65.0, 242.0]]], [Triangle, [[2.7, 1.21], [3.0, 2.718], [4.321, 3.212]]]]");
	output.PrintLine(figuresText, endl);

	std::list<std::string> stringList(stringVec.begin(), stringVec.end());
	output.PrintLine("Выводим std::list<std::string> stringList:");
	INTRA_ASSERT_EQUALS(ToString(stringList), "[Hello, Intra, Ranges]");
	output.PrintLine(stringList, endl);

	output.PrintLine("Добавляем в предыдущий список 5 элементов Cycle(stringVec)");
	CopyTo(Take(Cycle(stringVec), 5), LastAppender(stringList));
	output.PrintLine("Снова выводим stringList:");
	INTRA_ASSERT_EQUALS(ToString(stringList), "[Hello, Intra, Ranges, Hello, Intra, Ranges, Hello, Intra]");
	output.PrintLine(stringList);

	std::list<char> charList(stringVec[0].begin(), stringVec[0].end());
	INTRA_ASSERT_EQUALS(String(charList), "Hello");
	output.PrintLine("Выведем связный список символов: ", charList);

	std::deque<char> charDeque(stringVec[2].begin(), stringVec[2].end());
	INTRA_ASSERT_EQUALS(String(charDeque), "Ranges");
	output.PrintLine("Выведем deque символов: ", charDeque);

	String charListStr = charList;
	INTRA_ASSERT_EQUALS(charListStr, "Hello");
	output.PrintLine("Выведем строку, сконструированную из deque символов: ", charListStr);

	String charListDequeConcat = charList+charDeque;
	INTRA_ASSERT_EQUALS(charListDequeConcat, "HelloRanges");
	output.PrintLine("Выведем строку, полученную конкатенацией связного списка и deque символов: ", charListDequeConcat);

	output.PrintLine("Используем универсальный оператор +=, который работает со всеми контейнерами:");
	using Container::operator+=;
	charList += charDeque;
	charDeque += charList;
	output.PrintLine("charList += charDeque; charDeque += charList;");
	INTRA_ASSERT_EQUALS(String(charList), "HelloRanges");
	INTRA_ASSERT_EQUALS(String(charDeque), "RangesHelloRanges");
	output.PrintLine("В результате charDeque содержит: ", charDeque);
}
