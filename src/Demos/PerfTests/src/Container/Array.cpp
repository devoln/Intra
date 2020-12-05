#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#ifndef INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY
#define INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY
#endif


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include "Array.h"


#include "Test/PerfSummary.h"
#include "Test/TestGroup.h"
#include "IntraX/System/Stopwatch.h"
#include "IntraX/Container/Sequential/Array.h"
#include "IntraX/Container/Sequential/String.h"
#include "IntraX/Container/Sequential/List.h"



INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <vector>
#include <string>
#include <deque>
#include <list>
INTRA_WARNING_POP


using namespace Intra;
using namespace IO;

template<typename ARR> double TestContainerAddLast(unsigned times, unsigned size)
{
	Stopwatch timer;
	for(unsigned i=0; i<times; i++)
	{
		ARR arr;
		for(unsigned j=0; j<size; j++)
			arr.push_back(j);
	}
	return timer.ElapsedSeconds();
}

template<typename ARR> double TestArrayAddLastReserve(unsigned times, unsigned size)
{
	Stopwatch timer;
	for(unsigned i=0; i<times; i++)
	{
		ARR arr;
		arr.reserve(size);
		for(unsigned j=0; j<size; j++)
			arr.push_back(j);
	}
	return timer.ElapsedSeconds();
}

template<typename C, typename T> void push_front(C& c, const T& v) {c.push_front(v);}
template<typename T> void push_front(std::vector<T>& c, const T& v) {c.insert(c.begin(), v);}

template<typename ARR> double TestContainerAddFirst(unsigned times, unsigned size)
{
	Stopwatch timer;
	for(unsigned i=0; i<times; i++)
	{
		ARR arr;
		for(unsigned j=0; j<size; j++)
			push_front(arr, j);
	}
	return timer.ElapsedSeconds();
}

template<typename ARR> double TestArrayAddFirstReserve(unsigned times, unsigned size)
{
	Stopwatch timer;
	for(unsigned i=0; i<times; i++)
	{
		ARR arr;
		arr.reserve(size);
		for(unsigned j=0; j<size; j++)
			push_front(arr, j);
	}
	return timer.ElapsedSeconds();
}


template<typename C> void pop_front(C& c) {c.pop_front();}
template<typename T> void pop_front(std::vector<T>& c) {c.erase(c.begin());}

template<typename ARR> double TestContainerFirstElementRemove(unsigned size)
{
	ARR arr;
	Reserve(arr, size);
	for(unsigned i=0; i<size; i++)
		arr.push_back(i);

	Stopwatch timer;
	for(unsigned i=0; i<size; i++)
		pop_front(arr);
	return timer.ElapsedSeconds();
}

template<typename ARR> double TestContainerLastElementRemove(unsigned size)
{
	ARR arr;
	Reserve(arr, size);
	for(unsigned i=0; i<size; i++) 
		arr.push_back(i);

	Stopwatch timer;
	for(unsigned i=0; i<size; i++)
		arr.pop_back();
	return timer.ElapsedSeconds();
}


template<typename ARR> double TestContainerMiddleElementRemove(unsigned size)
{
	ARR arr;
	Reserve(arr, size);
	for(unsigned i=0; i<size; i++)
		arr.push_back(i);

	Stopwatch timer;
	for(unsigned i=size; i>0; i--)
		arr.erase(arr.begin()+index_t((i-1u)/2u));
	return timer.ElapsedSeconds();
}

template<typename ARR> double TestContainerRemoveRange(unsigned size, unsigned elementsToRemove)
{
	ARR arr;
	Reserve(arr, size);
	for(unsigned i=0; i<size; i++)
		arr.push_back(i);

	Stopwatch timer;
	for(unsigned i = size - elementsToRemove; i >= elementsToRemove; i -= elementsToRemove)
		arr.erase(arr.begin()+index_t(i/2u - elementsToRemove/2u + 1u), arr.begin() + index_t(i/2u + elementsToRemove/2u));
	return timer.ElapsedSeconds();
}

template<typename ARR> double TestContainerCopying(unsigned times, unsigned size)
{
	ARR arr;
	Reserve(arr, size);
	for(unsigned i=0; i<size; i++)
		arr.push_back(i);

	Stopwatch timer;
	for(unsigned i = 0; i<times; i++)
	{
		ARR arr2(arr);
	}
	return timer.ElapsedSeconds();
}


template<typename ARR> static ARR arrret(unsigned size)
{
	ARR arr;
	arr.resize(size);
	return arr;
}

template<typename ARR> double TestContainerReturn(unsigned times, unsigned size)
{
	Stopwatch timer;
	for(unsigned i=0; i<times; i++)
		ARR arr = arrret<ARR>(size);
	return timer.ElapsedSeconds();
}

template<typename ARR> double TestArraySmallReserve(unsigned times, unsigned size)
{
	Stopwatch timer;
	for(unsigned i=0; i<times; i++)
	{
		ARR arr;
		arr.reserve(size);
	}
	return timer.ElapsedSeconds();
}


static const String g_str="Строка, добавляемая в контейнер много раз.";

template<typename ARR> double TestContainerStringAddLast(unsigned times, unsigned elements)
{
	Stopwatch timer;
	for(unsigned i=0; i<times; i++)
	{
		ARR arr;
		for(unsigned j=0; j<elements; j++)
			arr.push_back(g_str);
	}
	return timer.ElapsedSeconds();
}

template<typename ARR> double TestContainerStringAddFirst(unsigned times, unsigned elements)
{
	Stopwatch timer;
	for(unsigned i=0; i<times; i++)
	{
		ARR arr;
		for(unsigned j=0; j<elements; j++)
			push_front(arr, g_str);
	}
	return timer.ElapsedSeconds();
}



template<typename ARR> double TestContainerFirstStringRemove(unsigned size)
{
	ARR arr;
	Reserve(arr, size);
	for(unsigned i=0; i<size; i++)
		arr.push_back(g_str);

	Stopwatch timer;
	for(unsigned i=0; i<size; i++)
		pop_front(arr);
	return timer.ElapsedSeconds();
}

template<typename ARR> double TestContainerLastStringRemove(unsigned size)
{
	ARR arr;
	Reserve(arr, size);
	for(unsigned i=0; i<size; i++)
		arr.push_back(g_str);

	Stopwatch timer;
	for(unsigned i=0; i<size; i++)
		arr.pop_back();
	return timer.ElapsedSeconds();
}

template<typename ARR> double TestContainerMiddleStringRemove(unsigned size)
{
	ARR arr;
	Reserve(arr, size);
	for(unsigned i=0; i<size; i++)
		arr.push_back(g_str);

	Stopwatch timer;
	for(unsigned i=size; i>0; i--)
		arr.erase(arr.begin()+index_t((i-1u)/2u));
	return timer.ElapsedSeconds();
}

template<typename ARR> double TestContainerStringRemoveRange(size_t size, size_t elementsToRemove)
{
	ARR arr;
	Reserve(arr, size);
	for(size_t i=0; i<size; i++)
		arr.push_back(g_str);

	Stopwatch timer;
	for(size_t i=size-elementsToRemove/2-1; i>elementsToRemove/2; i-=elementsToRemove)
		arr.erase(arr.begin() + index_t(i/2-elementsToRemove/2+1), arr.begin() + index_t(i/2+elementsToRemove/2));
	return timer.ElapsedSeconds();
}


template<typename ARR> double TestContainerStringCopying(unsigned times, unsigned size)
{
	ARR arr;
	Reserve(arr, size);
	for(unsigned i=0; i<size; i++)
		arr.push_back(g_str);

	Stopwatch timer;
	for(unsigned i=0; i<times; i++)
		ARR arr2(arr);
	return timer.ElapsedSeconds();
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
		PrintPerformanceResults(output, "В конец 10000000 элементов",
			{"std::vector", "std::deque", "std::list", "Array", "BList"},
			{
				TestContainerAddLast<std::vector<unsigned>>(1, 10000000),
				TestContainerAddLast<std::deque<unsigned>>(1, 10000000),
				TestContainerAddLast<std::list<unsigned>>(1, 10000000)
			},
			{
				TestContainerAddLast<Array<unsigned>>(1, 10000000),
				TestContainerAddLast<BList<unsigned>>(1, 10000000)
			});

		PrintPerformanceResults(output, "В начало 100000 элементов",
			{"std::vector", "std::deque", "std::list", "Array", "BList"},
			{
				TestContainerAddFirst<std::vector<unsigned>>(1, 100000),
				TestContainerAddFirst<std::deque<unsigned>>(1, 100000),
				TestContainerAddFirst<std::list<unsigned>>(1, 100000)
			},
			{
				TestContainerAddFirst<Array<unsigned>>(1, 100000),
				TestContainerAddFirst<BList<unsigned>>(1, 100000)
			});

		PrintPerformanceResults(output, "В конец 10000000 элементов с reserve",
			comparedArrays,
			{
				TestArrayAddLastReserve<std::vector<unsigned>>(1, 10000000)
			},
			{
				TestArrayAddLastReserve<Array<unsigned>>(1, 10000000)
			});

		PrintPerformanceResults(output, "В начало 100000 элементов с reserve",
			comparedArrays,
			{
				TestArrayAddFirstReserve<std::vector<unsigned>>(1, 100000)
			},
			{
				TestArrayAddFirstReserve<Array<unsigned>>(1, 100000)
			});
	}

	if(TestGroup gr{"Удаление int из контейнера"})
	{
		PrintPerformanceResults(output, "С конца 10000000 раз",
			comparedContainers,
			{
				TestContainerLastElementRemove<std::vector<unsigned>>(10000000),
				TestContainerLastElementRemove<std::deque<unsigned>>(10000000),
				TestContainerLastElementRemove<std::list<unsigned>>(10000000)
			},
			{
				TestContainerLastElementRemove<Array<unsigned>>(10000000),
				TestContainerLastElementRemove<BList<unsigned>>(10000000)
			});

		PrintPerformanceResults(output, "Из начала 100000 раз",
			comparedContainers,
			{
				TestContainerFirstElementRemove<std::vector<unsigned>>(100000),
				TestContainerFirstElementRemove<std::deque<unsigned>>(100000),
				TestContainerFirstElementRemove<std::list<unsigned>>(100000)
			},
			{
				TestContainerFirstElementRemove<Array<unsigned>>(100000),
				TestContainerFirstElementRemove<BList<unsigned>>(100000)
			});

		PrintPerformanceResults(output, "Из середины 100000 раз",
			{"std::vector", "std::deque", "Array"},
			{
				TestContainerMiddleElementRemove<std::vector<unsigned>>(100000),
				TestContainerMiddleElementRemove<std::deque<unsigned>>(100000)
			},
			{
				TestContainerMiddleElementRemove<Array<unsigned>>(100000)
			});

		PrintPerformanceResults(output, "Из середины по 20 элементов (всего 100000 элементов)",
			{"std::vector", "std::deque", "Array"},
			{
				TestContainerRemoveRange<std::vector<unsigned>>(100000, 20),
				TestContainerRemoveRange<std::deque<unsigned>>(100000, 20)
			},
			{
				TestContainerRemoveRange<Array<unsigned>>(100000, 20)
			});
	}

	if(TestGroup gr{"Копирование контейнеров int"})
	{
		PrintPerformanceResults(output, "Размер копируемых контейнеров 10000000",
			comparedContainers,
			{
				TestContainerCopying<std::vector<unsigned>>(10, 10000000),
				TestContainerCopying<std::deque<unsigned>>(10, 10000000),
				TestContainerCopying<std::list<unsigned>>(10, 10000000)},
			{
				TestContainerCopying<Array<unsigned>>(10, 10000000),
				TestContainerCopying<BList<unsigned>>(10, 10000000)
			});
	}

	if(TestGroup gr{"Возврат контейнеров из функции"})
	{
		PrintPerformanceResults(output, "Элементов в контейнере: 10 (10000000 раз)",
			comparedContainers,
			{
				TestContainerReturn<std::vector<unsigned>>(10000000, 10),
				TestContainerReturn<std::deque<unsigned>>(10000000, 10),
				TestContainerReturn<std::list<unsigned>>(10000000, 10)},
			{
				TestContainerReturn<Array<unsigned>>(10000000, 10),
				TestContainerReturn<BList<unsigned>>(10000000, 10)
			});

		PrintPerformanceResults(output, "Элементов в контейнере: 100 (1000000 раз)",
			comparedContainers,
			{
				TestContainerReturn<std::vector<unsigned>>(1000000, 100),
				TestContainerReturn<std::deque<unsigned>>(1000000, 100),
				TestContainerReturn<std::list<unsigned>>(1000000, 100)},
			{
				TestContainerReturn<Array<unsigned>>(1000000, 100),
				TestContainerReturn<BList<unsigned>>(1000000, 100)
			});

		PrintPerformanceResults(output, "Элементов в контейнере: 1000 (100000 раз)",
			comparedContainers,
			{
				TestContainerReturn<std::vector<unsigned>>(100000, 1000),
				TestContainerReturn<std::deque<unsigned>>(100000, 1000),
				TestContainerReturn<std::list<unsigned>>(100000, 1000)
			},
			{
				TestContainerReturn<Array<unsigned>>(100000, 1000),
				TestContainerReturn<BList<unsigned>>(100000, 1000)
			});
	}

	if(TestGroup gr{"Выделение памяти для небольших массивов"})
	{
		PrintPerformanceResults(output, "Выделение памяти для 50 элементов массива (1000000 раз)",
			comparedArrays,
			{
				TestArraySmallReserve<std::vector<unsigned>>(1000000, 50)
			},
			{
				TestArraySmallReserve<Array<unsigned>>(1000000, 50)
			});
	}

	if(TestGroup gr{"Добавление String в контейнер"})
	{
		PrintPerformanceResults(output, "В конец (1000000 элементов)",
			comparedContainers,
			{
				TestContainerStringAddLast<std::vector<String>>(1, 1000000),
				TestContainerStringAddLast<std::deque<String>>(1, 1000000),
				TestContainerStringAddLast<std::list<String>>(1, 1000000)
			},
			{
				TestContainerStringAddLast<Array<String>>(1, 1000000),
				TestContainerStringAddLast<BList<String>>(1, 1000000)
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
