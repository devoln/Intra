#pragma once

#include "Assert.h"
#include "Functional.h"
#include "Numeric.h"

INTRA_BEGIN

// TODO: https://foonathan.net/2016/10/strong-typedefs/

/** Use Owner<T*> to explicitly show that the pointer owns its data.

  Only one pointer can own an object. It can be assigned to other Owner but the previous Owner must be reset.
  @see gsl::owner and its description in C++ Core Guidelines for more information.
*/
template<typename T> using Owner = T;

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

template<typename T> struct Out
{
	T& mRef;
public:
	constexpr Out(T& x);
	Out& operator=(const Out&) = delete;
	T& operator=(T rhs)
	{
		mRef = INTRA_MOVE(rhs);
		return *this;
	}
};

template<typename T, size_t N> struct Array
{
	T Elements[N];
	constexpr T* Data() noexcept {return Elements;}
	constexpr const T* Data() const noexcept {return Elements;}
	constexpr index_t Length() const noexcept {return N;}
	T& operator[](Index index) {INTRA_PRECONDITION(size_t(index) < N); return Elements[size_t(index)];}
	const T& operator[](Index index) const {INTRA_PRECONDITION(size_t(index) < N); return Elements[size_t(index)];}
};
template<typename T, typename... Ts> Array(T, Ts...) ->
	Array<RequiresAssert<CSame<T, Ts...>, T>, 1 + sizeof...(Ts)>;

struct Overflow
{
	template<typename T> struct Wrap
	{
		T Value;
		Wrap() = default;
		template<typename U> constexpr Wrap(U u) noexcept: Value(T(u)) {}
		
		template<typename U> requires !CNoWrapConvertible<T, U>
		explicit constexpr operator U() const noexcept {return U(Value);}

		template<typename U> requires CNoWrapConvertible<T, U>
		constexpr operator U() const noexcept {return Value;}

		[[nodiscard]] constexpr Wrap operator+(Wrap rhs) const noexcept {return Wrap(TToUnsigned<T>(Value) + TToUnsigned<T>(rhs.Value));}
		[[nodiscard]] constexpr Wrap operator-(Wrap rhs) const noexcept {return Wrap(TToUnsigned<T>(Value) - TToUnsigned<T>(rhs.Value));}
		[[nodiscard]] constexpr Wrap operator*(Wrap rhs) const noexcept {return Wrap(Value * rhs.Value);}
		[[nodiscard]] constexpr Wrap operator/(Wrap rhs) const {return Wrap(Value / rhs.Value);}

		[[nodiscard]] constexpr Wrap operator-() const noexcept requires CSigned<T>
		{return T(1 + ~TToUnsigned<T>(Value));}

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
	INTRA_IGNORE_WARN_CONSTANT_CONDITION
	template<typename T> struct Checked
	{
		T Value;
		Checked() = default;

		template<CIntegral U> constexpr Checked(U u): Value(T(u))
		{
			constexpr bool isNarrowingCast = sizeof(T) < sizeof(U) ||
				sizeof(T) == sizeof(U) && CSigned<U> != CSigned<T>;
			if(!isNarrowingCast) return;
			constexpr U min = TCommonRange<U, T>::Min;
			constexpr U max = TCommonRange<U, T>::Max;
			INTRA_ASSERT(min <= u && u <= max);
		}

		template<CIntegral U> requires !CNoWrapConvertible<T, U>
		explicit constexpr operator U() const noexcept {return Checked<U>(Value).Value;}

		template<CNumber U> requires CNoWrapConvertible<T, U>
		constexpr operator U() const noexcept {return Value;}

		template<CIntegral U> constexpr Checked(Checked<U> t) noexcept: Value(t.Value) {}
		template<CIntegral U> constexpr Checked(Wrap<U> t) noexcept: Checked(t.Value) {}

		constexpr operator Wrap<T>() const {return Wrap<T>(Value);}

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

		[[nodiscard]] constexpr Checked operator-() const requires CSigned<T>
		{return Checked(T(1 + ~TToUnsigned<T>(Value)));}

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
		Saturate(const Saturate&) = default;
		
		template<CIntegral U> constexpr Saturate(U u) noexcept: Value(T(u))
		{
			constexpr bool isNarrowingCast = sizeof(T) < sizeof(U) ||
				sizeof(T) == sizeof(U) && CSigned<U> != CSigned<T>;
			if constexpr(isNarrowingCast)
				Value = T(Max(Min(u, TCommonRange<U, T>::Max), TCommonRange<U, T>::Min));
		}

		template<CIntegral U> requires !CNoWrapConvertible<T, U>
		explicit constexpr operator U() const noexcept {return Saturate<U>(Value).Value;}

		template<CNumber U> requires CNoWrapConvertible<T, U>
		constexpr operator U() const noexcept {return Value;}

		template<CIntegral U> constexpr Saturate(const Saturate<U>& t) noexcept: Saturate(t.Value) {}
		template<CIntegral U> constexpr Saturate(Checked<U> t) noexcept: Saturate(t.Value) {}
		template<CIntegral U> constexpr Saturate(Wrap<U> t) noexcept: Saturate(t.Value) {}
		template<CIntegral U> constexpr operator Checked<U>() const {return Checked<U>(Value);}
		template<CIntegral U> constexpr operator Wrap<U>() const noexcept {return Wrap<U>(Value);}


		[[nodiscard]] constexpr Saturate operator+(Saturate rhs) const noexcept
		{
			auto res = T(TToUnsigned<T>(Value) + TToUnsigned<T>(rhs.Value));
			if(rhs.Value >= 0)
			{
				if(res < Value)
					return MaxValueOf<T>;
			}
			else if(res > Value)
				return MinValueOf<T>;
			return Saturate(res);
		}

		[[nodiscard]] constexpr Saturate operator-(Saturate rhs) const noexcept
		{
			auto res = T(TToUnsigned<T>(Value) - TToUnsigned<T>(rhs.Value));
			if(rhs.Value < 0)
			{
				if(res < Value)
					return MaxValueOf<T>;
			}
			else if(res > Value)
				return MinValueOf<T>;
			return Saturate(res);
		}

		[[nodiscard]] constexpr Saturate operator*(Saturate rhs) const noexcept
		{
			if(Value == 0) return 0;
			if(CanMultiplyWithoutOverflow(rhs.Value, Value)) rhs.Value *= Value;
			else return (Value >= 0) == (rhs.Value >= 0)? MaxValueOf<T>: MinValueOf<T>;
			return rhs;
		}

		[[nodiscard]] constexpr Saturate operator/(Saturate rhs) const {return Saturate(Value / rhs.Value);}
		
		[[nodiscard]] constexpr Saturate operator-() const requires CSigned<T>
		{return Saturate(T(1 + ~TToUnsigned<T>(Value)));}

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
	template<typename T> using CheckedInDebug = TSelect<Checked<T>, Wrap<T>, !!Config::DebugCheckLevel>;
};

using Size = Overflow::CheckedInDebug<size_t>;
using ClampedSize = Overflow::Saturate<size_t>;
using Index = Overflow::CheckedInDebug<size_t>;
using ClampedIndex = Overflow::Saturate<size_t>;
using LongSize = Overflow::CheckedInDebug<uint64>;
using ClampedLongSize = Overflow::Saturate<uint64>;
using LongIndex = Overflow::CheckedInDebug<uint64>;
using ClampedLongIndex = Overflow::Saturate<uint64>;

static_assert(Overflow::Wrap<uint32>(0x1234567890ULL) == 0x34567890);
static_assert(Overflow::Saturate<uint32>(0x1234567890ULL) == 0xFFFFFFFF);
static_assert(ClampedLongSize(Overflow::Saturate<uint32>(0x1234567890ULL)) == 0xFFFFFFFF);
static_assert(ClampedIndex(Index(12345)) == 12345);
//static_assert(Overflow::Checked<uint32>(0x1234567890ULL) == 0x34567890); //compiler error
INTRA_END
