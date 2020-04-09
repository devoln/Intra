#pragma once

#include "Intra/Range/Concepts.h"

INTRA_BEGIN
/** Range that counts all elements that are put into it.
  Used for example to count result string length before conversion to it to avoid reallocation.
*/
template<typename T = int, typename CounterT = index_t> struct RCount
{
	static constexpr bool IsAnyInstanceInfinite = true;

	[[nodiscard]] constexpr bool Empty() const noexcept {return false;}
	[[nodiscard]] constexpr T First() const noexcept(noexcept(T())) {return T();}
	constexpr void Put(const T&) noexcept {Counter++;}
	constexpr void PopFirst() noexcept {Counter++;}
	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize elementsToPop) const
	{
		Counter += CounterT(elementsToPop);
		return index_t(elementsToPop);
	}

	CounterT Counter = 0;
};
INTRA_END
