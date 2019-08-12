#pragma once

#include "Core/CRange.h"

INTRA_CORE_RANGE_BEGIN
template<typename R> struct RangeForwardIterator
{
	typedef TValueTypeOf<R> value_type;
	typedef TReturnValueTypeOf<R> return_value_type;
	typedef return_value_type& reference;
	typedef value_type* pointer;
	typedef intptr difference_type;

	RangeForwardIterator() = default;
	constexpr forceinline RangeForwardIterator(null_t) {}
	constexpr forceinline RangeForwardIterator(const R& range): Range(range) {}
	forceinline INTRA_CONSTEXPR2 RangeForwardIterator& operator++() {Range.PopFirst(); return *this;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline RangeForwardIterator operator++(int) {auto copy = Range; Range.PopFirst(); return copy;}
	INTRA_NODISCARD constexpr forceinline return_value_type operator*() const {return Range.First();}
	INTRA_NODISCARD constexpr forceinline TRemoveReference<return_value_type>* operator->() const {return &Range.First();}

	INTRA_NODISCARD constexpr forceinline bool operator==(const RangeForwardIterator& rhs) const {return Range == rhs.Range;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(const RangeForwardIterator& rhs) const {return !operator==(rhs);}
	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const {return Range.Empty();}

	R Range;
};
INTRA_CORE_RANGE_END
