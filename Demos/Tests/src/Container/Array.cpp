#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include "Cpp/Warnings.h"
INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include "Array.h"


#include "Test/PerfSummary.h"
#include "Test/TestGroup.h"
#include "Platform/Time.h"
#include "Container/Sequential/Array.h"
#include "Container/Sequential/String.h"
#include "Container/Sequential/List.h"
#include "IO/LogSystem.h"



INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <vector>
#include <string>
#include <deque>
#include <list>
INTRA_WARNING_POP


using namespace Intra;
using namespace Intra::IO;

template<typename ARR> double TestContainerAddLast(uint times, uint size)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		ARR arr;
		for(uint j=0; j<size; j++)
			arr.push_back(j);
	}
	return timer.GetTime();
}

template<typename ARR> double TestArrayAddLastReserve(uint times, uint size)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		ARR arr;
		arr.reserve(size);
		for(uint j=0; j<size; j++)
			arr.push_back(j);
	}
	return timer.GetTime();
}

template<typename C, typename T> void push_front(C& c, const T& v) {c.push_front(v);}
template<typename T> void push_front(std::vector<T>& c, const T& v) {c.insert(c.begin(), v);}

template<typename ARR> double TestContainerAddFirst(uint times, uint size)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		ARR arr;
		for(uint j=0; j<size; j++)
			push_front(arr, j);
	}
	return timer.GetTime();
}

template<typename ARR> double TestArrayAddFirstReserve(uint times, uint size)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		ARR arr;
		arr.reserve(size);
		for(uint j=0; j<size; j++)
			push_front(arr, j);
	}
	return timer.GetTime();
}


template<typename C> void pop_front(C& c) {c.pop_front();}
template<typename T> void pop_front(std::vector<T>& c) {c.erase(c.begin());}

template<typename ARR> double TestContainerFirstElementRemove(uint size)
{
	ARR arr;
	Concepts::Reserve(arr, size);
	for(uint i=0; i<size; i++)
		arr.push_back(i);

	Timer timer;
	for(uint i=0; i<size; i++)
		pop_front(arr);
	return timer.GetTime();
}

template<typename ARR> double TestContainerLastElementRemove(uint size)
{
	ARR arr;
	Concepts::Reserve(arr, size);
	for(uint i=0; i<size; i++) 
		arr.push_back(i);

	Timer timer;
	for(uint i=0; i<size; i++)
		arr.pop_back();
	return timer.GetTime();
}


template<typename ARR> double TestContainerMiddleElementRemove(uint size)
{
	ARR arr;
	Concepts::Reserve(arr, size);
	for(uint i=0; i<size; i++)
		arr.push_back(i);

	Timer timer;
	for(uint i=size; i>0; i--)
		arr.erase(arr.begin()+intptr((i-1u)/2u));
	return timer.GetTime();
}

template<typename ARR> double TestContainerRemoveRange(uint size, uint elementsToRemove)
{
	ARR arr(size);
	for(uint i=0; i<size; i++)
		arr.push_back(i);

	Timer timer;
	for(uint i=size-elementsToRemove; i>=elementsToRemove; i-=elementsToRemove)
		arr.erase(arr.begin()+intptr(i/2u-elementsToRemove/2u+1u), arr.begin()+intptr(i/2u+elementsToRemove/2u));
	return timer.GetTime();
}

template<typename ARR> double TestContainerCopying(uint times, uint size)
{
	ARR arr;
	Concepts::Reserve(arr, size);
	for(uint i=0; i<size; i++)
		arr.push_back(i);

	Timer timer;
	for(uint i=0; i<times; i++)
		ARR arr2(arr);
	return timer.GetTime();
}


template<typename ARR> static ARR arrret(uint size)
{
	ARR arr;
	arr.resize(size);
	return arr;
}

template<typename ARR> double TestContainerReturn(uint times, uint size)
{
	Timer timer;
	for(uint i=0; i<times; i++)
		ARR arr = arrret<ARR>(size);
	return timer.GetTime();
}

template<typename ARR> double TestArraySmallReserve(uint times, uint size)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		ARR arr;
		arr.reserve(size);
	}
	return timer.GetTime();
}


static const String g_str="Строка, добавляемая в контейнер много раз.";

template<typename ARR> double TestContainerStringAddLast(uint times, uint elements)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		ARR arr;
		for(uint j=0; j<elements; j++)
			arr.push_back(g_str);
	}
	return timer.GetTime();
}

template<typename ARR> double TestContainerStringAddFirst(uint times, uint elements)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		ARR arr;
		for(uint j=0; j<elements; j++)
			push_front(arr, g_str);
	}
	return timer.GetTime();
}



template<typename ARR> double TestContainerFirstStringRemove(uint size)
{
	ARR arr;
	Concepts::Reserve(arr, size);
	for(uint i=0; i<size; i++)
		arr.push_back(g_str);

	Timer timer;
	for(uint i=0; i<size; i++)
		pop_front(arr);
	return timer.GetTime();
}

template<typename ARR> double TestContainerLastStringRemove(uint size)
{
	ARR arr;
	Concepts::Reserve(arr, size);
	for(uint i=0; i<size; i++)
		arr.push_back(g_str);

	Timer timer;
	for(uint i=0; i<size; i++)
		arr.pop_back();
	return timer.GetTime();
}

template<typename ARR> double TestContainerMiddleStringRemove(uint size)
{
	ARR arr;
	Concepts::Reserve(arr, size);
	for(uint i=0; i<size; i++)
		arr.push_back(g_str);

	Timer timer;
	for(uint i=size; i>0; i--)
		arr.erase(arr.begin()+intptr((i-1u)/2u));
	return timer.GetTime();
}

template<typename ARR> double TestContainerStringRemoveRange(size_t size, size_t elementsToRemove)
{
	ARR arr;
	Concepts::Reserve(arr, size);
	for(size_t i=0; i<size; i++)
		arr.push_back(g_str);

	Timer timer;
	for(size_t i=size-elementsToRemove/2-1; i>elementsToRemove/2; i-=elementsToRemove)
		arr.erase(arr.begin() + intptr(i/2-elementsToRemove/2+1), arr.begin() + intptr(i/2+elementsToRemove/2));
	return timer.GetTime();
}


template<typename ARR> double TestContainerStringCopying(uint times, uint size)
{
	ARR arr;
	Concepts::Reserve(arr, size);
	for(uint i=0; i<size; i++)
		arr.push_back(g_str);

	Timer timer;
	for(uint i=0; i<times; i++)
		ARR arr2(arr);
	return timer.GetTime();
}



void RunContainerPerfTests(FormattedWriter& output)
{
	StringView comparedArrays[] = {"std::vector", "Array"};
	StringView comparedContainers[] = {"std::vector", "std::deque", "std::list", "Array", "BList"};

	/*StringView comparedStringContainers[] = {
		"std::vector<std::string>", "std::deque<std::string>", "std::list<std::string>",
		"std::vector<String>", "std::deque<String>", "std::list<String>",
		"Array<std::string>", "BList<std::string>",
		"Array<String>", "BList<String>"
	};*/
	output.LineBreak(2);

	if(TestGroup gr{"Добавление int в контейнер"})
	{
		PrintPerformanceResults(output, "В конец 10000000 элементов 10 раз",
			{"std::vector", "std::deque", "std::list", "Array", "BList"},
			{
				TestContainerAddLast<std::vector<uint>>(10, 10000000),
				TestContainerAddLast<std::deque<uint>>(10, 10000000),
				TestContainerAddLast<std::list<uint>>(10, 10000000)
			},
			{
				TestContainerAddLast<Array<uint>>(10, 10000000),
				TestContainerAddLast<BList<uint>>(10, 10000000)
			});

		PrintPerformanceResults(output, "В начало 100000 элементов",
			{"std::vector", "std::deque", "std::list", "Array", "BList"},
			{
				TestContainerAddFirst<std::vector<uint>>(1, 100000),
				TestContainerAddFirst<std::deque<uint>>(1, 100000),
				TestContainerAddFirst<std::list<uint>>(1, 100000)
			},
			{
				TestContainerAddFirst<Array<uint>>(1, 100000),
				TestContainerAddFirst<BList<uint>>(1, 100000)
			});

		PrintPerformanceResults(output, "В конец 10000000 элементов 10 раз с reserve",
			comparedArrays,
			{
				TestArrayAddLastReserve<std::vector<uint>>(10, 10000000)
			},
			{
				TestArrayAddLastReserve<Array<uint>>(10, 10000000)
			});

		PrintPerformanceResults(output, "В начало 100000 элементов с reserve",
			comparedArrays,
			{
				TestArrayAddFirstReserve<std::vector<uint>>(1, 100000)
			},
			{
				TestArrayAddFirstReserve<Array<uint>>(1, 100000)
			});
	}

	if(TestGroup gr{"Удаление int из контейнера"})
	{
		PrintPerformanceResults(output, "С конца 10000000 раз",
			comparedContainers,
			{
				TestContainerLastElementRemove<std::vector<uint>>(10000000),
				TestContainerLastElementRemove<std::deque<uint>>(10000000),
				TestContainerLastElementRemove<std::list<uint>>(10000000)
			},
			{
				TestContainerLastElementRemove<Array<uint>>(10000000),
				TestContainerLastElementRemove<BList<uint>>(10000000)
			});

		PrintPerformanceResults(output, "Из начала 100000 раз",
			comparedContainers,
			{
				TestContainerFirstElementRemove<std::vector<uint>>(100000),
				TestContainerFirstElementRemove<std::deque<uint>>(100000),
				TestContainerFirstElementRemove<std::list<uint>>(100000)
			},
			{
				TestContainerFirstElementRemove<Array<uint>>(100000),
				TestContainerFirstElementRemove<BList<uint>>(100000)
			});

		PrintPerformanceResults(output, "Из середины 100000 раз",
			{"std::vector", "std::deque", "Array"},
			{
				TestContainerMiddleElementRemove<std::vector<uint>>(100000),
				TestContainerMiddleElementRemove<std::deque<uint>>(100000)
			},
			{
				TestContainerMiddleElementRemove<Array<uint>>(100000)
			});

		PrintPerformanceResults(output, "Из середины по 20 элементов (всего 100000 элементов)",
			{"std::vector", "std::deque", "Array"},
			{
				TestContainerRemoveRange<std::vector<uint>>(100000, 20),
				TestContainerRemoveRange<std::deque<uint>>(100000, 20)
			},
			{
				TestContainerRemoveRange<Array<uint>>(100000, 20)
			});
	}

	if(TestGroup gr{"Копирование контейнеров int"})
	{
		PrintPerformanceResults(output, "Размер копируемых контейнеров 10000000",
			comparedContainers,
			{
				TestContainerCopying<std::vector<uint>>(10, 10000000),
				TestContainerCopying<std::deque<uint>>(10, 10000000),
				TestContainerCopying<std::list<uint>>(10, 10000000)},
			{
				TestContainerCopying<Array<uint>>(10, 10000000),
				TestContainerCopying<BList<uint>>(10, 10000000)
			});
	}

	if(TestGroup gr{"Возврат контейнеров из функции"})
	{
		PrintPerformanceResults(output, "Элементов в контейнере: 10 (10000000 раз)",
			comparedContainers,
			{
				TestContainerReturn<std::vector<uint>>(10000000, 10),
				TestContainerReturn<std::deque<uint>>(10000000, 10),
				TestContainerReturn<std::list<uint>>(10000000, 10)},
			{
				TestContainerReturn<Array<uint>>(10000000, 10),
				TestContainerReturn<BList<uint>>(10000000, 10)
			});

		PrintPerformanceResults(output, "Элементов в контейнере: 100 (1000000 раз)",
			comparedContainers,
			{
				TestContainerReturn<std::vector<uint>>(1000000, 100),
				TestContainerReturn<std::deque<uint>>(1000000, 100),
				TestContainerReturn<std::list<uint>>(1000000, 100)},
			{
				TestContainerReturn<Array<uint>>(1000000, 100),
				TestContainerReturn<BList<uint>>(1000000, 100)
			});

		PrintPerformanceResults(output, "Элементов в контейнере: 1000 (100000 раз)",
			comparedContainers,
			{
				TestContainerReturn<std::vector<uint>>(100000, 1000),
				TestContainerReturn<std::deque<uint>>(100000, 1000),
				TestContainerReturn<std::list<uint>>(100000, 1000)
			},
			{
				TestContainerReturn<Array<uint>>(100000, 1000),
				TestContainerReturn<BList<uint>>(100000, 1000)
			});
	}

	if(TestGroup gr{"Выделение памяти для небольших массивов"})
	{
		PrintPerformanceResults(output, "Выделение памяти для 50 элементов массива (1000000 раз)",
			comparedArrays,
			{
				TestArraySmallReserve<std::vector<uint>>(1000000, 50)
			},
			{
				TestArraySmallReserve<Array<uint>>(1000000, 50)
			});
	}

	if(TestGroup gr{"Добавление String в контейнер"})
	{
		PrintPerformanceResults(output, "В конец (1000000 элементов 10 раз)",
			comparedContainers,
			{
				TestContainerStringAddLast<std::vector<String>>(10, 1000000),
				TestContainerStringAddLast<std::deque<String>>(10, 1000000),
				TestContainerStringAddLast<std::list<String>>(10, 1000000)
			},
			{
				TestContainerStringAddLast<Array<String>>(10, 1000000),
				TestContainerStringAddLast<BList<String>>(10, 1000000)
			});

		PrintPerformanceResults(output, "В начало (100000 элементов)",
			comparedContainers,
			{
				TestContainerStringAddFirst<std::vector<String>>(1, 100000),
				TestContainerStringAddFirst<std::deque<String>>(1, 100000),
				TestContainerStringAddFirst<std::list<String>>(1, 100000)
			},
			{
				TestContainerStringAddFirst<Array<String>>(1, 100000),
				TestContainerStringAddFirst<BList<String>>(1, 100000)
			});
	}

	if(TestGroup gr{"Удаление String из контейнера"})
	{
		PrintPerformanceResults(output, "С конца (1000000 элементов)",
			comparedContainers,
			{
				TestContainerLastStringRemove<std::vector<String>>(1000000),
				TestContainerLastStringRemove<std::deque<String>>(1000000),
				TestContainerLastStringRemove<std::list<String>>(1000000)
			},
			{
				TestContainerLastStringRemove<Array<String>>(1000000),
				TestContainerLastStringRemove<BList<String>>(1000000)
			});

		PrintPerformanceResults(output, "Из начала (100000 элементов)",
			comparedContainers,
			{
				TestContainerFirstStringRemove<std::vector<String>>(100000),
				TestContainerFirstStringRemove<std::deque<String>>(100000),
				TestContainerFirstStringRemove<std::list<String>>(100000)
			},
			{
				TestContainerFirstStringRemove<Array<String>>(100000),
				TestContainerFirstStringRemove<BList<String>>(100000)
			});

		PrintPerformanceResults(output, "Из середины (100000 элементов)",
			{"std::vector", "std::deque", "Array"},
			{
				TestContainerMiddleStringRemove<std::vector<String>>(100000),
				TestContainerMiddleStringRemove<std::deque<String>>(100000)
			},
			{
				TestContainerMiddleStringRemove<Array<String>>(100000)
			});

		PrintPerformanceResults(output, "Из середины по 20 (всего 100000 элементов)",
			{"std::vector", "std::deque", "Array"},
			{
				TestContainerStringRemoveRange<std::vector<String>>(100000, 20),
				TestContainerStringRemoveRange<std::deque<String>>(100000, 20)
			},
			{
				TestContainerStringRemoveRange<Array<String>>(100000, 20)
			});
	}

	if(TestGroup gr{"Копирование контейнеров строк"})
	{
		PrintPerformanceResults(output, "Размер контейнеров 1000000",
			comparedContainers,
			{
				TestContainerStringCopying<std::vector<String>>(10, 1000000),
				TestContainerStringCopying<std::deque<String>>(10, 1000000),
				TestContainerStringCopying<std::list<String>>(10, 1000000)
			},
			{
				TestContainerStringCopying<Array<String>>(10, 1000000),
				TestContainerStringCopying<BList<String>>(10, 1000000)
			});
	}
}

INTRA_WARNING_POP
