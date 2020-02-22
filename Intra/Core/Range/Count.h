#pragma once

#include "Core/Range/Concepts.h"

INTRA_BEGIN
/** Range that counts all elements that are put into it.
  Used for example to count result string length before conversion to it to avoid reallocation.
*/

template<typename T> struct CountRange
{
	enum: bool {RangeIsInfinite = true};

	CountRange() = default;
	constexpr forceinline CountRange(null_t) {}
	constexpr forceinline CountRange(index_t counter): Counter(counter) {}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return false;}
	INTRA_NODISCARD constexpr forceinline T First() const {return T();}
	constexpr forceinline void PopFirst() {Counter++;}

	constexpr forceinline void Put(const T&) {Counter++;}

	constexpr forceinline bool operator==(const CountRange& rhs) const {return Counter == rhs.Counter;}

	index_t Counter = 0;
};
INTRA_END
