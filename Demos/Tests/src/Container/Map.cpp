#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include "Cpp/Warnings.h"

#include "Map.h"

#include "Cpp/Compatibility.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifndef INTRA_STL_INTERFACE
#define INTRA_STL_INTERFACE
#endif


#include "Test/PerfSummary.h"
#include "Test/TestGroup.h"
#include "Test/TestData.h"
#include "System/Stopwatch.h"
#include "IO/FormattedWriter.h"
#include "IO/Std.h"

INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <map>
#include <unordered_map>
INTRA_WARNING_POP

#include "Container/Associative/LinearMap.h"
#include "Container/Associative/HashMap.h"

using namespace Intra;
using namespace IO;

template<typename MAP> double TestMapPopulation(uint times, uint size)
{
	Stopwatch timer;
	for(uint i=0; i<times; i++)
	{
		MAP map;
		PopulateMapRandom(map, size);
	}
	return timer.ElapsedSeconds();
}

template<typename K, typename V> V& GetPairSecondValue(std::pair<K,V>& p) {return p.second;}
template<typename K, typename V> V& GetPairSecondValue(Meta::Pair<K,V>& p) {return p.second;}
template<typename K, typename V> V& GetPairSecondValue(KeyValuePair<K,V>& p) {return p.Value;}

template<typename MAP> double TestMapIterationSumValues(uint times, uint size)
{
	MAP map;
	PopulateMapRandom(map, size);
	Stopwatch timer;
	typename MAP::mapped_type result = typename MAP::mapped_type();
	for(uint i=0; i<times; i++)
	{
		for(auto&& element: map)
			result += GetPairSecondValue(element);
	}
	const double time = timer.ElapsedSeconds();
	srand(ToHash(result));
	return time;
}

template<typename K, typename V> double TestOrderedMapIterationSumValues(uint times, uint size)
{
	HashMap<K, V> map;
	PopulateMapRandom(map, size);
	Stopwatch timer;
	V result = V();
	for(uint i=0; i<times; i++)
	{
		map.SortByKey();
		for(auto&& element: map) result += GetPairSecondValue(element);
	}
	const double time = timer.ElapsedSeconds();
	srand(ToHash(result));
	return time;
}

template<typename MAP> double TestMapSuccessfulSearching(uint times, uint size)
{
	MAP map;
	PopulateMapRandom(map, size);
	auto keys = GetRandomValueArray<typename MAP::key_type>(size);
	Stopwatch timer;
	typename MAP::mapped_type result = typename MAP::mapped_type();
	for(uint i=0; i<times; i++)
		for(uint j=0; j<size; j++)
			result += map[keys[j]];
	const double time = timer.ElapsedSeconds();
	srand(ToHash(result));
	return time;
}

double TestHashMapSuccessfulSearching(uint times, uint size)
{
	HashMap<String, uint> map;
	PopulateMapRandom(map, size);
	auto keys = GetRandomValueArray<String>(size);
	Stopwatch timer;
	uint result = 0;
	for(uint i=0; i<times; i++)
		for(uint j=0; j<size; j++)
			result += map[keys[j]];
	const double time = timer.ElapsedSeconds();
	srand(ToHash(result));

	
	size_t bucketLoads[20];
	auto stats = map.GetStats(bucketLoads);
	Std.PrintLine("num buckets: ", stats.NumBuckets);
	Std.PrintLine("free bucket count: ", stats.FreeBucketCount);
	Std.PrintLine("max bucket load: ", stats.MaxBucketLoad);
	Std.PrintLine("average bucket load: ", stats.AverageBucketLoad);
	Std.PrintLine(Range::Take(bucketLoads, stats.MaxBucketLoad+1));

	return time;
}

template<typename MAP> double TestMapUnsuccessfulSearching(uint times, uint size)
{
	MAP map;
	PopulateMapRandom(map, size);
	auto keys = GetRandomValueArray<typename MAP::key_type>(size*2);
	keys = keys.Drop(keys.Length()/2);
	Stopwatch timer;
	uint result = 0;
	for(uint i=0; i<times; i++)
		for(uint j=0; j<size; j++)
			result += (map.find(keys[j])!=map.end());
	const double time = timer.ElapsedSeconds();
	srand(result);
	return time;
}


void RunMapPerfTests(FormattedWriter& output)
{
	static const StringView comparedContainers[] = {"std::map", "std::unordered_map", "LinearMap", "HashMap"};

	if(TestGroup gr{"Заполнение случайными ключами uint и значениями uint"})
	{
		for(uint count=1; count<=1000000; count*=10)
		{
			uint times = 1000000u/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapPopulation<std::map<uint, uint>>(times, count),
					TestMapPopulation<std::unordered_map<uint, uint>>(times, count)
				},
				{
					TestMapPopulation<LinearMap<uint, uint>>(times, count),
					TestMapPopulation<HashMap<uint, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Заполнение случайными ключами String и значениями uint"})
	{
		for(uint count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapPopulation<std::map<String, uint>>(times, count),
					TestMapPopulation<std::unordered_map<String, uint, Hash::HasherObject>>(times, count)
				},
				{
					TestMapPopulation<LinearMap<String, uint>>(times, count),
					TestMapPopulation<HashMap<String, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Заполнение случайными ключами Big<64> и значениями uint"})
	{
		for(uint count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapPopulation<std::map<Big<64>, uint>>(times, count),
					TestMapPopulation<std::unordered_map<Big<64>, uint, Hash::HasherObject>>(times, count)
				},
				{
					TestMapPopulation<LinearMap<Big<64>, uint>>(times, count),
					TestMapPopulation<HashMap<Big<64>, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Итерация по контейнеру с ключом uint и значением uint с вычислением суммы всех значений"})
	{
		for(uint count=1; count<=1000000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapIterationSumValues<std::map<uint, uint>>(times, count),
					TestMapIterationSumValues<std::unordered_map<uint, uint>>(times, count)
				},
				{
					TestMapIterationSumValues<LinearMap<uint, uint>>(times, count),
					TestMapIterationSumValues<HashMap<uint, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Итерация по контейнеру с ключом String и значением uint с вычислением суммы всех значений"})
	{
		for(uint count=1; count<=100000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapIterationSumValues<std::map<String, uint>>(times, count),
					TestMapIterationSumValues<std::unordered_map<String, uint, Hash::HasherObject>>(times, count)
				},
				{
					TestMapIterationSumValues<LinearMap<String, uint>>(times, count),
					TestMapIterationSumValues<HashMap<String, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Итерация по контейнеру с ключом Big<64> и значением uint с вычислением суммы всех значений"})
	{
		for(uint count=1; count<=100000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapIterationSumValues<std::map<Big<64>, uint>>(times, count),
					TestMapIterationSumValues<std::unordered_map<Big<64>, uint, Hash::HasherObject>>(times, count)
				},
				{
					TestMapIterationSumValues<LinearMap<Big<64>, uint>>(times, count),
					TestMapIterationSumValues<HashMap<Big<64>, uint>>(times, count)
				});
		}
	}



	if(TestGroup gr{"Упорядоченная итерация по контейнеру с ключом uint и значением uint с вычислением суммы всех значений"})
	{
		for(uint count=1; count<=1000000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				{"std::map", "HashMap"},
				{
					TestMapIterationSumValues<std::map<uint, uint>>(times, count)
				},
				{
					TestOrderedMapIterationSumValues<uint, uint>(times, count)
				});
		}
	}

	if(TestGroup gr{"Упорядоченная итерация по контейнеру с ключом String и значением uint с вычислением суммы всех значений"})
	{
		for(uint count=1; count<=100000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				{"std::map", "HashMap"},
				{
					TestMapIterationSumValues<std::map<String, uint>>(times, count)
				},
				{
					TestOrderedMapIterationSumValues<String, uint>(times, count)
				});
		}
	}

	if(TestGroup gr{"Упорядоченная итерация по контейнеру с ключом Big<64> и значением uint с вычислением суммы всех значений"})
	{
		for(uint count=1; count<=100000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				{"std::map", "HashMap"},
				{
					TestMapIterationSumValues<std::map<Big<64>, uint>>(times, count)
				},
				{
					TestOrderedMapIterationSumValues<Big<64>, uint>(times, count)
				});
		}
	}



	if(TestGroup gr{"Поиск существующих в контейнере uint элементов по ключу uint с вычислением суммы всех найденных значений"})
	{
		for(uint count=1; count<=1000000; count*=10)
		{
			uint times = 1000000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapSuccessfulSearching<std::map<uint, uint>>(times, count),
					TestMapSuccessfulSearching<std::unordered_map<uint, uint>>(times, count)
				},
				{
					TestMapSuccessfulSearching<LinearMap<uint, uint>>(times, count),
					TestMapSuccessfulSearching<HashMap<uint, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Поиск существующих в контейнере uint элементов с ключом String с вычислением суммы всех найденных значений"})
	{
		for(uint count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapSuccessfulSearching<std::map<String, uint>>(times, count),
					TestMapSuccessfulSearching<std::unordered_map<String, uint, Hash::HasherObject>>(times, count)
				},
				{
					TestMapSuccessfulSearching<LinearMap<String, uint>>(times, count),
					TestMapSuccessfulSearching<HashMap<String, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Поиск существующих в контейнере uint элементов по ключу Big<64> с вычислением суммы всех найденных значений"})
	{
		for(uint count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapSuccessfulSearching<std::map<Big<64>, uint>>(times, count),
					TestMapSuccessfulSearching<std::unordered_map<Big<64>, uint, Hash::HasherObject>>(times, count)
				},
				{
					TestMapSuccessfulSearching<LinearMap<Big<64>, uint>>(times, count),
					TestMapSuccessfulSearching<HashMap<Big<64>, uint>>(times, count)
				});
		}
	}


	if(TestGroup gr{"Поиск несуществующих в контейнере uint элементов по ключу uint"})
	{
		for(uint count=1; count<=1000000; count*=10)
		{
			uint times = 1000000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapUnsuccessfulSearching<std::map<uint, uint>>(times, count),
					TestMapUnsuccessfulSearching<std::unordered_map<uint, uint>>(times, count)
				},
				{
					TestMapUnsuccessfulSearching<LinearMap<uint, uint>>(times, count),
					TestMapUnsuccessfulSearching<HashMap<uint, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Поиск несуществующих в контейнере uint элементов по ключу String"})
	{
		for(uint count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapUnsuccessfulSearching<std::map<String, uint>>(times, count),
					TestMapUnsuccessfulSearching<std::unordered_map<String, uint, Hash::HasherObject>>(times, count)
				},
				{
					TestMapUnsuccessfulSearching<LinearMap<String, uint>>(times, count),
					TestMapUnsuccessfulSearching<HashMap<String, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{"Поиск несуществующих в контейнере с ключом Big<64> и значением uint"})
	{
		for(uint count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(output, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapUnsuccessfulSearching<std::map<Big<64>, uint>>(times, count),
					TestMapUnsuccessfulSearching<std::unordered_map<Big<64>, uint, Hash::HasherObject>>(times, count)
				},
				{
					TestMapUnsuccessfulSearching<LinearMap<Big<64>, uint>>(times, count),
					TestMapUnsuccessfulSearching<HashMap<Big<64>, uint>>(times, count)
				});
		}
	}
}

INTRA_WARNING_POP
