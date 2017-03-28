#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"
#include "Math/Random.h"
#include "Platform/Time.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Math {


Random<sbyte> Random<sbyte>::Global((uint(DateTime::StartupTimeBasedSeed())));
Random<byte> Random<byte>::Global((uint(DateTime::StartupTimeBasedSeed())));
Random<short> Random<short>::Global((uint(DateTime::StartupTimeBasedSeed())));
Random<ushort> Random<ushort>::Global((uint(DateTime::StartupTimeBasedSeed())));
Random<int> Random<int>::Global((uint(DateTime::StartupTimeBasedSeed())));
Random<uint> Random<uint>::Global((uint(DateTime::StartupTimeBasedSeed())));
Random<long64> Random<long64>::Global((uint(DateTime::StartupTimeBasedSeed())));
Random<ulong64> Random<ulong64>::Global((uint(DateTime::StartupTimeBasedSeed())));

Random<float> Random<float>::Global((uint(DateTime::StartupTimeBasedSeed())));
Random<double> Random<double>::Global((uint(DateTime::StartupTimeBasedSeed())));

Unique<RandomNoise::Data> RandomNoise::data = new RandomNoise::Data;


}}
