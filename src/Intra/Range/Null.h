#pragma once

#include "Intra/Assert.h"
#include "Intra/TypeSafe.h"

INTRA_BEGIN
template<typename T> struct REmptyRange
{
	[[nodiscard]] constexpr bool Empty() const noexcept {return true;}
	[[nodiscard]] constexpr index_t Length() const noexcept {return 0;}
	[[nodiscard]] constexpr T First() const {INTRA_PRECONDITION(false); return T();}
	[[nodiscard]] constexpr T Last() const {INTRA_PRECONDITION(false); return T();}
	constexpr void PopFirst() const {INTRA_PRECONDITION(false);}
	constexpr void PopLast() const {INTRA_PRECONDITION(false);}
	constexpr T operator[](size_t) const {INTRA_PRECONDITION(false); return T();}
	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize maxElementsToPop) const noexcept
	{
		(void)maxElementsToPop;
		return 0;
	}
	constexpr T* Data() const noexcept {return null;}
};
template<typename T> constexpr REmptyRange<T> EmptyRange;
constexpr struct {template<typename T> constexpr void Put(T&&) const {}} NullSink;
INTRA_END
