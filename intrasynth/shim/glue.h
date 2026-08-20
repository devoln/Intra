#pragma once

#include <Cpp/Fundamental.h>
#include <Cpp/Warnings.h>
#include <Cpp/Features.h>
#include <Meta/Type.h>
#include <Utils/Span.h>
#include <Math/Math.h>
#include <Math/SineRange.h>

#ifndef INTRA_BEGIN
#define INTRA_BEGIN
#define INTRA_END
#endif

#ifndef INTRA_FORCEINLINE
#define INTRA_FORCEINLINE forceinline
#endif

namespace Intra {
using int8 = sbyte;
using uint8 = byte;
using int16 = short;
using uint16 = ushort;
using int32 = int;
using uint32 = uint;
using int64 = long64;
using uint64 = ulong64;

using Cpp::Move;
using Cpp::Forward;

using namespace Math;

namespace Math {
constexpr const double Sqrt2 = 1.4142135623730950488;
}
}

template<bool Cond, typename T = void> using Requires = Intra::Meta::EnableIf<Cond, T>;

using namespace Intra;
using namespace Intra::Range;
