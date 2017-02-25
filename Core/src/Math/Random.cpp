#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"
#include "Math/Random.h"
#include "Platform/Time.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Math {


Random<sbyte> Random<sbyte>::Global(uint(DateTime::Now().TimeBasedRandomValue()));
Random<byte> Random<byte>::Global(uint(DateTime::Now().TimeBasedRandomValue()));
Random<short> Random<short>::Global(uint(DateTime::Now().TimeBasedRandomValue()));
Random<ushort> Random<ushort>::Global(uint(DateTime::Now().TimeBasedRandomValue()));
Random<int> Random<int>::Global(uint(DateTime::Now().TimeBasedRandomValue()));
Random<uint> Random<uint>::Global(uint(DateTime::Now().TimeBasedRandomValue()));
Random<long64> Random<long64>::Global(uint(DateTime::Now().TimeBasedRandomValue()));
Random<ulong64> Random<ulong64>::Global(uint(DateTime::Now().TimeBasedRandomValue()));

Random<float> Random<float>::Global(uint(DateTime::Now().TimeBasedRandomValue()));
Random<double> Random<double>::Global(uint(DateTime::Now().TimeBasedRandomValue()));

RandomNoise::Data* RandomNoise::data = new RandomNoise::Data;


}}
