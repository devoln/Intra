#pragma once

#include <Intra/Core.h>

namespace Intra { INTRA_BEGIN

template<class Op, CNumber T> [[nodiscard]] constexpr bool IsOverflowOp(T a, T b)
{
	if constexpr((MinValueOf<T> == 0 || MinValueOf<T> == -Infinity) && MaxValueOf<T> == Infinity) return false;
	else
	{
	#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
		if constexpr(CBasicIntegral<T>)
		{
		#ifdef __INTEL_COMPILER
			if(IsConstantEvaluated())
		#endif
			{
				if constexpr(CSameUnqual<Op, decltype(Add)>) return __builtin_add_overflow(a, b, &a);
				else if constexpr(CSameUnqual<Op, decltype(Sub)>) return __builtin_sub_overflow(a, b, &a);
				else if constexpr(CSameUnqual<Op, decltype(Mul)>) return __builtin_mul_overflow(a, b, &a);
			}
		}
	#endif
		if constexpr(CSameUnqual<Op, decltype(Add)>)
		{
			const auto res = T(TToUnsigned<T>(a) + TToUnsigned<T>(b));
			return b < 0? res >= a: res < a;
		}
		else if constexpr(CSameUnqual<Op, decltype(Sub)>)
		{
			const auto res = T(TToUnsigned<T>(a) - TToUnsigned<T>(b));
			return b > 0? res >= a: res < a;
		}
		else if constexpr(CSameUnqual<Op, decltype(Mul)>)
		{
			using MaxType = TIntOfSizeAtLeast<sizeof(int64), CBasicSigned<T>>;
			if(MaxType(MaxValueOf<T>) < MaxValueOf<MaxType>)
			{
				const auto mul = MaxType(a)*MaxType(b);
				return mul == MaxType(T(mul));
			}
			if(a == 0) return false;
			const auto apos = TToUnsigned<T>(a < 0? -a: a);
			const auto bpos = TToUnsigned<T>(b < 0? -b: b);
			const auto mulPos = apos*bpos;
			return mulPos > TToUnsigned<T>(MaxValueOf<T>) || apos != mulPos / apos;
		}
		return false;
	}
}

template<typename T> struct NSafeCast
{

};

// TODO
template<typename T> concept CIntegral = CBasicIntegral<T>;
template<typename T> concept CFloatingPoint = CBasicFloatingPoint<T>;
template<typename T> concept CSigned = CBasicSigned<T>;

template<CIntegral T> struct NWrapOverflow
{
	T Value{};
	NWrapOverflow() = default;
	template<CNumber U> constexpr NWrapOverflow(U u) noexcept: Value(T(u)) {}
	
	template<CNumber U> explicit(!CNoWrapConvertible<T, U>) constexpr operator U() const noexcept {return U(Value);}

	[[nodiscard]] constexpr NWrapOverflow operator+(const NWrapOverflow& rhs) const noexcept {return NWrapOverflow(TToUnsigned<T>(Value) + TToUnsigned<T>(rhs.Value));}
	[[nodiscard]] constexpr NWrapOverflow operator-(const NWrapOverflow& rhs) const noexcept {return NWrapOverflow(TToUnsigned<T>(Value) - TToUnsigned<T>(rhs.Value));}
	[[nodiscard]] constexpr NWrapOverflow operator*(const NWrapOverflow& rhs) const noexcept {return NWrapOverflow(Value * rhs.Value);}
	[[nodiscard]] constexpr NWrapOverflow operator/(const NWrapOverflow& rhs) const {return NWrapOverflow(Value / rhs.Value);}

	[[nodiscard]] constexpr Wrap operator-() const noexcept requires CBasicSigned<T> {return T(1 + ~TToUnsigned<T>(Value));}

	[[nodiscard]] constexpr T& WrappedValue() noexcept {return Value;}
	[[nodiscard]] constexpr const T& WrappedValue() const noexcept {return Value;}
	using MetaGenOpAssign = TTag<>;
	using MetaGenOpCompare = TTag<>;
	using MetaInheritNumericTraits = TTag<>;
};

template<typename T> struct NCheckedOverflow
{
	T Value;
	NCheckedOverflow() = default;

	template<CBasicIntegral U> constexpr NCheckedOverflow(U u): Value(T(u))
	{
		if constexpr(!CLosslessConvertible<U, T>)
			INTRA_ASSERT(CommonMinValue<U, T> <= u && u <= CommonMaxValue<U, T>);
	}

	template<CNumber U> explicit(!CNoWrapConvertible<T, U>) constexpr operator U() const noexcept {return Checked<U>(Value).Value;}

	template<CBasicIntegral U> constexpr NCheckedOverflow(const NCheckedOverflow<U>& t) noexcept: Value(t.Value) {}
	template<CBasicIntegral U> constexpr NCheckedOverflow(const Wrap<U>& t) noexcept: NCheckedOverflow(t.Value) {}

	constexpr operator Wrap<T>() const {return Wrap<T>(Value);}

	[[nodiscard]] constexpr NCheckedOverflow operator+(NCheckedOverflow rhs) const
	{
		const auto res = T(TToUnsigned<T>(Value) + TToUnsigned<T>(rhs.Value));
		const bool noOverflow = rhs.Value >= 0? res >= Value: res < Value;
		INTRA_ASSERT(noOverflow);
		return NCheckedOverflow(res);
	}

	[[nodiscard]] constexpr NCheckedOverflow operator-(NCheckedOverflow rhs) const
	{
		const auto res = T(TToUnsigned<T>(Value) - TToUnsigned<T>(rhs.Value));
		const bool noOverflow = rhs.Value <= 0? res >= Value: res < Value;
		INTRA_ASSERT(noOverflow);
		return NCheckedOverflow(res);
	}

	[[nodiscard]] constexpr NCheckedOverflow operator*(NCheckedOverflow rhs) const
	{
		INTRA_ASSERT(CanMultiplyWithoutOverflow(Value, rhs.Value));
		return NCheckedOverflow(Value * rhs.Value);
	}

	[[nodiscard]] constexpr NCheckedOverflow operator/(NCheckedOverflow rhs) const {return NCheckedOverflow(Value / rhs.Value);}

	[[nodiscard]] constexpr NCheckedOverflow operator-() const requires CBasicSigned<T>
	{return Checked(T(1 + ~TToUnsigned<T>(Value)));}

	[[nodiscard]] constexpr T& WrappedValue() noexcept {return Value;}
	[[nodiscard]] constexpr const T& WrappedValue() const noexcept {return Value;}
	using MetaGenOpAssign = TTag<>;
	using MetaGenOpCompare = TTag<>;
	using MetaInheritNumericTraits = TTag<>;
};
template<typename T> struct Saturate
{
	T Value;
	Saturate() = default;
	Saturate(const Saturate&) = default;
		
	template<CBasicIntegral U> constexpr Saturate(U u) noexcept: Value(T(u))
	{
		constexpr bool isNarrowingCast = sizeof(T) < sizeof(U) ||
			sizeof(T) == sizeof(U) && CBasicSigned<U> != CBasicSigned<T>;
		if constexpr(isNarrowingCast)
			Value = T(Max(Min(u, TCommonRange<U, T>::Max), TCommonRange<U, T>::Min));
	}

	template<CNumber U> explicit(!CNoWrapConvertible<T, U>) constexpr operator U() const noexcept {return Saturate<U>(Value).Value;}

	template<CBasicIntegral U> constexpr Saturate(const Saturate<U>& t) noexcept: Saturate(t.Value) {}
	template<CBasicIntegral U> constexpr Saturate(Checked<U> t) noexcept: Saturate(t.Value) {}
	template<CBasicIntegral U> constexpr Saturate(Wrap<U> t) noexcept: Saturate(t.Value) {}
	template<CBasicIntegral U> constexpr operator Checked<U>() const {return Checked<U>(Value);}
	template<CBasicIntegral U> constexpr operator Wrap<U>() const noexcept {return Wrap<U>(Value);}


	[[nodiscard]] constexpr Saturate operator+(Saturate rhs) const noexcept
	{
		auto res = T(TToUnsigned<T>(Value) + TToUnsigned<T>(rhs.Value));
		if(rhs.Value >= 0)
		{
			if(res < Value) return MaxValueOf<T>;
		}
		else if(res > Value) return MinValueOf<T>;
		return Saturate(res);
	}

	[[nodiscard]] constexpr Saturate operator-(Saturate rhs) const noexcept
	{
		auto res = T(TToUnsigned<T>(Value) - TToUnsigned<T>(rhs.Value));
		if(rhs.Value < 0)
		{
			if(res < Value) return MaxValueOf<T>;
		}
		else if(res > Value) return MinValueOf<T>;
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
		
	[[nodiscard]] constexpr Saturate operator-() const requires CBasicSigned<T>
	{return Saturate(T(1 + ~TToUnsigned<T>(Value)));}

	[[nodiscard]] constexpr T& WrappedValue() noexcept {return Value;}
	[[nodiscard]] constexpr const T& WrappedValue() const noexcept {return Value;}
	using MetaGenOpAssign = TTag<>;
	using MetaGenOpCompare = TTag<>;
	using MetaInheritNumericTraits = TTag<>;
};
template<typename T> using CheckedInDebug = TSelect<Checked<T>, T, Config::DebugCheckLevel != 0>;

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

} INTRA_END
