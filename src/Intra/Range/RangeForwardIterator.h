#pragma once

#include "Intra/CRange.h"

INTRA_CORE_RANGE_BEGIN
template<typename R> struct RangeForwardIterator
{
	typedef TRangeValue<R> value_type;
	typedef TRangeValueRef<R> return_value_type;
	typedef return_value_type& reference;
	typedef value_type* pointer;
	typedef index_t difference_type;

	RangeForwardIterator() = default;
	constexpr RangeForwardIterator(decltype(nullptr)) {}
	constexpr RangeForwardIterator(const R& range): Range(range) {}
	INTRA_FORCEINLINE INTRA_CONSTEXPR2 RangeForwardIterator& operator++() {Range.PopFirst(); return *this;}
	[[nodiscard]] INTRA_CONSTEXPR2 INTRA_FORCEINLINE RangeForwardIterator operator++(int) {auto copy = Range; Range.PopFirst(); return copy;}
	[[nodiscard]] constexpr return_value_type operator*() const {return Range.First();}
	[[nodiscard]] constexpr TRemoveReference<return_value_type>* operator->() const {return &Range.First();}

	[[nodiscard]] constexpr bool operator==(const RangeForwardIterator& rhs) const {return Range == rhs.Range;}
	[[nodiscard]] constexpr bool operator!=(const RangeForwardIterator& rhs) const {return !operator==(rhs);}
	[[nodiscard]] constexpr bool operator==(decltype(nullptr)) const {return Range.Empty();}

	R Range;
};
INTRA_CORE_RANGE_END
