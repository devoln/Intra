#define INTRA_STL_INTERFACE
#include "PerfTestString.h"


#include "Test/PerformanceTest.h"
#include "IO/LogSystem.h"
#include "Containers/String.h"
#include "Core/Time.h"

#include <string>
#ifndef INTRA_MINIMIZE_CRT
#include <sstream>
#endif

using namespace Intra;
using namespace Intra::IO;


template<typename String> double TestStringReading(uint times, size_t strsize)
{
	int c=0;
	String str = GenerateRandomString<String>(strsize);
	
	Timer timer;
	for(uint j=0; j<times; j++)
	{
		for(size_t i=0; i<strsize; i++)
			c += str[i];
		srand(c); //Препятствуем оптимизации, удаляющей внешний цикл
	}
	return timer.GetTime();
}


template<typename String> double TestStringWriting(uint times, size_t strsize)
{
	char c='d';
	String str(strsize, ' ');

	Timer timer;
	for(uint j=0; j<times; j++)
		for(uint i=0; i<strsize; i++)
			str[i]=c;
	return timer.GetTime();
}



template<typename String> bool cmpString(const String& str1, const String& str2) {return str2==str1;}

template<typename String> double TestStringComparing1(uint times, size_t strsize)
{
	bool(*func)(const String& str1, const String& str2)=cmpString<String>;
	String str1 = GenerateRandomString<String>(strsize);
	String str2 = str1;

	Timer timer;
	for(uint i=0; i<times; i++)
		func(str1, str2);
	return timer.GetTime();
}

template<typename String> double TestStringComparing2(uint times, size_t strsize)
{
	bool(*func)(const String& str1, const String& str2)=cmpString<String>;
	String str1 = GenerateRandomString<String>(strsize+1);
	String str2(str1.begin(), str1.end()-1);

	Timer timer;
	for(uint i=0; i<times; i++)
		func(str1, str2);
	return timer.GetTime();
}

template<typename String> double TestStringComparing3(uint times, size_t strsize)
{
	bool(*func)(const String& str1, const String& str2)=cmpString<String>;
	String str1 = GenerateRandomString<String>(strsize);
	String str2 = str1;
	str2[0]='A';

	Timer timer;
	for(uint i=0; i<times; i++)
		func(str1, str2);
	return timer.GetTime();
}



template<typename String> double TestStringAccumulation1(uint times, size_t strsize, uint concatenations)
{
	String c = GenerateRandomString<String>(strsize);
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		String ss;
		for(uint j=0; j<concatenations; j++)
			ss+=c;
	}
	return timer.GetTime();
}

template<typename String> double TestStringAccumulation2(uint times, uint concatenations)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		String ss;
		for(uint j=0; j<concatenations; j++)
			ss+="str";
	}
	return timer.GetTime();
}

template<typename String> double TestStringConcatenationChain(uint times, size_t strsize)
{
	String c = GenerateRandomString<String>(strsize);
	Timer timer;
	for(uint j=0; j<times; j++)
		String ss = c+c+c+c+c+c+c+c+c+c;
	return timer.GetTime();
}

template<typename String> double TestStringConcatenationOptimizedChain(uint times, size_t strsize)
{
	String c = GenerateRandomString<String>(strsize);
	Timer timer;
	for(uint j=0; j<times; j++)
	{
		String ss = c;
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
	return timer.GetTime();
}

template<typename String> double TestStringConcatenation(uint times, size_t strsize)
{
	String c = GenerateRandomString<String>(strsize);
	String ss;
	Timer timer;
	for(uint j=0; j<times; j++)
		ss=c+c;
	return timer.GetTime();
}



template<typename String> double TestStringAddChar(uint times)
{
	String ss;
	Timer timer;
	for(uint j=0; j<times; j++)
		ss.push_back('g');
	return timer.GetTime();
}

template<typename String> double TestStringAddChar2(uint times, uint concatenations)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		String ss;
		for(uint j=0; j<concatenations; j++)
			ss=ss+'g';
	}
	return timer.GetTime();
}

template<typename String> double TestStringPlusChar(uint times, size_t strsize)
{
	String c = GenerateRandomString<String>(strsize);
	Timer timer;
	for(uint j=0; j<times; j++)
		String ss = c+'g';
	return timer.GetTime();
}



template<typename String> double TestStringCopying(uint times, size_t strsize)
{
	String str = GenerateRandomString<String>(strsize);
	Timer timer;
	for(uint j=0; j<times; j++)
		String copy=str;
	return timer.GetTime();
}

template<typename String> double TestCStrToStringCopying(uint times, size_t strsize)
{
	String strr = GenerateRandomString<String>(strsize);
	const char* str = strr.c_str();
	Timer timer;
	for(uint j=0; j<times; j++)
		String copy = str;
	return timer.GetTime();
}



double TestSprintf(uint times)
{
	Timer timer;
	char str[40];
	for(uint i=0; i<times; i++)
		sprintf(str, "int=%i, double=%f!", i, 234.0963);
	return timer.GetTime();
}

double TestStdToStringFormatting(uint times)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		std::string str = "int=";
		str += std::to_string(i);
		str += ", double=";
		str += std::to_string(234.0963);
		str.push_back('!');
	}
	return timer.GetTime();
}

#ifndef INTRA_MINIMIZE_CRT
double TestStreamFormatting(uint times)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		std::stringstream ss;
		ss << "int=" << i << ", double=" << 234.0963 << "!";
		ss.str();
	}
	return timer.GetTime();
}
#endif

double TestStringFormatting(uint times)
{
	Timer timer;
	for(uint i=0; i<times; i++)
		String str = String::Format("int=<^>, double=<^>!")(i)(234.0963, 7);
	return timer.GetTime();
}

double TestWStringStackFormatting(uint times)
{
	byte stackBuffer[1024];
	typedef GenericString<wchar, Memory::SizedAllocator<Memory::LinearAllocator>> WStackString;
	WStackString::Allocator arena(stackBuffer, 2);
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		arena.Reset();
#ifndef INTRA_UNICODE_STRING_LITERAL_SUPPORT
		WStackString str = WStackString::Format(L"int=<^>, double=<^>!", arena)(i)(234.0963, 7);
#else
		WStackString str = WStackString::Format(u"int=<^>, double=<^>!", arena)(i)(234.0963, 7);
#endif
	}
	return timer.GetTime();
}



template<typename String> double TestStringSubStr(uint times)
{
	String str = "Очень длинная строка, из которой нужно извлечь подстроку много раз для того, "
		"чтобы сравнить производительность извлечения подстрок классов String и std::string";
	Timer timer;
	for(uint i=0; i<times; i++)
		String substr = str.substr(4, 123-4);
	return timer.GetTime();
}




void RunStringPerfTests(Logger& logger)
{
	size_t sizeToAllocate = 2000;
	auto mem = Memory::StaticBufferAllocator::Allocate(sizeToAllocate, {__FILE__, (uint)__LINE__});
	Memory::StaticBufferAllocator::Free(mem, sizeToAllocate);

	StringView comparedStrings[2]={"std::string", "String"};

	if(TestGroup gr{logger, "operator[]"})
	{
		PrintPerformanceResults(logger, "Посимвольное чтение operator[](50000 раз, 2000 символов)",
			comparedStrings,
			{TestStringReading<std::string>(50000, 2000)},
			{TestStringReading<String>(50000, 2000)});

		PrintPerformanceResults(logger, "Посимвольная запись(50000 раз, 2000 символов)",
			comparedStrings,
			{TestStringWriting<std::string>(50000, 2000)},
			{TestStringWriting<String>(50000, 2000)});
	}

	if(TestGroup gr{logger, "Сравнение строк"})
	{
		PrintPerformanceResults(logger, "Одинаковые - 2000 символов",
			comparedStrings,
			{TestStringComparing1<std::string>(10000000, 2000)},
			{TestStringComparing1<String>(10000000, 2000)});

		PrintPerformanceResults(logger, "Одинаковые, с длинами 2001 и 2000",
			comparedStrings,
			{TestStringComparing2<std::string>(10000000, 2000)},
			{TestStringComparing2<String>(10000000, 2000)});

		PrintPerformanceResults(logger, "Отличающиеся первым символом(2000 символов):",
			comparedStrings,
			{TestStringComparing3<std::string>(10000000, 2000)},
			{TestStringComparing3<String>(10000000, 2000)});
	}

	if(TestGroup gr{logger, "Конкатенация строк"})
	{
		PrintPerformanceResults(logger, "Накопление строки (ss+=c) 5000000 конкатенаций (c - 50 символов)",
			comparedStrings,
			{TestStringAccumulation1<std::string>(1, 50, 5000000)},
			{TestStringAccumulation1<String>(1, 50, 5000000)});

		PrintPerformanceResults(logger, "Накопление строки (String+=\"str\") 50000000 конкатенаций",
			comparedStrings,
			{TestStringAccumulation2<std::string>(1, 50000000)},
			{TestStringAccumulation2<String>(1, 50000000)});

		PrintPerformanceResults(logger, "Цепочка из 10 конкатенаций (ss=c+c+...+c, где c - строка из 50 символов), 1000000 повторений",
			comparedStrings,
			{TestStringConcatenationChain<std::string>(1000000, 50)},
			{TestStringConcatenationChain<String>(1000000, 50)});

		PrintPerformanceResults(logger, "Оптимизированные 10 конкатенаций (ss=c; s+=c; s+=c; ... s+=c; где c - строка из 50 символов), 1000000 повторений",
			comparedStrings,
			{TestStringConcatenationOptimizedChain<std::string>(1000000, 50)},
			{TestStringConcatenationOptimizedChain<String>(1000000, 50)});

		PrintPerformanceResults(logger, "Конкатенация 2 строк (ss=c+c, где c - строка из 50 символов), 1000000 повторений",
			comparedStrings,
			{TestStringConcatenation<std::string>(1000000, 50)},
			{TestStringConcatenation<String>(1000000, 50)});

		PrintPerformanceResults(logger, "Добавление символа в конец строки (ss+='g') 10000000 раз",
			comparedStrings,
			{TestStringAddChar<std::string>(10000000)},
			{TestStringAddChar<String>(10000000)});

		PrintPerformanceResults(logger, "Добавление символа в конец строки (ss=ss+'g') 50000 раз, 10 повторений.",
			comparedStrings,
			{TestStringAddChar2<std::string>(10, 50000)},
			{TestStringAddChar2<String>(10, 50000)});

		PrintPerformanceResults(logger, "Добавление символа в конец строки без изменения исходной (ss=c+'g', где c - строка из 100 симв.)",
			comparedStrings,
			{TestStringPlusChar<std::string>(1000000, 100)},
			{TestStringPlusChar<String>(1000000, 100)});
	}

	if(TestGroup gr{logger, "Копирование строк"})
	{
		PrintPerformanceResults(logger, "Объект=Объект, String (2000 символов)",
			comparedStrings,
			{TestStringCopying<std::string>(10000000, 2000)},
			{TestStringCopying<String>(10000000, 2000)});

		PrintPerformanceResults(logger, "Объект=C-строка(2000 символов)",
			comparedStrings,
			{TestCStrToStringCopying<std::string>(10000000, 2000)},
			{TestCStrToStringCopying<String>(10000000, 2000)});
	}

	if(TestGroup gr{logger, "Форматирование строк"})
	{
		PrintPerformanceResults(logger, "Вставка int и double",
			{"sprintf", "std::string + std::to_string", 
#ifndef INTRA_MINIMIZE_CRT
			"stringstream",
#endif
			"String::Format", "WStackString::Format"},
			{TestSprintf(1000000), TestStdToStringFormatting(1000000),
#ifndef INTRA_MINIMIZE_CRT
			TestStreamFormatting(1000000)
#endif
			},
			{TestStringFormatting(1000000), TestWStringStackFormatting(1000000)});
	}

	if(TestGroup gr{logger, "Взятие подстроки"})
	{
		PrintPerformanceResults(logger, "Взятие подстроки",
			comparedStrings,
			{TestStringSubStr<std::string>(10000000)},
			{TestStringSubStr<String>(10000000)});
	}
}
