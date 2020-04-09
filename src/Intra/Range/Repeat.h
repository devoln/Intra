#pragma once

#include "Intra/Range/Concepts.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED
template<typename T> struct Repeat
{
	static constexpr bool IsAnyInstanceInfinite = true;
	T Value;
	Repeat() = default;
	constexpr Repeat(T val): Value(Move(val)) {}
	[[nodiscard]] constexpr bool Empty() const noexcept {return false;}
	[[nodiscard]] constexpr const T& First() const noexcept {return Value;}
	constexpr void PopFirst() noexcept {}
	constexpr index_t PopFirstCount(ClampedSize numElementsToPop) noexcept {return index_t(numElementsToPop);}
	[[nodiscard]] constexpr const T& operator[](Index) const noexcept {return Value;}
};
INTRA_END
