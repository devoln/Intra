#pragma once

#include "Assert.h"
#include "Functional.h"
#include "Numeric.h"

INTRA_BEGIN
/** Use NotNull<T*> to explicitly show that the pointer must not be null.
  An attempt to pass null (nullptr) or NULL directly will result in a compile time error.
  An attempt to pass null pointer in runtime will cause an assertion failure.
*/
template<typename T> struct NotNull
{
	NotNull(int) = delete;
	NotNull(decltype(null)) = delete;
	constexpr NotNull(T ptr): mPtr(ptr) {INTRA_PRECONDITION(ptr != null);}
	constexpr operator T() const {return mPtr;}
private:
	T mPtr;
};

/** Use NonNegative<T> to explicitly show that the number must not be negative.
  An attempt to pass a negative value in runtime will cause an assertion failure.
*/
template<typename T> struct NonNegative
{
	constexpr NonNegative(T val): mVal(val) {INTRA_PRECONDITION(val >= 0);}
	constexpr operator T() const {return mVal;}
private:
	T mVal;
};

struct Overflow
{
	template<typename T> struct Wrap
	{
		T Value;
		Wrap() = default;
		template<typename U> constexpr Wrap(U u) noexcept: Value(T(u)) {}
		template<typename U, typename = Requires<CPlainIntegral<U>>> explicit constexpr operator U() const {return U(Value);}

		[[nodiscard]] constexpr Wrap operator+(Wrap rhs) const noexcept {return Wrap(TToUnsigned<T>(Value) + TToUnsigned<T>(rhs.Value));}
		[[nodiscard]] constexpr Wrap operator-(Wrap rhs) const noexcept {return Wrap(TToUnsigned<T>(Value) - TToUnsigned<T>(rhs.Value));}
		[[nodiscard]] constexpr Wrap operator*(Wrap rhs) const noexcept {return Wrap(Value * rhs.Value);}
		[[nodiscard]] constexpr Wrap operator/(Wrap rhs) const {return Wrap(Value / rhs.Value);}

		template<typename U = T, Requires<CSigned<U>>>
		[[nodiscard]] constexpr Wrap operator-() const noexcept {return T(1 + ~TToUnsigned<T>(Value));}

		constexpr Wrap& operator+=(Wrap rhs) noexcept {return *this = *this + rhs;}
		constexpr Wrap& operator-=(Wrap rhs) noexcept {return *this = *this - rhs;}
		constexpr Wrap& operator*=(Wrap rhs) noexcept {return *this = *this * rhs;}
		constexpr Wrap& operator/=(Wrap rhs) {return *this = *this / rhs;}

		[[nodiscard]] constexpr bool operator==(Wrap rhs) const noexcept {return Value == rhs.Value;}
		[[nodiscard]] constexpr bool operator!=(Wrap rhs) const noexcept {return Value != rhs.Value;}
		[[nodiscard]] constexpr bool operator<(Wrap rhs) const noexcept {return Value < rhs.Value;}
		[[nodiscard]] constexpr bool operator>(Wrap rhs) const noexcept {return Value > rhs.Value;}
		[[nodiscard]] constexpr bool operator<=(Wrap rhs) const noexcept {return Value <= rhs.Value;}
		[[nodiscard]] constexpr bool operator>=(Wrap rhs) const noexcept {return Value >= rhs.Value;}
	};
	INTRA_IGNORE_WARNING_CONSTANT_CONDITION
	template<typename T> struct Checked
	{
		T Value;
		Checked() = default;
		template<typename U, typename = Requires<CIntegral<U>>> constexpr Checked(U u): Value(T(u))
		{
			constexpr bool isNarrowingCast = sizeof(T) < sizeof(U) ||
				sizeof(T) == sizeof(U) && CSigned<U> != CSigned<T>;
			if(!isNarrowingCast) return;
			constexpr U min = TCommonRange<U, T>::Min;
			constexpr U max = TCommonRange<U, T>::Max;
			INTRA_ASSERT(min <= u && u <= max);
		}
		template<typename U, typename = Requires<CPlainIntegral<U>>>
		explicit constexpr operator U() const {return Checked<U>(Value).Value;}

		[[nodiscard]] constexpr Checked operator+(Checked rhs) const
		{
			const auto res = T(TToUnsigned<T>(Value) + TToUnsigned<T>(rhs.Value));
			const bool noOverflow = rhs.Value >= 0? res >= Value: res < Value;
			INTRA_ASSERT(noOverflow);
			return Checked(res);
		}

		[[nodiscard]] constexpr Checked operator-(Checked rhs) const
		{
			const auto res = T(TToUnsigned<T>(Value) - TToUnsigned<T>(rhs.Value));
			const bool noOverflow = rhs.Value <= 0? res >= Value: res < Value;
			INTRA_ASSERT(noOverflow);
			return Checked(res);
		}

		[[nodiscard]] constexpr Checked operator*(Checked rhs) const
		{
			INTRA_ASSERT(CanMultiplyWithoutOverflow(Value, rhs.Value));
			return Checked(Value * rhs.Value);
		}

		[[nodiscard]] constexpr Checked operator/(Checked rhs) const {return Checked(Value / rhs.Value);}

		template<typename U = T> [[nodiscard]] constexpr Requires<
			CSigned<U>,
		Checked> operator-() const {return Checked(T(1 + ~TToUnsigned<T>(Value)));}

		constexpr Checked& operator+=(Checked rhs) {return *this = *this + rhs;}
		constexpr Checked& operator-=(Checked rhs) {return *this = *this - rhs;}
		constexpr Checked& operator*=(Checked rhs) {return *this = *this * rhs;}
		constexpr Checked& operator/=(Checked rhs) {return *this = *this / rhs;}

		[[nodiscard]] constexpr bool operator==(Checked rhs) const noexcept {return Value == rhs.Value;}
		[[nodiscard]] constexpr bool operator!=(Checked rhs) const noexcept {return Value != rhs.Value;}
		[[nodiscard]] constexpr bool operator<(Checked rhs) const noexcept {return Value < rhs.Value;}
		[[nodiscard]] constexpr bool operator>(Checked rhs) const noexcept {return Value > rhs.Value;}
		[[nodiscard]] constexpr bool operator<=(Checked rhs) const noexcept {return Value <= rhs.Value;}
		[[nodiscard]] constexpr bool operator>=(Checked rhs) const noexcept {return Value >= rhs.Value;}
	};
	template<typename T> struct Saturate
	{
		T Value;
		Saturate() = default;
		template<typename U, typename = Requires<CPlainIntegral<U>>>
		constexpr Saturate(U u) noexcept: Value(T(u))
		{
			constexpr bool isNarrowingCast = sizeof(T) < sizeof(U) ||
				sizeof(T) == sizeof(U) && CSigned<U> != CSigned<T>;
			if constexpr(isNarrowingCast)
				Value = T(FMax(FMin(u, TCommonRange<U, T>::Max), TCommonRange<U, T>::Min));
		}

		template<typename U, typename = Requires<CPlainIntegral<U>>>
		explicit constexpr operator U() const noexcept {return Saturate<U>(Value).Value;}

		constexpr Saturate(Checked<T> t) noexcept: Value(t.Value) {}
		constexpr operator Checked<T>() const {return Checked<T>(Value);}

		[[nodiscard]] constexpr Saturate operator+(Saturate rhs) const noexcept
		{
			auto res = T(TToUnsigned<T>(Value) + TToUnsigned<T>(rhs.Value));
			if(rhs.Value >= 0)
			{
				if(res < Value)
					return LMaxOf<T>;
			}
			else if(res > Value)
				return LMinOf<T>;
			return Saturate(res);
		}

		[[nodiscard]] constexpr Saturate operator-(Saturate rhs) const noexcept
		{
			auto res = T(TToUnsigned<T>(Value) - TToUnsigned<T>(rhs.Value));
			if(rhs.Value < 0)
			{
				if(res < Value)
					return LMaxOf<T>;
			}
			else if(res > Value)
				return LMinOf<T>;
			return Saturate(res);
		}

		[[nodiscard]] constexpr Saturate operator*(Saturate rhs) const noexcept
		{
			if(Value == 0) return 0;
			if(CanMultiplyWithoutOverflow(rhs.Value, Value)) rhs.Value *= Value;
			else return (Value >= 0) == (rhs.Value >= 0)? LMaxOf<T>: LMinOf<T>;
			return rhs;
		}

		[[nodiscard]] constexpr Saturate operator/(Saturate rhs) const {return Saturate(Value / rhs.Value);}
		
		template<typename U = T, typename = Requires<CSigned<U>>>
		[[nodiscard]] constexpr Saturate operator-() const {return Saturate(T(1 + ~TToUnsigned<T>(Value)));}

		constexpr Saturate& operator+=(Saturate rhs) noexcept {return *this = *this + rhs;}
		constexpr Saturate& operator-=(Saturate rhs) noexcept {return *this = *this - rhs;}
		constexpr Saturate& operator*=(Saturate rhs) noexcept {return *this = *this * rhs;}
		constexpr Saturate& operator/=(Saturate rhs) {return *this = *this / rhs;}

		[[nodiscard]] constexpr bool operator==(Saturate rhs) const noexcept {return Value == rhs.Value;}
		[[nodiscard]] constexpr bool operator!=(Saturate rhs) const noexcept {return Value != rhs.Value;}
		[[nodiscard]] constexpr bool operator<(Saturate rhs) const noexcept {return Value < rhs.Value;}
		[[nodiscard]] constexpr bool operator>(Saturate rhs) const noexcept {return Value > rhs.Value;}
		[[nodiscard]] constexpr bool operator<=(Saturate rhs) const noexcept {return Value <= rhs.Value;}
		[[nodiscard]] constexpr bool operator>=(Saturate rhs) const noexcept {return Value >= rhs.Value;}
	};
	template<typename T> using CheckedInDebug =
#ifdef INTRA_DEBUG
		Checked<T>;
#else
		Wrap<T>;
#endif
};

using Size = Overflow::CheckedInDebug<size_t>;
using ClampedSize = Overflow::Saturate<size_t>;
using Index = Overflow::CheckedInDebug<size_t>;
using ClampedIndex = Overflow::Saturate<size_t>;
using LongSize = Overflow::CheckedInDebug<uint64>;
using ClampedLongSize = Overflow::Saturate<uint64>;
using LongIndex = Overflow::CheckedInDebug<uint64>;
using ClampedLongIndex = Overflow::Saturate<uint64>;
INTRA_END
