#include "Core/Core.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if !defined(_HAS_EXCEPTIONS) && !defined(INTRA_EXCEPTIONS_ENABLED) && defined(_MSC_VER)
#define _HAS_EXCEPTIONS 0
#endif

#include "Core/Range/Concepts.h"
#include "IO/FormattedWriter.h"
#include "Container/Sequential/String.h"

INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <vector>
#include <string>
#include <list>
#include <deque>
#include <map>
#include <unordered_map>
INTRA_WARNING_POP

using namespace Intra;

struct Point
{
	float x, y;
	template<typename V> void ForEachField(V&& v) const {v(x); v(y);}
};

//TODO: move this
template<typename Char> void TestStrings(FormattedWriter& output)
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

void TestRangeStlInterop(FormattedWriter& output)
{
	std::vector<std::string> stringVec = {"Hello", "Intra", "Ranges"};
	output.PrintLine("Print std::vector<std::string> stringVec:");
	INTRA_ASSERT_EQUALS(StringOf(stringVec), "[Hello, Intra, Ranges]");
	output.PrintLine(stringVec);
	output.LineBreak();

	std::unordered_map<std::string, std::vector<Point>> figureMap = {
		{"Triangle", {{2.7f, 1.21f}, {3, 2.718f}, {4.321f, 3.212f}}},
		{"Line", {{1.1112f, 5.234f}, {6.22f, 5.45f}}},
		{"Point", {{65, 242}}}
	};
	output.PrintLine("Convert std::unordered_map<std::string, std::vector<int>> into a string on the stack and print it:");
	char figuresTextBuf[200];
	StringView figuresText = (SpanOutput<char>(figuresTextBuf) << figureMap).WrittenRange();
	INTRA_ASSERT_EQUALS(Count(figuresText, '['), Count(figuresText, ']'));
	output.PrintLine(figuresText);
	output.LineBreak();
	
	std::map<std::string, std::vector<Point>> orderedFigureMap = {
		{"Triangle", {{2.7f, 1.21f}, {3, 2.718f}, {4.321f, 3.212f}}},
		{"Line", {{1.1112f, 5.234f}, {6.22f, 5.45f}}},
		{"Point", {{65, 242}}}
	};
	output.PrintLine("Now d the same with std::map<std::string, std::vector<int>>:");
	figuresText = (SpanOutput<char>(figuresTextBuf) << orderedFigureMap).WrittenRange();
	INTRA_ASSERT_EQUALS(figuresText, "[[Line, [[1.1112, 5.234], [6.22, 5.45]]], [Point, [[65.0, 242.0]]], [Triangle, [[2.7, 1.21], [3.0, 2.718], [4.321, 3.212]]]]");
	output.PrintLine(figuresText);
	output.LineBreak();

	std::list<std::string> stringList(stringVec.begin(), stringVec.end());
	output.PrintLine("Print std::list<std::string> stringList:");
	INTRA_ASSERT_EQUALS(StringOf(stringList), "[Hello, Intra, Ranges]");
	output.PrintLine(stringList);
	output.LineBreak();

	output.PrintLine("Add 5 elements of Cycle(stringVec) to stringList");
	CopyTo(Take(Cycle(stringVec), 5), LastAppender(stringList));
	output.PrintLine("Print stringList:");
	INTRA_ASSERT_EQUALS(StringOf(stringList), "[Hello, Intra, Ranges, Hello, Intra, Ranges, Hello, Intra]");
	output.PrintLine(stringList);

	std::list<char> charList(stringVec[0].begin(), stringVec[0].end());
	INTRA_ASSERT_EQUALS(String(charList), "Hello");
	output.PrintLine("Print std::list<char>: ", charList);

	std::deque<char> charDeque(stringVec[2].begin(), stringVec[2].end());
	INTRA_ASSERT_EQUALS(String(charDeque), "Ranges");
	output.PrintLine("Print std::deque<char>: ", charDeque);

	String charListStr = charList;
	INTRA_ASSERT_EQUALS(charListStr, "Hello");
	output.PrintLine("Print String constructed from std::deque<char>: ", charListStr);

	String charListDequeConcat = charList + charDeque;
	INTRA_ASSERT_EQUALS(charListDequeConcat, "HelloRanges");
	output.PrintLine("Print String built by concatenation std::list<char> + std::deque<char>: ", charListDequeConcat);

	output.PrintLine("Use an universal operator+= working with all containers even STL:");
	charList += charDeque;
	charDeque += charList;
	output.PrintLine("charList += charDeque; charDeque += charList;");
	INTRA_ASSERT_EQUALS(String(charList), "HelloRanges");
	INTRA_ASSERT_EQUALS(String(charDeque), "RangesHelloRanges");
	output.PrintLine("Resulting charDeque = ", charDeque);
}

INTRA_WARNING_POP
