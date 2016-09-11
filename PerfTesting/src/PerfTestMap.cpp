#include "PerfTestMap.h"

#ifndef INTRA_STL_INTERFACE
#define INTRA_STL_INTERFACE
#endif

#include "Test/PerformanceTest.h"
#include "IO/LogSystem.h"
#include "Core/Time.h"

#include <map>
#include <unordered_map>

#include "Containers/LinearMap.h"
#include "Containers/HashMap.h"

using namespace Intra;

template<typename MAP> double TestMapPopulation(uint times, uint size)
{
	Timer timer;
	for(uint i=0; i<times; i++)
	{
		MAP map;
		PopulateMapRandom(map, size);
	}
	return timer.GetTime();
}

template<typename K, typename V> V& GetPairSecondValue(std::pair<K,V>& p) {return p.second;}
template<typename K, typename V> V& GetPairSecondValue(core::pair<K,V>& p) {return p.second;}
template<typename K, typename V> V& GetPairSecondValue(KeyValuePair<K,V>& p) {return p.Value;}

template<typename MAP> double TestMapIterationSumValues(uint times, uint size)
{
	MAP map;
	PopulateMapRandom(map, size);
	Timer timer;
	typename MAP::mapped_type result = typename MAP::mapped_type();
	for(uint i=0; i<times; i++)
	{
		for(auto&& element: map) result += GetPairSecondValue(element);
	}
	double time = timer.GetTime();
	srand(ToHash(result));
	return time;
}

template<typename K, typename V> double TestOrderedMapIterationSumValues(uint times, uint size)
{
	HashMap<K, V> map;
	PopulateMapRandom(map, size);
	Timer timer;
	map.SortByKey();
	V result = V();
	for(uint i=0; i<times; i++)
	{
		for(auto&& element: map) result += GetPairSecondValue(element);
	}
	double time = timer.GetTime();
	srand(ToHash(result));
	return time;
}

template<typename MAP> double TestMapSuccessfulSearching(uint times, uint size)
{
	MAP map;
	PopulateMapRandom(map, size);
	auto keys = GetRandomValueArray<typename MAP::key_type>(size);
	Timer timer;
	typename MAP::mapped_type result = typename MAP::mapped_type();
	for(uint i=0; i<times; i++)
		for(uint j=0; j<size; j++)
			result += map[keys[j]];
	double time = timer.GetTime();
	srand(ToHash(result));
	return time;
}

double TestHashMapSuccessfulSearching(uint times, uint size)
{
	HashMap<String, uint> map;
	PopulateMapRandom(map, size);
	auto keys = GetRandomValueArray<String>(size);
	Timer timer;
	uint result = 0;
	for(uint i=0; i<times; i++)
		for(uint j=0; j<size; j++)
			result += map[keys[j]];
	double time = timer.GetTime();
	srand(ToHash(result));

	size_t numBuckets, freeBucketCount, maxBucketLoad;
	double averageBucketLoad;
	size_t bucketLoads[20] = {0};
	map.GetStats(&numBuckets, &freeBucketCount, &averageBucketLoad, &maxBucketLoad, bucketLoads, 20);
	IO::Console.PrintLine("num buckets: ", numBuckets, IO::endl,
		"free bucket count: ", freeBucketCount, IO::endl,
		"max bucket load: ", maxBucketLoad, IO::endl,
		"average bucket load: ", averageBucketLoad, IO::endl,
		AsRange(bucketLoads).Take(maxBucketLoad+1));

	return time;
}

template<typename MAP> double TestMapUnsuccessfulSearching(uint times, uint size)
{
	MAP map;
	PopulateMapRandom(map, size);
	auto keys = GetRandomValueArray<typename MAP::key_type>(size*2)($/2, $);
	Timer timer;
	uint result = 0;
	for(uint i=0; i<times; i++)
		for(uint j=0; j<size; j++)
			result += (map.find(keys[j])!=map.end());
	double time = timer.GetTime();
	srand(result);
	return time;
}


void RunMapPerfTests(IO::Logger& logger)
{
	static const StringView comparedContainers[] = {"std::map", "std::unordered_map", "LinearMap", "HashMap"};

	if(TestGroup gr{logger, "Заполнение случайными ключами uint и значениями uint"})
	{
		for(int count=1; count<=1000000; count*=10)
		{
			uint times = 1000000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
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

	if(TestGroup gr{logger, "Заполнение случайными ключами String и значениями uint"})
	{
		for(int count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapPopulation<std::map<String, uint>>(times, count),
					TestMapPopulation<std::unordered_map<String, uint, HasherObject<String>>>(times, count)
				},
				{
					TestMapPopulation<LinearMap<String, uint>>(times, count),
					TestMapPopulation<HashMap<String, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{logger, "Заполнение случайными ключами Big<64> и значениями uint"})
	{
		for(int count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapPopulation<std::map<Big<64>, uint>>(times, count),
					TestMapPopulation<std::unordered_map<Big<64>, uint, HasherObject<Big<64>>>>(times, count)
				},
				{
					TestMapPopulation<LinearMap<Big<64>, uint>>(times, count),
					TestMapPopulation<HashMap<Big<64>, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{logger, "Итерация по контейнеру с ключом uint и значением uint с вычислением суммы всех значений"})
	{
		for(int count=1; count<=1000000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
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

	if(TestGroup gr{logger, "Итерация по контейнеру с ключом String и значением uint с вычислением суммы всех значений"})
	{
		for(int count=1; count<=100000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapIterationSumValues<std::map<String, uint>>(times, count),
					TestMapIterationSumValues<std::unordered_map<String, uint, HasherObject<String>>>(times, count)
				},
				{
					TestMapIterationSumValues<LinearMap<String, uint>>(times, count),
					TestMapIterationSumValues<HashMap<String, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{logger, "Итерация по контейнеру с ключом Big<64> и значением uint с вычислением суммы всех значений"})
	{
		for(int count=1; count<=100000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapIterationSumValues<std::map<Big<64>, uint>>(times, count),
					TestMapIterationSumValues<std::unordered_map<Big<64>, uint, HasherObject<Big<64>>>>(times, count)
				},
				{
					TestMapIterationSumValues<LinearMap<Big<64>, uint>>(times, count),
					TestMapIterationSumValues<HashMap<Big<64>, uint>>(times, count)
				});
		}
	}



	if(TestGroup gr{logger, "Упорядоченная итерация по контейнеру с ключом uint и значением uint с вычислением суммы всех значений"})
	{
		for(int count=1; count<=1000000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
				{"std::map", "HashMap"},
				{
					TestMapIterationSumValues<std::map<uint, uint>>(times, count)
				},
				{
					TestOrderedMapIterationSumValues<uint, uint>(times, count)
				});
		}
	}

	if(TestGroup gr{logger, "Упорядоченная итерация по контейнеру с ключом String и значением uint с вычислением суммы всех значений"})
	{
		for(int count=1; count<=100000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
				{"std::map", "HashMap"},
				{
					TestMapIterationSumValues<std::map<String, uint>>(times, count)
				},
				{
					TestOrderedMapIterationSumValues<String, uint>(times, count)
				});
		}
	}

	if(TestGroup gr{logger, "Упорядоченная итерация по контейнеру с ключом Big<64> и значением uint с вычислением суммы всех значений"})
	{
		for(int count=1; count<=100000; count*=10)
		{
			uint times = 10000000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
				{"std::map", "HashMap"},
				{
					TestMapIterationSumValues<std::map<Big<64>, uint>>(times, count)
				},
				{
					TestOrderedMapIterationSumValues<Big<64>, uint>(times, count)
				});
		}
	}



	if(TestGroup gr{logger, "Поиск существующих в контейнере uint элементов по ключу uint с вычислением суммы всех найденных значений"})
	{
		for(int count=1; count<=1000000; count*=10)
		{
			uint times = 1000000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
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

	if(TestGroup gr{logger, "Поиск существующих в контейнере uint элементов с ключом String с вычислением суммы всех найденных значений"})
	{
		for(int count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapSuccessfulSearching<std::map<String, uint>>(times, count),
					TestMapSuccessfulSearching<std::unordered_map<String, uint, HasherObject<String>>>(times, count)
				},
				{
					TestMapSuccessfulSearching<LinearMap<String, uint>>(times, count),
					TestMapSuccessfulSearching<HashMap<String, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{logger, "Поиск существующих в контейнере uint элементов по ключу Big<64> с вычислением суммы всех найденных значений"})
	{
		for(int count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapSuccessfulSearching<std::map<Big<64>, uint>>(times, count),
					TestMapSuccessfulSearching<std::unordered_map<Big<64>, uint, HasherObject<Big<64>>>>(times, count)
				},
				{
					TestMapSuccessfulSearching<LinearMap<Big<64>, uint>>(times, count),
					TestMapSuccessfulSearching<HashMap<Big<64>, uint>>(times, count)
				});
		}
	}


	if(TestGroup gr{logger, "Поиск несуществующих в контейнере uint элементов по ключу uint"})
	{
		for(int count=1; count<=1000000; count*=10)
		{
			uint times = 1000000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
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

	if(TestGroup gr{logger, "Поиск несуществующих в контейнере uint элементов по ключу String"})
	{
		for(int count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapUnsuccessfulSearching<std::map<String, uint>>(times, count),
					TestMapUnsuccessfulSearching<std::unordered_map<String, uint, HasherObject<String>>>(times, count)
				},
				{
					TestMapUnsuccessfulSearching<LinearMap<String, uint>>(times, count),
					TestMapUnsuccessfulSearching<HashMap<String, uint>>(times, count)
				});
		}
	}

	if(TestGroup gr{logger, "Поиск несуществующих в контейнере с ключом Big<64> и значением uint"})
	{
		for(int count=1; count<=100000; count*=10)
		{
			uint times = 100000/count;
			PrintPerformanceResults(logger, *String::Format()(count)(" элементов, ")(times)(" раз"),
				comparedContainers,
				{
					TestMapUnsuccessfulSearching<std::map<Big<64>, uint>>(times, count),
					TestMapUnsuccessfulSearching<std::unordered_map<Big<64>, uint, HasherObject<Big<64>>>>(times, count)
				},
				{
					TestMapUnsuccessfulSearching<LinearMap<Big<64>, uint>>(times, count),
					TestMapUnsuccessfulSearching<HashMap<Big<64>, uint>>(times, count)
				});
		}
	}
}

