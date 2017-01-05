#include "Core/Core.h"
#include "Math/Random.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Math {


Random<sbyte> Random<sbyte>::Global;
Random<byte> Random<byte>::Global;
Random<short> Random<short>::Global;
Random<ushort> Random<ushort>::Global;
Random<int> Random<int>::Global;
Random<uint> Random<uint>::Global;
Random<long64> Random<long64>::Global;
Random<ulong64> Random<ulong64>::Global;

Random<float> Random<float>::Global;
Random<double> Random<double>::Global;

RandomNoise::Data* RandomNoise::data = new RandomNoise::Data;


}}
