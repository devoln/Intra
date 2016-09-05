#include "PerfTestRandom.h"
#include "Core/Time.h"
#include "Test/PerformanceTest.h"

#include "Math/Random.h"
#include <random>
#include <stdlib.h>

using namespace Intra;

float g_A = 0;
int g_B = 0;

void RunRandomPerfTests(IO::Logger& logger)
{
	std::random_device r;
	std::mt19937 mt19937(r());

	Math::Random<float> frandom(3787847832u);

	Timer tim;

	for(int i=0; i<100000000; i++) g_A += mt19937()/(float)mt19937.max();
	double time2 = tim.GetTimeAndReset();

	for(int i=0; i<100000000; i++) g_A += rand()/32767.0f;
	double time1 = tim.GetTimeAndReset();

	for(int i=0; i<100000000; i++) g_A += frandom();
	double time3 = tim.GetTimeAndReset();


	PrintPerformanceResults(logger, "float в диапазоне [0.0; 1.0] 100000000 раз",
		{"rand()/32767.0f", "mt19937()/(float)mt19937.max()", "Math::Random<float>()"},
		{time1, time2},
		{time3});
}

