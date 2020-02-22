#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include "PerfTestRandom.h"

#include "System/Stopwatch.h"
#include "Test/PerfSummary.h"
#include "Core/Compatibility.h"
#include "Random/FastUniform.h"


INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <random>
#include <cstdlib>
INTRA_WARNING_POP

using namespace Intra;

float g_A = 0;
int g_B = 0;

void RunRandomPerfTests(IO::FormattedWriter& logger)
{
	std::random_device r;
	std::mt19937 mt19937(r());

	Random::FastUniform<float> frandom(3787847832u);

	Stopwatch tim;

	for(int i=0; i<100000000; i++) g_A += float(mt19937())/float(mt19937.max());
	double time2 = tim.GetElapsedSecondsAndReset();

	for(int i=0; i<100000000; i++) g_A += float(rand())/32767.0f;
	double time1 = tim.GetElapsedSecondsAndReset();

	for(int i=0; i<100000000; i++) g_A += frandom();
	double time3 = tim.GetElapsedSecondsAndReset();


	PrintPerformanceResults(logger, "float in range [0.0; 1.0] 100000000 times",
		{"rand()/32767.0f", "mt19937()/(float)mt19937.max()", "Random<float>()"},
		{time1, time2},
		{time3});
}

INTRA_WARNING_POP
