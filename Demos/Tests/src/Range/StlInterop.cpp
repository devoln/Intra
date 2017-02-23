#include "Range.hh"
#include "IO/Stream.h"
#include <vector>
#include <string>
#include <list>
#include <deque>
#include <unordered_map>

using namespace Intra;
using namespace Range;
using namespace IO;
using namespace Algo;

struct Point
{
	float x, y;
	template<typename V> void ForEachField(V&& v) {v(x); v(y);}
	template<typename V> void ForEachField(V&& v) const {v(x); v(y);}
};

//TODO: перенести это
template<typename Char> void TestStrings()
{
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

void RunRangeStlInteropTests()
{
	std::vector<std::string> stringVec = {"Hello", "Intra", "Ranges"};
	Console.PrintLine("Выводим std::vector<std::string> stringVec:");
	Console.PrintLine(stringVec, endl);

	std::unordered_map<std::string, std::vector<Point>> figureMap = {
		{"Triangle", {{2.7f, 1.21f}, {3, 2.71828f}, {4.321f, 3.2123f}}},
		{"Line", {{1.1112f, 5.234f}, {6.22f, 5.45f}}},
		{"Point", {{65, 242}}}
	};
	Console.PrintLine("Переведём std::unordered_map<std::string, std::vector<int>> в строку на стеке и выведем её:");
	char figuresTextBuf[200];
	StringView figuresText = (OutputArrayRange<char>(figuresTextBuf) << figureMap).GetWrittenData();
	Console.PrintLine(figuresText, endl);

	std::list<std::string> stringList(stringVec.begin(), stringVec.end());
	Console.PrintLine("Выводим std::list<std::string> stringList:");
	Console.PrintLine(stringList, endl);

	Console.PrintLine("Добавляем в предыдущий список 10 элементов Cycle(stringVec)");
	CopyTo(Take(Cycle(stringVec), 10), LastAppender(stringList));
	Console.PrintLine("Снова выводим stringList:");
	Console.PrintLine(stringList);

	std::list<char> charList(stringVec[0].begin(), stringVec[0].end());
	Console.PrintLine("Выведем связный список символов: ", charList);
	std::deque<char> charDeque(stringVec[2].begin(), stringVec[2].end());
	Console.PrintLine("Выведем deque символов: ", charDeque);
	String charListStr = charList;
	Console.PrintLine("Выведем строку, сконструированную из deque символов: ", charListStr);
	String charListConcat = charList+charDeque;
	Console.PrintLine("Выведем строку, полученную конкатенацией связного списка и deque символов: ", charListConcat);

	Console.PrintLine("Используем универсальный оператор +=, который работает со всеми контейнерами:");
	using Container::operator+=;
	charList += charDeque;
	charDeque += charList;
	Console.PrintLine("charList += charDeque; charDeque += charList;");
	Console.PrintLine("В результате charDeque содержит: ", charDeque);
}
