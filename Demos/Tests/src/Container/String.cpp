
#ifndef INTRA_STL_INTERFACE
#define INTRA_STL_INTERFACE
#endif

#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include "Cpp/Warnings.h"
#include "Cpp/Runtime.h"
INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include "String.h"

#include "Cpp/Compatibility.h"
#include "Test.hh"
#include "IO/LogSystem.h"
#include "Container/Sequential/String.h"
#include "System/Stopwatch.h"
#include "Range/Output/OutputArrayRange.h"
#include "Range/Output/Inserter.h"


INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <string>
#include <vector>
#ifndef INTRA_MINIMIZE_CRT
#include <sstream>
#endif
INTRA_WARNING_POP

using namespace Intra;
using namespace IO;
using namespace Range;

template<typename String> double TestStringReading(uint times, size_t strsize)
{
	int c=0;
	String str = GenerateRandomString<String>(strsize);
	
	Stopwatch timer;
	for(uint j=0; j<times; j++)
	{
		for(size_t i=0; i<strsize; i++)
			c += str[i];
		srand(uint(c)); //Препятствуем оптимизации, удаляющей внешний цикл
	}
	return timer.ElapsedSeconds();
}


template<typename String> double TestStringWriting(uint times, size_t strsize)
{
	char c = 'd';
	String str(strsize, ' ');

	Stopwatch timer;
	for(uint j=0; j<times; j++)
		for(uint i=0; i<strsize; i++)
			str[i] = c;
	return timer.ElapsedSeconds();
}



template<typename String> bool cmpString(const String& str1, const String& str2) {return str2 == str1;}

template<typename String> double TestStringComparing1(uint times, size_t strsize)
{
	bool(*func)(const String& str1, const String& str2)=cmpString<String>;
	String str1 = GenerateRandomString<String>(strsize);
	String str2 = str1;

	Stopwatch timer;
	for(uint i=0; i<times; i++)
		func(str1, str2);
	return timer.ElapsedSeconds();
}

template<typename String> double TestStringComparing2(uint times, size_t strsize)
{
	bool(*func)(const String& str1, const String& str2)=cmpString<String>;
	String str1 = GenerateRandomString<String>(strsize+1);
	String str2(str1.begin(), str1.end()-1);

	Stopwatch timer;
	for(uint i=0; i<times; i++)
		func(str1, str2);
	return timer.ElapsedSeconds();
}

template<typename String> double TestStringComparing3(uint times, size_t strsize)
{
	bool(*func)(const String& str1, const String& str2)=cmpString<String>;
	String str1 = GenerateRandomString<String>(strsize);
	String str2 = str1;
	str2[0]='A';

	Stopwatch timer;
	for(uint i=0; i<times; i++)
		func(str1, str2);
	return timer.ElapsedSeconds();
}



template<typename String> double TestStringAccumulation1(uint times, size_t strsize, uint concatenations)
{
	String c = GenerateRandomString<String>(strsize);
	Stopwatch timer;
	for(uint i=0; i<times; i++)
	{
		String ss;
		for(uint j=0; j<concatenations; j++)
			ss += c;
	}
	return timer.ElapsedSeconds();
}

template<typename S> double TestStringAccumulation2(uint times, uint concatenations)
{
	Stopwatch timer;
	for(uint i=0; i<times; i++)
	{
		S ss;
		for(uint j=0; j<concatenations; j++)
			ss += "str";
	}
	return timer.ElapsedSeconds();
}

template<typename S> double TestStringConcatenationChain(uint times, size_t strsize)
{
	S c = GenerateRandomString<S>(strsize);
	Stopwatch timer;
	for(uint j=0; j<times; j++)
		S ss = c+c+c+c+c+c+c+c+c+c;
	return timer.ElapsedSeconds();
}

template<typename S> double TestStringConcatenationOptimizedChain(uint times, size_t strsize)
{
	S c = GenerateRandomString<S>(strsize);
	Stopwatch timer;
	for(uint j=0; j<times; j++)
	{
		S ss = c;
		ss += c;
		ss += c;
		ss += c;
		ss += c;
		ss += c;
		ss += c;
		ss += c;
		ss += c;
		ss += c;
	}
	return timer.ElapsedSeconds();
}

template<typename S> double TestStringConcatenation(uint times, size_t strsize)
{
	const S c = GenerateRandomString<S>(strsize);
	S ss;
	Stopwatch timer;
	for(uint j=0; j<times; j++)
		ss = c + c;
	return timer.ElapsedSeconds();
}



template<typename S> double TestStringAddChar(uint times)
{
	S ss;
	Stopwatch timer;
	for(uint j=0; j<times; j++)
		ss.push_back('g');
	return timer.ElapsedSeconds();
}

template<typename S> double TestStringAddChar2(uint times, uint concatenations)
{
	Stopwatch timer;
	for(uint i=0; i<times; i++)
	{
		S ss;
		for(uint j=0; j<concatenations; j++)
			ss = ss + 'g';
	}
	return timer.ElapsedSeconds();
}

template<typename S> double TestStringPlusChar(uint times, size_t strsize)
{
	S c = GenerateRandomString<S>(strsize);
	Stopwatch timer;
	for(uint j=0; j<times; j++)
		S ss = c + 'g';
	return timer.ElapsedSeconds();
}



template<typename S> double TestStringCopying(uint times, size_t strsize)
{
	const S str = GenerateRandomString<S>(strsize);
	Stopwatch timer;
	for(uint j=0; j<times; j++)
		S copy = str;
	return timer.ElapsedSeconds();
}

template<typename S> double TestCStrToStringCopying(uint times, size_t strsize)
{
	S strr = GenerateRandomString<S>(strsize);
	const char* const str = strr.c_str();
	Stopwatch timer;
	for(uint j=0; j<times; j++)
		S copy = str;
	return timer.ElapsedSeconds();
}



double TestSprintf(uint times)
{
	Stopwatch timer;
	char str[40];
	for(uint i=0; i<times; i++)
		sprintf(str, "int=%i, double=%f!", i, 234.0963);
	return timer.ElapsedSeconds();
}

#ifndef INTRA_MINIMIZE_CRT
double TestStdToStringFormatting(uint times)
{
	Stopwatch timer;
	for(uint i=0; i<times; i++)
	{
		std::string str = "int=";
		str += std::to_string(i);
		str += ", double=";
		str += std::to_string(234.0963);
		str.push_back('!');
	}
	return timer.ElapsedSeconds();
}

double TestStrStreamFormatting(uint times)
{
	Stopwatch timer;
	for(uint i=0; i<times; i++)
	{
		std::stringstream ss;
		ss << "int=" << i << ", double=" << 234.0963 << "!";
		auto str = ss.str();
	}
	return timer.ElapsedSeconds();
}

double TestSharedStrStreamFormatting(uint times)
{
	Stopwatch timer;
	std::stringstream ss;
	for(uint i = 0; i<times; i++)
	{
		ss << "int=" << i << ", double=" << 234.0963 << "!";
		auto str = ss.str();
		ss.clear();
		ss.seekp(0);
		ss.seekg(0);
	}
	return timer.ElapsedSeconds();
}
#endif

template<typename S> double TestStringFormatting(uint times)
{
	Stopwatch timer;
	for(uint i=0; i<times; i++)
		S str = Container::StringFormatter<S>("int=<^>, double=<^>!")(i)(234.0963, 7);
	return timer.ElapsedSeconds();
}

double TestStackStreamFormatting(uint times)
{
	char stackBuffer[40];
	Stopwatch timer;
	for(uint i=0; i<times; i++)
	{
		OutputArrayRange<char> strStream = stackBuffer;
		strStream << "int=" << i << ", double=" << 234.0963 << "!";
		auto str = strStream.GetWrittenData();
		(void)str;
	}
	return timer.ElapsedSeconds();
}

template<typename S> double TestAppenderStreamFormatting(uint times)
{
	Stopwatch timer;
	for(uint i = 0; i<times; i++)
	{
		S result;
		LastAppender(result) << "int=" << i << ", double=" << 234.0963 << "!";
	}
	return timer.ElapsedSeconds();
}



template<typename S> double TestStringSubStr(uint times)
{
	S str = "Очень длинная строка, из которой нужно извлечь подстроку много раз для того, "
		"чтобы сравнить производительность извлечения подстрок классов String и std::string";
	Stopwatch timer;
	for(uint i=0; i<times; i++)
		S substr = str.substr(4, 123-4);
	return timer.ElapsedSeconds();
}

double TestStringSubStrView(uint times)
{
	String str = "Очень длинная строка, из которой нужно извлечь подстроку много раз для того, "
		"чтобы сравнить производительность извлечения подстрок классов String и std::string";
	Stopwatch timer;
	for(uint i = 0; i<times; i++)
	{
		StringView substr = str(4, 123);
		(void)substr;
	}
	return timer.ElapsedSeconds();
}




void RunStringPerfTests(FormattedWriter& output)
{
	StringView comparedStrings[2] = {"std::string", "String"};

	if(TestGroup gr{"operator[]"})
	{
		PrintPerformanceResults(output, "Посимвольное чтение operator[](50000 раз, 2000 символов)",
			comparedStrings,
			{TestStringReading<std::string>(50000, 2000)},
			{TestStringReading<String>(50000, 2000)});

		PrintPerformanceResults(output, "Посимвольная запись(50000 раз, 2000 символов)",
			comparedStrings,
			{TestStringWriting<std::string>(50000, 2000)},
			{TestStringWriting<String>(50000, 2000)});
	}

	if(TestGroup gr{"Сравнение строк"})
	{
		PrintPerformanceResults(output, "Одинаковые - 2000 символов",
			comparedStrings,
			{TestStringComparing1<std::string>(10000000, 2000)},
			{TestStringComparing1<String>(10000000, 2000)});

		PrintPerformanceResults(output, "Одинаковые, с длинами 2001 и 2000",
			comparedStrings,
			{TestStringComparing2<std::string>(10000000, 2000)},
			{TestStringComparing2<String>(10000000, 2000)});

		PrintPerformanceResults(output, "Отличающиеся первым символом (2000 символов):",
			comparedStrings,
			{TestStringComparing3<std::string>(10000000, 2000)},
			{TestStringComparing3<String>(10000000, 2000)});
	}

	if(TestGroup gr{"Конкатенация строк"})
	{
		PrintPerformanceResults(output, "Накопление строки (ss+=c) 5000000 конкатенаций (c - 50 символов)",
			comparedStrings,
			{TestStringAccumulation1<std::string>(1, 50, 5000000)},
			{TestStringAccumulation1<String>(1, 50, 5000000)});

		PrintPerformanceResults(output, "Накопление строки (String+=\"str\") 50000000 конкатенаций",
			comparedStrings,
			{TestStringAccumulation2<std::string>(1, 50000000)},
			{TestStringAccumulation2<String>(1, 50000000)});

		PrintPerformanceResults(output, "Цепочка из 10 конкатенаций (ss=c+c+...+c, где c - строка из 50 символов), 1000000 повторений",
			comparedStrings,
			{TestStringConcatenationChain<std::string>(1000000, 50)},
			{TestStringConcatenationChain<String>(1000000, 50)});

		PrintPerformanceResults(output, "Оптимизированные 10 конкатенаций (ss=c; s+=c; s+=c; ... s+=c; где c - строка из 50 символов), 1000000 повторений",
			comparedStrings,
			{TestStringConcatenationOptimizedChain<std::string>(1000000, 50)},
			{TestStringConcatenationOptimizedChain<String>(1000000, 50)});

		PrintPerformanceResults(output, "Конкатенация 2 строк (ss=c+c, где c - строка из 50 символов), 1000000 повторений",
			comparedStrings,
			{TestStringConcatenation<std::string>(1000000, 50)},
			{TestStringConcatenation<String>(1000000, 50)});

		PrintPerformanceResults(output, "Добавление символа в конец строки (ss += 'g') 10000000 раз",
			comparedStrings,
			{TestStringAddChar<std::string>(10000000)},
			{TestStringAddChar<String>(10000000)});

		PrintPerformanceResults(output, "Добавление символа в конец строки (ss = ss + 'g') 100000 раз",
			comparedStrings,
			{TestStringAddChar2<std::string>(1, 100000)},
			{TestStringAddChar2<String>(1, 100000)});

		PrintPerformanceResults(output, "Добавление символа в конец строки без изменения исходной (ss = c + 'g', где c - строка из 100 симв.)",
			comparedStrings,
			{TestStringPlusChar<std::string>(1000000, 100)},
			{TestStringPlusChar<String>(1000000, 100)});
	}

	if(TestGroup gr{"Копирование строк"})
	{
		PrintPerformanceResults(output, "Объект=Объект, String (2000 символов)",
			comparedStrings,
			{TestStringCopying<std::string>(10000000, 2000)},
			{TestStringCopying<String>(10000000, 2000)});

		PrintPerformanceResults(output, "Объект=C-строка(2000 символов)",
			comparedStrings,
			{TestCStrToStringCopying<std::string>(10000000, 2000)},
			{TestCStrToStringCopying<String>(10000000, 2000)});
	}

	if(TestGroup gr{"Форматирование строк"})
	{
		PrintPerformanceResults(output, "Вставка int и double",
			{"sprintf",
#ifndef INTRA_MINIMIZE_CRT
			 "std::string + std::to_string",
			"stringstream",
			"общий stringstream",
#endif
			"StringFormatter<std::string>",
			"StringFormatter<std::vector<char>>",
			"String::Format",
			"Stack char stream <<",
			"LastAppender(std::string)",
			"LastAppender(String)"
			},
			{TestSprintf(1000000),
#ifndef INTRA_MINIMIZE_CRT
			TestStdToStringFormatting(1000000),
			TestStrStreamFormatting(1000000),
			TestSharedStrStreamFormatting(1000000)
#endif
			},
			{
				TestStringFormatting<std::string>(1000000),
				TestStringFormatting<std::vector<char>>(1000000),
				TestStringFormatting<String>(1000000),
				TestStackStreamFormatting(1000000),
				TestAppenderStreamFormatting<std::string>(1000000),
				TestAppenderStreamFormatting<String>(1000000)
			});
	}

	if(TestGroup gr{"Взятие подстроки"})
	{
		PrintPerformanceResults(output, "Взятие подстроки",
			{"std::string", "String", "String -> StringView"},
			{TestStringSubStr<std::string>(10000000)},
			{TestStringSubStr<String>(10000000), TestStringSubStrView(10000000)});
	}
}

INTRA_WARNING_POP
