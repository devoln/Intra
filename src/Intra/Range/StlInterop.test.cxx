#include "Intra/Range/Concepts.h"
#include "Extra/IO/FormattedWriter.h"
#include "Extra/Container/Sequential/String.h"
#include "Extra/System/Debug.h"

INTRA_PUSH_DISABLE_ALL_WARNINGS
#if !defined(_HAS_EXCEPTIONS) && !defined(INTRA_EXCEPTIONS_ENABLED) && defined(_MSC_VER)
#define _HAS_EXCEPTIONS 0
#endif
#include <vector>
#include <string>
#include <list>
#include <deque>
#include <map>
#include <unordered_map>
INTRA_WARNING_POP

INTRA_BEGIN
struct Point
{
	int x, y;
	template<typename V> void ForEachField(V&& v) const {v(x); v(y);}
};

INTRA_MODULE_UNITTEST
{
	std::vector<std::string> stringVec = {"Hello", "Intra", "Ranges"};
	INTRA_ASSERT_EQUALS(StringOf(stringVec), "[Hello, Intra, Ranges]");

	std::unordered_map<std::string, std::vector<Point>> figureMap = {
		{"Triangle", {{3, 1}, {3, 2}, {4, 3}}},
		{"Line", {{1, 5}, {6, 5}}}
	};
	char figuresTextBuf[200];
	StringView figuresText = (SpanOutput<char>(figuresTextBuf) << figureMap).WrittenRange();
	INTRA_ASSERT(figuresText == "[[Line, [[1, 5], [6, 5]]], [Triangle, [[3, 1], [3, 2], [4, 3]]]]" ||
		figuresText == "[[Triangle, [[3, 1], [3, 2], [4, 3]]], [Line, [[1, 5], [6, 5]]]]");
	
	std::map<std::string, std::vector<Point>> orderedFigureMap = {
		{"Triangle", {{3, 1}, {3, 2}, {4, 3}}},
		{"Line", {{1, 5}, {6, 5}}},
		{"Point", {{65, 242}}}
	};
	figuresText = (SpanOutput<char>(figuresTextBuf) << orderedFigureMap).WrittenRange();
	INTRA_ASSERT_EQUALS(figuresText, "[[Line, [[1, 5], [6, 5]]], [Point, [[65, 242]]], [Triangle, [[3, 1], [3, 2], [4, 3]]]]");

	std::list<std::string> stringList(stringVec.begin(), stringVec.end());
	INTRA_ASSERT_EQUALS(StringOf(stringList), "[Hello, Intra, Ranges]");

	CopyTo(Take(Cycle(stringVec), 5), LastAppender(stringList));
	INTRA_ASSERT_EQUALS(StringOf(stringList), "[Hello, Intra, Ranges, Hello, Intra, Ranges, Hello, Intra]");

	std::list<char> charList(stringVec[0].begin(), stringVec[0].end());
	INTRA_ASSERT_EQUALS(String(charList), "Hello");

	std::deque<char> charDeque(stringVec[2].begin(), stringVec[2].end());
	INTRA_ASSERT_EQUALS(String(charDeque), "Ranges");

	String charListDequeConcat = charList + charDeque;
	INTRA_ASSERT_EQUALS(charListDequeConcat, "HelloRanges");

	charList += charDeque;
	INTRA_ASSERT_EQUALS(String(charList), "HelloRanges");
	charDeque += charList;
	INTRA_ASSERT_EQUALS(String(charDeque), "RangesHelloRanges");
}

INTRA_END
