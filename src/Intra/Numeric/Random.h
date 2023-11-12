#pragma once

#include <Intra/Range.h>

namespace Intra { INTRA_BEGIN
	template<typename T> struct FastUniform;

// TODO: reinterpret as byte stream
constexpr auto FastUniformU16 = Recurrence([](uint32 a) {return a * 16807;}, 157898685u)|Map([](auto x){return uint16(x >> 16);});

// in range [0; 1]
constexpr auto FastUniformF32 = Recurrence([](uint32 a) {return a * 16807;}, 157898685u)|Map([](auto x){return x * 2.32830645e-10f + 0.5f;});

} INTRA_END
