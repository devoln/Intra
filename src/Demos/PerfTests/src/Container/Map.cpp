#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif



#include "Map.h"

#include "Intra/Compatibility.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifndef INTRA_STL_INTERFACE
#define INTRA_STL_INTERFACE
#endif


#include "Test/PerfSummary.h"
#include "Test/TestGroup.h"
#include "Test/TestData.h"
#include "IntraX/System/Stopwatch.h"
#include "IntraX/IO/FormattedWriter.h"
#include "IntraX/IO/Std.h"

INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <map>
#include <unordered_map>
INTRA_WARNING_POP

#define INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY

#include "IntraX/Container/Associative/LinearMap.h"
#include "IntraX/Container/Associative/HashMap.h"

using namespace Intra;
using namespace IO;

template<typename MAP> double TestMapPopulation(unsigned times, unsigned size)
{
	Stopwatch timer;
	for(unsigned i=0; i<times; i++)
	{
		MAP map;
		PopulateMapRandom(map, size);
	}
	return timer.ElapsedSeconds();
}

template<typename K, typename V> V& GetPairSecondValue(std::pair<K,V>& p) {return p.second;}
template<typename K, typename V> V& GetPairSecondValue(Pair<K,V>& p) {return p.second;}
template<typename K, typename V> V& GetPairSecondValue(KeyValuePair<K,V>& p) {return p.Value;}

template<typename MAP> double TestMapIterationSumValues(unsigned times, unsigned size)
{
	MAP map;
	PopulateMapRandom(map, size);
	Stopwatch timer;
	typename MAP::mapped_type result = typename MAP::mapped_type();
	for(unsigned i=0; i<times; i++)
	{
		for(auto&& element: map)
			result += GetPairSecondValue(element);
	}
	const double time = timer.ElapsedSeconds();
	srand(ToHash(result));
	return time;
}

template<typename K, typename V> double TestOrderedMapIterationSumValues(unsigned times, unsigned size)
{
	HashMap<K, V> map;
	PopulateMapRandom(map, size);
	Stopwatch timer;
	V result = V();
	for(unsigned i=0; i<times; i++)
	{
		map.SortByKey();
		for(auto&& element: map) result += GetPairSecondValue(element);
	}
	const double time = timer.ElapsedSeconds();
	srand(ToHash(result));
	return time;
}

template<typename MAP> double TestMapSuccessfulSearching(unsigned times, unsigned size)
{
	MAP map;
	PopulateMapRandom(map, size);
	auto keys = GetRandomValueArray<typename MAP::key_type>(size);
	Stopwatch timer;
	typename MAP::mapped_type result = typename MAP::mapped_type();
	for(unsigned i=0; i<times; i++)
		for(unsigned j=0; j<size; j++)
			result += map[keys[j]];
	const double time = timer.ElapsedSeconds();
	srand(ToHash(result));
	return time;
}

double TestHashMapSuccessfulSearching(unsigned times, unsigned size)
{
	HashMap<String, unsigned> map;
	PopulateMapRandom(map, size);
	auto keys = GetRandomValueArray<String>(size);
	Stopwatch timer;
	unsigned result = 0;
	for(unsigned i=0; i<times; i++)
		for(unsigned j=0; j<size; j++)
			result += map[keys[j]];
	const double time = timer.ElapsedSeconds();
	srand(ToHash(result));

	
	size_t bucketLoads[20];
	auto stats = map.GetStats(bucketLoads);
	Std.PrintLine("num buckets: ", stats.NumBuckets);
	Std.PrintLine("free bucket count: ", stats.FreeBucketCount);
	Std.PrintLine("max bucket load: ", stats.MaxBucketLoad);
	Std.PrintLine("average bucket load: ", stats.AverageBucketLoad);
	Std.PrintLine(Take(bucketLoads, stats.MaxBucketLoad+1));

	return time;
}

template<typename MAP> double TestMapUnsuccessfulSearching(unsigned times, unsigned size)
{
	MAP map;
	PopulateMapRandom(map, size);
	auto keys = GetRandomValueArray<typename MAP::key_type>(size*2);
	keys = keys.Drop(keys.Length()/2);
	Stopwatch timer;
	unsigned result = 0;
	for(unsigned i=0; i<times; i++)
		for(unsigned j=0; j<size; j++)
			result += (map.find(keys[j])!=map.end());
	const double time = timer.ElapsedSeconds();
	srand(result);
	return time;
}


void RunMapPerfTests(FormattedWriter& output)
{
	static const StringView comparedContainers[] = {"std::map", "std::unordered_map", "LinearMap", "HashMap"};

	if(TestGroup gr{"Заполнение случайными ключами unsigned и значениями unsigned"})
	{
		for(unsigned count=1; count<=1000000; count*=10)
		{
			unsigned times = 1000000u/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapPopulation<std::map<unsigned, unsigned>>(times, count),
					TestMapPopulation<std::unordered_map<unsigned, unsigned>>(times, count)
				},
				{
					TestMapPopulation<LinearMap<unsigned, unsigned>>(times, count),
					TestMapPopulation<HashMap<unsigned, unsigned>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Заполнение случайными ключами String и значениями unsigned"})
	{
		for(unsigned count=1; count<=100000; count*=10)
		{
			unsigned times = 100000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapPopulation<std::map<String, unsigned>>(times, count),
					TestMapPopulation<std::unordered_map<String, unsigned, Hash::HasherObject>>(times, count)
				},
				{
					TestMapPopulation<LinearMap<String, unsigned>>(times, count),
					TestMapPopulation<HashMap<String, unsigned>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Заполнение случайными ключами Big<64> и значениями unsigned"})
	{
		for(unsigned count=1; count<=100000; count*=10)
		{
			unsigned times = 100000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapPopulation<std::map<Big<64>, unsigned>>(times, count),
					TestMapPopulation<std::unordered_map<Big<64>, unsigned, Hash::HasherObject>>(times, count)
				},
				{
					TestMapPopulation<LinearMap<Big<64>, unsigned>>(times, count),
					TestMapPopulation<HashMap<Big<64>, unsigned>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Итерация по контейнеру с ключом unsigned и значением unsigned с вычислением суммы всех значений"})
	{
		for(unsigned count=1; count<=1000000; count*=10)
		{
			unsigned times = 10000000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapIterationSumValues<std::map<unsigned, unsigned>>(times, count),
					TestMapIterationSumValues<std::unordered_map<unsigned, unsigned>>(times, count)
				},
				{
					TestMapIterationSumValues<LinearMap<unsigned, unsigned>>(times, count),
					TestMapIterationSumValues<HashMap<unsigned, unsigned>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Итерация по контейнеру с ключом String и значением unsigned с вычислением суммы всех значений"})
	{
		for(unsigned count=1; count<=100000; count*=10)
		{
			unsigned times = 10000000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapIterationSumValues<std::map<String, unsigned>>(times, count),
					TestMapIterationSumValues<std::unordered_map<String, unsigned, Hash::HasherObject>>(times, count)
				},
				{
					TestMapIterationSumValues<LinearMap<String, unsigned>>(times, count),
					TestMapIterationSumValues<HashMap<String, unsigned>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Итерация по контейнеру с ключом Big<64> и значением unsigned с вычислением суммы всех значений"})
	{
		for(unsigned count=1; count<=100000; count*=10)
		{
			unsigned times = 10000000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapIterationSumValues<std::map<Big<64>, unsigned>>(times, count),
					TestMapIterationSumValues<std::unordered_map<Big<64>, unsigned, Hash::HasherObject>>(times, count)
				},
				{
					TestMapIterationSumValues<LinearMap<Big<64>, unsigned>>(times, count),
					TestMapIterationSumValues<HashMap<Big<64>, unsigned>>(times, count)
				});
		}
	}



	if(TestGroup gr{"Упорядоченная итерация по контейнеру с ключом unsigned и значением unsigned с вычислением суммы всех значений"})
	{
		for(unsigned count=1; count<=1000000; count*=10)
		{
			unsigned times = 10000000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				{"std::map", "HashMap"},
				{
					TestMapIterationSumValues<std::map<unsigned, unsigned>>(times, count)
				},
				{
					TestOrderedMapIterationSumValues<unsigned, unsigned>(times, count)
				});
		}
	}

	if(TestGroup gr{"Упорядоченная итерация по контейнеру с ключом String и значением unsigned с вычислением суммы всех значений"})
	{
		for(unsigned count=1; count<=100000; count*=10)
		{
			unsigned times = 10000000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				{"std::map", "HashMap"},
				{
					TestMapIterationSumValues<std::map<String, unsigned>>(times, count)
				},
				{
					TestOrderedMapIterationSumValues<String, unsigned>(times, count)
				});
		}
	}

	if(TestGroup gr{"Упорядоченная итерация по контейнеру с ключом Big<64> и значением unsigned с вычислением суммы всех значений"})
	{
		for(unsigned count=1; count<=100000; count*=10)
		{
			unsigned times = 10000000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				{"std::map", "HashMap"},
				{
					TestMapIterationSumValues<std::map<Big<64>, unsigned>>(times, count)
				},
				{
					TestOrderedMapIterationSumValues<Big<64>, unsigned>(times, count)
				});
		}
	}



	if(TestGroup gr{"Поиск существующих в контейнере unsigned элементов по ключу unsigned с вычислением суммы всех найденных значений"})
	{
		for(unsigned count=1; count<=1000000; count*=10)
		{
			unsigned times = 1000000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapSuccessfulSearching<std::map<unsigned, unsigned>>(times, count),
					TestMapSuccessfulSearching<std::unordered_map<unsigned, unsigned>>(times, count)
				},
				{
					TestMapSuccessfulSearching<LinearMap<unsigned, unsigned>>(times, count),
					TestMapSuccessfulSearching<HashMap<unsigned, unsigned>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Поиск существующих в контейнере unsigned элементов с ключом String с вычислением суммы всех найденных значений"})
	{
		for(unsigned count=1; count<=100000; count*=10)
		{
			unsigned times = 100000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapSuccessfulSearching<std::map<String, unsigned>>(times, count),
					TestMapSuccessfulSearching<std::unordered_map<String, unsigned, Hash::HasherObject>>(times, count)
				},
				{
					TestMapSuccessfulSearching<LinearMap<String, unsigned>>(times, count),
					TestMapSuccessfulSearching<HashMap<String, unsigned>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Поиск существующих в контейнере unsigned элементов по ключу Big<64> с вычислением суммы всех найденных значений"})
	{
		for(unsigned count=1; count<=100000; count*=10)
		{
			unsigned times = 100000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapSuccessfulSearching<std::map<Big<64>, unsigned>>(times, count),
					TestMapSuccessfulSearching<std::unordered_map<Big<64>, unsigned, Hash::HasherObject>>(times, count)
				},
				{
					TestMapSuccessfulSearching<LinearMap<Big<64>, unsigned>>(times, count),
					TestMapSuccessfulSearching<HashMap<Big<64>, unsigned>>(times, count)
				});
		}
	}


	if(TestGroup gr{"Поиск несуществующих в контейнере unsigned элементов по ключу unsigned"})
	{
		for(unsigned count=1; count<=1000000; count*=10)
		{
			unsigned times = 1000000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapUnsuccessfulSearching<std::map<unsigned, unsigned>>(times, count),
					TestMapUnsuccessfulSearching<std::unordered_map<unsigned, unsigned>>(times, count)
				},
				{
					TestMapUnsuccessfulSearching<LinearMap<unsigned, unsigned>>(times, count),
					TestMapUnsuccessfulSearching<HashMap<unsigned, unsigned>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Поиск несуществующих в контейнере unsigned элементов по ключу String"})
	{
		for(unsigned count=1; count<=100000; count*=10)
		{
			unsigned times = 100000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapUnsuccessfulSearching<std::map<String, unsigned>>(times, count),
					TestMapUnsuccessfulSearching<std::unordered_map<String, unsigned, Hash::HasherObject>>(times, count)
				},
				{
					TestMapUnsuccessfulSearching<LinearMap<String, unsigned>>(times, count),
					TestMapUnsuccessfulSearching<HashMap<String, unsigned>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Поиск несуществующих в контейнере с ключом Big<64> и значением unsigned"})
	{
		for(unsigned count=1; count<=100000; count*=10)
		{
			unsigned times = 100000/count;
			PrintPerformanceResults(output, String() << count << " элементов, " << times << " раз",
				comparedContainers,
				{
					TestMapUnsuccessfulSearching<std::map<Big<64>, unsigned>>(times, count),
					TestMapUnsuccessfulSearching<std::unordered_map<Big<64>, unsigned, Hash::HasherObject>>(times, count)
				},
				{
					TestMapUnsuccessfulSearching<LinearMap<Big<64>, unsigned>>(times, count),
					TestMapUnsuccessfulSearching<HashMap<Big<64>, unsigned>>(times, count)
				});
		}
	}
}

INTRA_WARNING_POP
