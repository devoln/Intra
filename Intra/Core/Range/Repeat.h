#pragma once

#include "Core/Range/Concepts.h"

#include "Core/Range/Take.h"
#include "Core/Optional.h"

#include "Core/Range/Span.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<typename T> struct RRepeat
{
	enum: bool {RangeIsInfinite = true};

	RRepeat() = default;
	constexpr forceinline RRepeat(null_t) {}
	constexpr forceinline RRepeat(T&& val): mValue(Move(val)) {}
	constexpr forceinline RRepeat(const T& val): mValue(val) {}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mValue == null;}
	constexpr forceinline const T& First() const {return *mValue;}

	INTRA_CONSTEXPR2 forceinline void PopFirst() {}
	INTRA_NODISCARD constexpr forceinline const T& operator[](size_t) const {return *mValue;}
	
private:
	Optional<T> mValue;
};


template<typename T> INTRA_NODISCARD constexpr forceinline RRepeat<T> Repeat(T&& val) noexcept {return {Forward<T>(val)};}
template<typename T> INTRA_NODISCARD constexpr forceinline auto Repeat(T&& val, size_t n) noexcept {return Take(Repeat(Forward<T>(val)), n);}
template<typename T, size_t N> INTRA_NODISCARD constexpr forceinline auto Repeat(T(&arr)[N]) noexcept {return Repeat(SpanOf(arr));}
template<typename T, size_t N> INTRA_NODISCARD constexpr forceinline auto Repeat(T(&arr)[N], size_t n) noexcept {return Take(Repeat(SpanOf(arr)), n);}
INTRA_CORE_RANGE_END
