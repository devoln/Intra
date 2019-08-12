#pragma once

#include "Core/Range/Concepts.h"

INTRA_CORE_RANGE_BEGIN
/** Range that counts all elements that are put into it.
  Used for example to count result string length before conversion to it to avoid reallocation.
*/

template<typename T> struct CountRange
{
	enum: bool {RangeIsInfinite = true};

	CountRange() = default;
	constexpr forceinline CountRange(null_t) {}
	constexpr forceinline CountRange(size_t counter): Counter(counter) {}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return false;}
	INTRA_NODISCARD constexpr forceinline const T& First() const {static const T empty; return empty;}
	INTRA_CONSTEXPR2 forceinline void PopFirst() {Counter++;}

	INTRA_CONSTEXPR2 forceinline void Put(const T&) {Counter++;}

	constexpr forceinline bool operator==(const CountRange& rhs) const {return Counter == rhs.Counter;}

	size_t Counter = 0;
};
INTRA_CORE_RANGE_END
