#pragma once

#include <Intra/Core.h>
#include <Intra/Meta.h>
#include <Intra/Numeric/Math.h>
#include <Intra/Numeric/Traits.h>

namespace Intra { INTRA_BEGIN

template<auto Op, CNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr bool IsOverflowOp(T a, T b)
{
	if constexpr((MinValueOf<T> == 0 || double(MinValueOf<T>) == -Infinity) && double(MaxValueOf<T>) == Infinity) return false;
	else
	{
	#if defined(__GNUC__) || defined(__clang__)
		if constexpr(CBasicIntegral<T>)
		{
			if constexpr(VSameTypes(Op, Add)) return __builtin_add_overflow(a, b, &a);
			else if constexpr(VSameTypes(Op, Sub)) return __builtin_sub_overflow(a, b, &a);
			else if constexpr(VSameTypes(Op, Mul)) return __builtin_mul_overflow(a, b, &a);
		}
	#endif
		if constexpr(VSameTypes(Op, Add))
		{
			const auto res = T(TToUnsigned<T>(a) + TToUnsigned<T>(b));
			return b < 0? res >= a: res < a;
		}
		else if constexpr(VSameTypes(Op, Sub))
		{
			const auto res = T(TToUnsigned<T>(a) - TToUnsigned<T>(b));
			return b > 0? res >= a: res < a;
		}
		else if constexpr(VSameTypes(Op, Mul))
		{
			using MaxType = TIntOfSizeAtLeast<sizeof(int64), CBasicSigned<T>>;
			if constexpr(MaxType(MaxValueOf<T>) < MaxValueOf<MaxType>)
			{
				const auto mul = MaxType(a)*MaxType(b);
				return mul != MaxType(T(mul));
			}
			if(a == 0) return false;
			const auto apos = TToUnsigned<T>(Abs(a));
			const auto bpos = TToUnsigned<T>(Abs(b));
			const auto mulPos = apos * bpos;
			return mulPos > TToUnsigned<T>(MaxValueOf<T>) || bpos != mulPos / apos;
		}
		return false;
	}
}

template<typename T> struct NSafeCast
{
	// TODO
};

template<CIntegral T> struct NWrapOverflow;
template<typename T> struct NCheckedOverflow;
template<typename T> struct NSaturateOverflow;

template<typename T> concept COverflowWrapper =
	CInstanceOfTemplate<T, NWrapOverflow> ||
	CInstanceOfTemplate<T, NCheckedOverflow> ||
	CInstanceOfTemplate<T, NSaturateOverflow>;

template<CIntegral T> struct NWrapOverflow
{
	NWrapOverflow() = default;
	template<CNumber U> INTRA_FORCEINLINE constexpr NWrapOverflow(U u) noexcept: mValue(T(u)) {}
	
	// NOTE: on Clang 16 explicit(!CNoWrapConvertible<T, U>) evaluates even if requires evalutes to false, we must avoid it to prevent concept recursion
	template<typename U> requires (!COverflowWrapper<U>) && CNumber<U>// && (!CNoWrapConvertible<T, U>)
	explicit INTRA_FORCEINLINE constexpr operator U() const noexcept {return U(mValue);}
	//template<typename U> requires (!COverflowWrapper<U>) && CNumber<U> && CNoWrapConvertible<T, U>
	//INTRA_FORCEINLINE constexpr operator U() const noexcept {return U(mValue);}
	template<COverflowWrapper U> INTRA_FORCEINLINE constexpr NWrapOverflow(const U& t) noexcept: NWrapOverflow(t.WrappedValue()) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr NWrapOverflow operator+(const NWrapOverflow& rhs) const noexcept {return NWrapOverflow(TToUnsigned<T>(mValue) + TToUnsigned<T>(rhs.mValue));}
	[[nodiscard]] INTRA_FORCEINLINE constexpr NWrapOverflow operator-(const NWrapOverflow& rhs) const noexcept {return NWrapOverflow(TToUnsigned<T>(mValue) - TToUnsigned<T>(rhs.mValue));}
	[[nodiscard]] INTRA_FORCEINLINE constexpr NWrapOverflow operator*(const NWrapOverflow& rhs) const noexcept {return NWrapOverflow(mValue * rhs.mValue);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr NWrapOverflow operator/(const NWrapOverflow& rhs) const {return NWrapOverflow(mValue / rhs.mValue);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr NWrapOverflow operator%(const NWrapOverflow& rhs) const {return NWrapOverflow(mValue % rhs.mValue);}

	[[nodiscard]] INTRA_FORCEINLINE constexpr NWrapOverflow operator-() const noexcept requires CBasicSigned<T> {return T(1 + ~TToUnsigned<T>(mValue));}

	[[nodiscard]] INTRA_FORCEINLINE constexpr T& WrappedValue() noexcept {return mValue;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr const T& WrappedValue() const noexcept {return mValue;}
	using TagGenOpAssign = TTag<>;
	using TagGenOpCompare = TTag<>;
	using TagGenOpMixedCompare = TTag<>;
	using TagInheritNumericTraits = TTag<>;

private:
	T mValue{};
};

template<typename T> struct NCheckedOverflow
{
	NCheckedOverflow() = default;

	template<CNumber U> requires (!CBasicIntegral<U>) explicit INTRA_FORCEINLINE constexpr NCheckedOverflow(U u): mValue(T(u))
	{
		if constexpr(!CLosslessConvertible<U, T>)
			INTRA_ASSERT((CommonMinValue<U, T> <= u && u <= CommonMaxValue<U, T>));
	}
	template<CBasicIntegral U> INTRA_FORCEINLINE constexpr NCheckedOverflow(U u): mValue(T(u))
	{
		if constexpr(!CLosslessConvertible<U, T>)
			INTRA_ASSERT((CommonMinValue<U, T> <= u && u <= CommonMaxValue<U, T>));
	}

	// NOTE: on Clang 16 explicit(!CNoWrapConvertible<T, U>) evaluates even if requires evalutes to false, we must avoid it to prevent concept recursion
	template<typename U> requires (!COverflowWrapper<U>) && CNumber<U>// && (!CNoWrapConvertible<T, U>)
	explicit INTRA_FORCEINLINE constexpr operator U() const noexcept {return NCheckedOverflow<U>(mValue).WrappedValue();}
	//template<typename U> requires (!COverflowWrapper<U>) && CNumber<U> && CNoWrapConvertible<T, U>
	//INTRA_FORCEINLINE constexpr operator U() const noexcept {return NCheckedOverflow<U>(mValue).WrappedValue();}

	template<COverflowWrapper U> INTRA_FORCEINLINE constexpr NCheckedOverflow(const U& t) noexcept: NCheckedOverflow(t.WrappedValue()) {}

	[[nodiscard]] constexpr NCheckedOverflow operator+(NCheckedOverflow rhs) const
	{
		const auto res = T(TToUnsigned<T>(mValue) + TToUnsigned<T>(rhs.mValue));
		const bool noOverflow = rhs.mValue >= 0? res >= mValue: res < mValue;
		INTRA_ASSERT(noOverflow);
		return NCheckedOverflow(res);
	}

	[[nodiscard]] constexpr NCheckedOverflow operator-(NCheckedOverflow rhs) const
	{
		const auto res = T(TToUnsigned<T>(mValue) - TToUnsigned<T>(rhs.mValue));
		const bool noOverflow = rhs.mValue <= 0? res >= mValue: res < mValue;
		INTRA_ASSERT(noOverflow);
		return NCheckedOverflow(res);
	}

	[[nodiscard]] constexpr NCheckedOverflow operator*(NCheckedOverflow rhs) const
	{
		INTRA_ASSERT(!IsOverflowOp<Mul>(mValue, rhs.mValue));
		return NCheckedOverflow(mValue * rhs.mValue);
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr NCheckedOverflow operator/(NCheckedOverflow rhs) const {return NCheckedOverflow(mValue / rhs.mValue);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr NCheckedOverflow operator%(NCheckedOverflow rhs) const {return NCheckedOverflow(mValue % rhs.mValue);}

	[[nodiscard]] INTRA_FORCEINLINE constexpr NCheckedOverflow operator-() const requires CBasicSigned<T>
	{return NCheckedOverflow(T(1 + ~TToUnsigned<T>(mValue)));}

	[[nodiscard]] INTRA_FORCEINLINE constexpr T& WrappedValue() noexcept {return mValue;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr const T& WrappedValue() const noexcept {return mValue;}
	using TagGenOpAssign = TTag<>;
	using TagGenOpCompare = TTag<>;
	using TagGenOpMixedCompare = TTag<>;
	using TagInheritNumericTraits = TTag<>;

private:
	T mValue;
};

template<typename T> struct NSaturateOverflow
{
	NSaturateOverflow() = default;
	NSaturateOverflow(const NSaturateOverflow&) = default;
		
	template<CBasicIntegral U> INTRA_FORCEINLINE constexpr NSaturateOverflow(U u) noexcept: mValue(T(u))
	{
		constexpr bool isNarrowingCast = sizeof(T) < sizeof(U) ||
			CSameSize<T, U> && CBasicSigned<U> != CBasicSigned<T>;
		if constexpr(isNarrowingCast)
			mValue = T(Max(Min(u, CommonMaxValue<U, T>), CommonMinValue<U, T>));
	}
	template<CBasicFloatingPoint U> explicit INTRA_FORCEINLINE constexpr NSaturateOverflow(U u) noexcept: mValue(T(Max(Min(u, CommonMaxValue<U, T>), CommonMinValue<U, T>))) {}

	// NOTE: on Clang 16 explicit(!CNoWrapConvertible<T, U>) evaluates even if requires evalutes to false, we must avoid it to prevent concept recursion
	template<typename U> requires (!COverflowWrapper<U>) && CNumber<U>// && (!CNoWrapConvertible<T, U>)
	explicit INTRA_FORCEINLINE constexpr operator U() const noexcept {return NSaturateOverflow<U>(mValue).WrappedValue();}
	//template<typename U> requires (!COverflowWrapper<U>) && CNumber<U> && CNoWrapConvertible<T, U>
	//INTRA_FORCEINLINE constexpr operator U() const noexcept {return NSaturateOverflow<U>(mValue).WrappedValue();}

	template<COverflowWrapper U> INTRA_FORCEINLINE constexpr NSaturateOverflow(const U& t) noexcept: NSaturateOverflow(t.WrappedValue()) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr NSaturateOverflow operator+(NSaturateOverflow rhs) const noexcept
	{
		const auto res = T(TToUnsigned<T>(mValue) + TToUnsigned<T>(rhs.mValue));
		if(rhs.mValue >= 0)
		{
			if(res < mValue) return MaxValueOf<T>;
		}
		else if(res > mValue) return MinValueOf<T>;
		return NSaturateOverflow(res);
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr NSaturateOverflow operator-(NSaturateOverflow rhs) const noexcept
	{
		auto res = T(TToUnsigned<T>(mValue) - TToUnsigned<T>(rhs.mValue));
		if(rhs.mValue < 0)
		{
			if(res < mValue) return MaxValueOf<T>;
		}
		else if(res > mValue) return MinValueOf<T>;
		return NSaturateOverflow(res);
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr NSaturateOverflow operator*(NSaturateOverflow rhs) const noexcept
	{
		if(mValue == 0) return 0;
		if(!IsOverflowOp<Mul>(rhs.mValue, mValue)) rhs.mValue *= mValue;
		else return (mValue >= 0) == (rhs.mValue >= 0)? MaxValueOf<T>: MinValueOf<T>;
		return rhs;
	}

	[[nodiscard]] constexpr NSaturateOverflow operator/(NSaturateOverflow rhs) const {return NSaturateOverflow(mValue / rhs.mValue);}
	[[nodiscard]] constexpr NSaturateOverflow operator%(NSaturateOverflow rhs) const {return NSaturateOverflow(mValue % rhs.mValue);}
		
	[[nodiscard]] constexpr NSaturateOverflow operator-() const requires CBasicSigned<T>
	{return NSaturateOverflow(T(1 + ~TToUnsigned<T>(mValue)));}

	[[nodiscard]] constexpr T& WrappedValue() noexcept {return mValue;}
	[[nodiscard]] constexpr const T& WrappedValue() const noexcept {return mValue;}
	using TagGenOpAssign = TTag<>;
	using TagGenOpCompare = TTag<>;
	using TagGenOpMixedCompare = TTag<>;
	using TagInheritNumericTraits = TTag<>;

private:
	T mValue;
};

namespace z_D {template<typename T> concept CNonOverflowNumber = !COverflowWrapper<T> && CNumber<T>;}

template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W operator+(const W& lhs, const T& rhs) {return lhs + W(rhs);}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W operator+(const T& lhs, const W& rhs) {return W(lhs) + rhs;}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W& operator+=(const W& lhs, const T& rhs) {return lhs = lhs + W(rhs);}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W operator-(const W& lhs, const T& rhs) {return lhs - W(rhs);}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W operator-(const T& lhs, const W& rhs) {return W(lhs) - rhs;}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W& operator-=(const W& lhs, const T& rhs) {return lhs = lhs - W(rhs);}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W operator*(const W& lhs, const T& rhs) {return lhs * W(rhs);}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W operator*(const T& lhs, const W& rhs) {return W(lhs) * rhs;}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W& operator*=(const W& lhs, const T& rhs) {return lhs = lhs * W(rhs);}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W operator/(const W& lhs, const T& rhs) {return lhs / W(rhs);}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W operator/(const T& lhs, const W& rhs) {return W(lhs) / rhs;}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W& operator/=(const W& lhs, const T& rhs) {return lhs = lhs / W(rhs);}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W operator%(const W& lhs, const T& rhs) {return lhs % W(rhs);}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W operator%(const T& lhs, const W& rhs) {return W(lhs) % rhs;}
template<COverflowWrapper W, z_D::CNonOverflowNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr W& operator%=(const W& lhs, const T& rhs) {return lhs = lhs % W(rhs);}

template<COverflowWrapper W> INTRA_FORCEINLINE constexpr W operator++(const W& lhs, int) {auto res = lhs; lhs += 1; return res;}
template<COverflowWrapper W> INTRA_FORCEINLINE constexpr W& operator++(const W& lhs) {return lhs += 1;}
template<COverflowWrapper W> INTRA_FORCEINLINE constexpr W operator--(const W& lhs, int) {auto res = lhs; lhs -= 1; return res;}
template<COverflowWrapper W> INTRA_FORCEINLINE constexpr W& operator--(const W& lhs) {return lhs -= 1;}


template<typename T> using NDebugOverflow = TSelect<NCheckedOverflow<T>, T, Config::DebugCheckLevel != 0>;

using Size = NCheckedOverflow<size_t>;
using ClampedSize = NSaturateOverflow<size_t>;
using Index = NCheckedOverflow<size_t>;
using ClampedIndex = NSaturateOverflow<size_t>;
using LongSize = NCheckedOverflow<uint64>;
using ClampedLongSize = NSaturateOverflow<uint64>;
using LongIndex = NCheckedOverflow<uint64>;
using ClampedLongIndex = NSaturateOverflow<uint64>;

#if INTRA_CONSTEXPR_TEST
static_assert(CNumber<ClampedSize>);
static_assert(CNumber<NCheckedOverflow<size_t>>);
static_assert(!IsOverflowOp<Mul>(1, 3));
static_assert(!IsOverflowOp<Mul>(int64(1), int64(3)));
static_assert(!IsOverflowOp<Mul>(uint32(1), uint32(1)));
static_assert(NCheckedOverflow<uint32>(1) * NCheckedOverflow<uint32>(0) == 0);
static_assert(NCheckedOverflow<uint32>(1) * NCheckedOverflow<uint32>(1) == 1);
static_assert(NWrapOverflow<uint32>(0) == 0);
static_assert(CommonMaxValue<int32, uint32> == MaxValueOf<int32>);
static_assert(CommonMinValue<int32, size_t> == 0);
static_assert(z_D::CGenCompareOp<decltype(Equal), NWrapOverflow<uint32>, int>);
static_assert(z_D::CGenCompareOp<decltype(Equal), size_t, const ClampedSize&>);
static_assert(CValueWrapper<ClampedSize&>);
static_assert(z_D::CWrapperOpValid<decltype(Equal), ClampedSize&, int>);
static_assert(CValueWrapper<const ClampedSize>);
static_assert(CValueWrapper<const decltype(NWrapOverflow<uint32>(0x1234567890ULL))&&>);
static_assert(NWrapOverflow<uint32>(0x1234567890ULL) == 0x34567890U);
static_assert(NSaturateOverflow<uint32>(0x1234567890ULL) == 0xFFFFFFFF);
static_assert(ClampedLongSize(NSaturateOverflow<uint32>(0x1234567890ULL)) == 0xFFFFFFFF);
static_assert(ClampedIndex(Index(12345)) == 12345);
static_assert([](ClampedSize clampedSize) {return clampedSize >= 3;}(5));
static_assert(CNumber<Index>);
#endif
//static_assert(NCheckedOverflow<uint32>(0x1234567890ULL) == 0x34567890); //compiler error

} INTRA_END
