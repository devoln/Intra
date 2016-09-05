#include "Core/Core.h"
#include "Math/Random.h"

namespace Intra { namespace Math {


INTRA_OPTIONAL_THREAD_LOCAL Random<sbyte> Random<sbyte>::Global;
INTRA_OPTIONAL_THREAD_LOCAL Random<byte> Random<byte>::Global;
INTRA_OPTIONAL_THREAD_LOCAL Random<short> Random<short>::Global;
INTRA_OPTIONAL_THREAD_LOCAL Random<ushort> Random<ushort>::Global;
INTRA_OPTIONAL_THREAD_LOCAL Random<int> Random<int>::Global;
INTRA_OPTIONAL_THREAD_LOCAL Random<uint> Random<uint>::Global;
INTRA_OPTIONAL_THREAD_LOCAL Random<long64> Random<long64>::Global;
INTRA_OPTIONAL_THREAD_LOCAL Random<ulong64> Random<ulong64>::Global;

INTRA_OPTIONAL_THREAD_LOCAL Random<float> Random<float>::Global;
INTRA_OPTIONAL_THREAD_LOCAL Random<double> Random<double>::Global;

RandomNoise::Data* RandomNoise::data = new RandomNoise::Data;


}}
