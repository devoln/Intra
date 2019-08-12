#pragma once

#include "Core/Assert.h"

INTRA_CORE_RANGE_BEGIN
template<typename T> struct NullRange
{
	INTRA_NODISCARD constexpr forceinline T* begin() const {return null;}
	INTRA_NODISCARD constexpr forceinline T* end() const {return null;}

	NullRange() = default;
	constexpr forceinline NullRange(null_t) {}
	INTRA_NODISCARD constexpr forceinline bool Empty() const {return true;}
	INTRA_NODISCARD constexpr forceinline index_t Length() const {return 0;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T First() const {INTRA_DEBUG_ASSERT(false); return T();}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T Last() const {INTRA_DEBUG_ASSERT(false); return T();}
	INTRA_CONSTEXPR2 forceinline void PopFirst() {INTRA_DEBUG_ASSERT(false);}
	INTRA_CONSTEXPR2 forceinline void PopLast() {INTRA_DEBUG_ASSERT(false);}
	INTRA_CONSTEXPR2 forceinline T operator[](size_t) {INTRA_DEBUG_ASSERT(false); return T();}
	
	INTRA_NODISCARD INTRA_CONSTEXPR2 NullRange operator()(size_t startIndex, size_t endIndex) const
	{
		(void)startIndex; (void)endIndex;
		INTRA_DEBUG_ASSERT(startIndex == 0 && endIndex == 0);
		return NullRange();
	}

	INTRA_CONSTEXPR2 forceinline void Put(const T&) {}
};
INTRA_CORE_RANGE_END
