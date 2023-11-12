#pragma once

#include <Intra/Core.h>

namespace Intra { INTRA_BEGIN

/** Use Owner<T*> or Owner<Span<T>> to explicitly indicate that the pointer or range owns its data and must be manually deleted.

  Only one pointer or range can own an object. It can be assigned to other Owner but the previous Owner must be reset.
  @see gsl::owner and its description in C++ Core Guidelines for more information.
*/
template<typename T> using Owner = T;

/** Use NotNull<T*> to explicitly show that the pointer must not be nullptr.
  An attempt to pass nullptr or NULL directly will result in a compile time error.
  An attempt to pass nullptr pointer in runtime will cause an assertion failure.
*/
template<typename T> struct NotNull
{
	NotNull(int) = delete;
	NotNull(decltype(nullptr)) = delete;
	constexpr NotNull(T ptr): mPtr(ptr) {INTRA_PRECONDITION(ptr != nullptr);}
	constexpr operator T() const {return mPtr;}
	template<CStaticCastable<T> U> explicit constexpr operator U() const {return static_cast<U>(mPtr);}
private:
	T mPtr;
};

/** Use NonNegative<T> to explicitly show that the number must not be negative.
  An attempt to pass a negative value in runtime will cause an assertion failure.
*/
template<typename T> struct NonNegative
{
	constexpr NonNegative(T val): mVal(INTRA_MOVE(val)) {INTRA_PRECONDITION(val >= 0);}
	constexpr operator T() const {return mVal;}
private:
	T mVal;
};

template<typename T> struct Out
{
	T& Ref;
	INTRA_FORCEINLINE explicit constexpr Out(T& x): Ref(x) {}
	Out& operator=(const Out&) = delete;
	template<typename U> requires CAssignable<T&, U&&> INTRA_FORCEINLINE constexpr T& operator=(U&& rhs) {return Ref = INTRA_FWD(rhs);}
};

struct TMod;

using TOperations = TList<
	decltype(Negate), decltype(Deref),

	decltype(Add), decltype(Sub), decltype(Mul), decltype(Div), TMod,
	decltype(LShift), decltype(RShift), decltype(BitAnd), decltype(BitOr), decltype(BitXor), decltype(And), decltype(Or),
	decltype(Equal), decltype(NotEqual), decltype(Less), decltype(Greater), decltype(LEqual), decltype(GEqual),
	decltype(Min), decltype(Max)
>;

template<class W> using TWrappedTypeRef = decltype(Val<W>().WrappedValue());
template<class W> using TWrappedType = TUnqualRef<TWrappedTypeRef<W>>;
template<class W> concept CValueWrapper = CConstructible<TRemoveReference<W>, TWrappedTypeRef<W>>;

template<class W> concept CBitsetEnum = CScopedEnum<W> && requires {W::Meta_BitsetWidth;};

namespace z_D {
template<class Op, typename T1, typename T2> [[nodiscard]] constexpr auto ValueWrapperOpImpl(T1&& a, T2&& b)
{
	if constexpr(CValueWrapper<T1>)
	{
		if constexpr(CSameUnqualRef<T1, T2>)
		{
			if constexpr(CCallable<Op, TWrappedTypeRef<T1>, TWrappedTypeRef<T2>>)
				return Op()(a.WrappedValue(), b.WrappedValue());
			else static_assert(CFalse<Op>);
		}
		else if constexpr(CCallable<Op, TWrappedTypeRef<T1>, T2>)
			return Op()(a.WrappedValue(), b);
		else static_assert(CFalse<Op>);
	}
	else if constexpr(CValueWrapper<T2>)
	{
		if constexpr(CCallable<Op, T1, TWrappedTypeRef<T2>>)
			return Op()(a, b.WrappedValue());
		else static_assert(CFalse<Op>);
	}
	else static_assert(CFalse<Op>);
}

template<class Op, typename T1, typename T2>
concept CWrapperOpValid =
#if defined(_MSC_VER) && !defined(__clang__) // NOTE: relaxed check to workaround a bug in MSVC 17.7.6
	CValueWrapper<T1> || CValueWrapper<T2>;
#else
	CNonVoid<decltype(ValueWrapperOpImpl<Op>(Val<T1>(), Val<T2>()))>;
#endif

template<typename CmpOp, typename T1, typename T2> concept CGenCompareOp = (
	CValueWrapper<T1> && CSameUnqualRef<T1, T2> && requires {TRemoveReference<T1>::TagGenOpCompare::True;} ||
	CValueWrapper<T1> && requires {TRemoveReference<T1>::TagGenOpMixedCompare::True;} ||
	CValueWrapper<T2> && requires {TRemoveReference<T2>::TagGenOpMixedCompare::True;}) &&
	CWrapperOpValid<CmpOp, T1, T2>;
}

inline namespace Operators {
template<typename T1, typename T2> requires
	requires {TRemoveReference<T2>::TagGenMixedTypeAddOps::True;} && (!CValueWrapper<T2> && !CSameUnqualRef<T1, T2>) &&
	CCallable<decltype(Add), T2, T1>
[[nodiscard]] constexpr auto operator+(T1&& lhs, T2&& rhs) {return INTRA_FWD(rhs) + INTRA_FWD(lhs);}

template<typename T1, typename T2> requires
	requires {TRemoveReference<T2>::TagGenMixedTypeAddOps::True;} && (!CValueWrapper<T2> && !CSameUnqualRef<T1, T2>) &&
	CCallable<decltype(Add), T2, T1> && CCallable<decltype(Negate), T2>
[[nodiscard]] constexpr auto operator-(T1&& lhs, T2&& rhs) {return -INTRA_FWD(rhs) + INTRA_FWD(lhs);}

template<typename T1, typename T2> requires
	requires {TRemoveReference<T2>::TagGenMixedTypeMulOps::True;} && (!CValueWrapper<T2> && !CSameUnqualRef<T1, T2>) &&
	CCallable<decltype(Mul), T2, T1>
[[nodiscard]] constexpr auto operator*(T1&& lhs, T2&& rhs) {return INTRA_FWD(rhs) * INTRA_FWD(lhs);}


template<CEnum T> requires requires {T::TagGenSameTypeBitOps;} && CCallable<decltype(BitAnd), T, T>
[[nodiscard]] INTRA_FORCEINLINE constexpr auto operator&(T lhs, T rhs) {return T(TUnderlyingType<T>(lhs) & TUnderlyingType<T>(rhs));}

template<CEnum T> requires requires {T::TagGenSameTypeBitOps;} && CCallable<decltype(BitOr), T, T>
[[nodiscard]] INTRA_FORCEINLINE constexpr auto operator|(T lhs, T rhs) {return T(TUnderlyingType<T>(lhs) | TUnderlyingType<T>(rhs));}

template<CEnum T> requires requires {T::TagGenSameTypeBitOps;} && CCallable<decltype(BitXor), T, T>
[[nodiscard]] INTRA_FORCEINLINE constexpr auto operator^(T lhs, T rhs) {return T(TUnderlyingType<T>(lhs) ^ TUnderlyingType<T>(rhs));}

template<CEnum T> requires requires {T::TagGenSameTypeBitOps;} && CCallable<decltype(BitNot), T, T>
[[nodiscard]] INTRA_FORCEINLINE constexpr auto operator~(T v) {return T(~TUnderlyingType<T>(v));}


template<typename T1, typename T2> requires
	(CValueWrapper<T1> && CSameUnqualRef<T1, T2> && requires {TRemoveReference<T1>::TagGenSameTypeAddOps::True;} ||
	CValueWrapper<T1> && requires {TRemoveReference<T1>::TagGenMixedTypeAddOps::True;} ||
	CValueWrapper<T2> && requires {TRemoveReference<T2>::TagGenMixedTypeAddOps::True;}) &&
	z_D::CWrapperOpValid<decltype(Add), T1, T2>
[[nodiscard]] constexpr auto operator+(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(Add)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires
	(CValueWrapper<T1> && CSameUnqualRef<T1, T2> && requires {TRemoveReference<T1>::TagGenSameTypeAddOps::True;} ||
	CValueWrapper<T1> && requires {TRemoveReference<T1>::TagGenMixedTypeAddOps::True;} ||
	CValueWrapper<T2> && requires {TRemoveReference<T2>::TagGenMixedTypeAddOps::True;}) &&
	z_D::CWrapperOpValid<decltype(Sub), T1, T2>
[[nodiscard]] constexpr auto operator-(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(Sub)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires
	(CValueWrapper<T1> && CSameUnqualRef<T1, T2> && requires {TRemoveReference<T1>::TagGenSameTypeMulOps::True;} ||
	CValueWrapper<T1> && requires {TRemoveReference<T1>::TagGenMixedTypeMulOps::True;} ||
	CValueWrapper<T2> && requires {TRemoveReference<T2>::TagGenMixedTypeMulOps::True;}) &&
	z_D::CWrapperOpValid<decltype(Mul), T1, T2>
[[nodiscard]] constexpr auto operator*(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(Mul)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires
	(CValueWrapper<T1> && CSameUnqualRef<T1, T2> && requires {TRemoveReference<T1>::TagGenSameTypeMulOps::True;} ||
	CValueWrapper<T1> && requires {TRemoveReference<T1>::TagGenMixedTypeMulOps::True;} ||
	CValueWrapper<T2> && requires {TRemoveReference<T2>::TagGenMixedTypeMulOps::True;}) &&
	z_D::CWrapperOpValid<decltype(Div), T1, T2>
[[nodiscard]] constexpr auto operator/(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(Div)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires
	(CValueWrapper<T1> && CSameUnqualRef<T1, T2> && requires {TRemoveReference<T1>::TagGenSameTypeMulOps::True;} ||
	CValueWrapper<T1> && requires {TRemoveReference<T1>::TagGenMixedTypeMulOps::True;} ||
	CValueWrapper<T2> && requires {TRemoveReference<T2>::TagGenMixedTypeMulOps::True;}) &&
	z_D::CWrapperOpValid<TMod, T1, T2>
[[nodiscard]] constexpr auto operator%(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<TMod>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires
	(CValueWrapper<T1> && CSameUnqualRef<T1, T2> && requires {TRemoveReference<T1>::TagGenSameTypeBitOps::True;} ||
	CValueWrapper<T1> && requires {TRemoveReference<T1>::TagGenMixedTypeBitOps::True;} ||
	CValueWrapper<T2> && requires {TRemoveReference<T2>::TagGenMixedTypeBitOps::True;}) &&
	z_D::CWrapperOpValid<decltype(BitAnd), T1, T2>
[[nodiscard]] constexpr auto operator&(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(BitAnd)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires
	(CValueWrapper<T1> && CSameUnqualRef<T1, T2> && requires {TRemoveReference<T1>::TagGenSameTypeBitOps::True;} ||
	CValueWrapper<T1> && requires {TRemoveReference<T1>::TagGenMixedTypeBitOps::True;} ||
	CValueWrapper<T2> && requires {TRemoveReference<T2>::TagGenMixedTypeBitOps::True;}) &&
	z_D::CWrapperOpValid<decltype(BitOr), T1, T2>
[[nodiscard]] constexpr auto operator|(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(BitOr)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires
	(CValueWrapper<T1> && CSameUnqualRef<T1, T2> && requires {TRemoveReference<T1>::TagGenSameTypeBitOps::True;} ||
	CValueWrapper<T1> && requires {TRemoveReference<T1>::TagGenMixedTypeBitOps::True;} ||
	CValueWrapper<T2> && requires {TRemoveReference<T2>::TagGenMixedTypeBitOps::True;}) &&
	z_D::CWrapperOpValid<decltype(BitXor), T1, T2>
[[nodiscard]] constexpr auto operator^(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(BitXor)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}


template<class T> requires requires {TRemoveReference<T>::TagGenUnaryOps::True;}
[[nodiscard]] constexpr auto operator+(T&& x) {return INTRA_FWD(x);}

template<class T> requires requires {TRemoveReference<T>::TagGenUnaryOps::True;} && CHasOpSub<int, T>
[[nodiscard]] constexpr auto operator-(T&& x) {return 0 - INTRA_FWD(x);}

template<class T> requires requires {TRemoveReference<T>::TagGenUnaryOps::True;}
[[nodiscard]] constexpr auto operator++(T&& x) -> decltype(x += 1) {return x += 1;}

template<class T> requires requires {TRemoveReference<T>::TagGenUnaryOps::True;}
[[nodiscard]] constexpr auto operator--(T&& x) -> decltype(x -= 1) {return x -= 1;}

template<class T> requires requires {TRemoveReference<T>::TagGenUnaryOps::True;}
[[nodiscard]] constexpr auto operator++(T&& x, int) requires requires {++x;} {auto res = x; ++x; return res;}

template<class T> requires requires {TRemoveReference<T>::TagGenUnaryOps::True;}
[[nodiscard]] constexpr auto operator--(T&& x, int) requires requires {--x;} {auto res = x; --x; return res;}

template<typename T1, typename... T2> requires requires {TRemoveReference<T1>::TagGenOpAssign::True;}
constexpr auto operator+=(T1&& lhs, T2&&... rhs) noexcept(noexcept(lhs = INTRA_FWD(lhs) + (INTRA_FWD(rhs) + ...)))
	-> decltype(lhs = INTRA_FWD(lhs) + (INTRA_FWD(rhs) + ...)) {return lhs = INTRA_FWD(lhs) + (INTRA_FWD(rhs) + ...);}

template<typename T1, typename... T2> requires requires {TRemoveReference<T1>::TagGenOpAssign::True;}
constexpr auto operator-=(T1&& lhs, T2&&... rhs) noexcept(noexcept(lhs = INTRA_FWD(lhs) - (INTRA_FWD(rhs) + ...)))
	-> decltype(lhs = INTRA_FWD(lhs) - (INTRA_FWD(rhs) + ...)) {return lhs = INTRA_FWD(lhs) - (INTRA_FWD(rhs) + ...);}

template<typename T1, typename... T2> requires requires {TRemoveReference<T1>::TagGenOpAssign::True;}
constexpr auto operator*=(T1&& lhs, T2&&... rhs) noexcept(noexcept(lhs = INTRA_FWD(lhs) * (INTRA_FWD(rhs) + ...)))
	-> decltype(lhs = INTRA_FWD(lhs) * (INTRA_FWD(rhs) + ...)) {return lhs = INTRA_FWD(lhs) * (INTRA_FWD(rhs) + ...);}

template<typename T1, typename... T2> requires requires {TRemoveReference<T1>::TagGenOpAssign::True;}
constexpr auto operator/=(T1&& lhs, T2&&... rhs) noexcept(noexcept(lhs = INTRA_FWD(lhs) / (INTRA_FWD(rhs) + ...)))
	-> decltype(lhs = INTRA_FWD(lhs) / (INTRA_FWD(rhs) + ...)) {return lhs = INTRA_FWD(lhs) / (INTRA_FWD(rhs) + ...);}

template<typename T1, typename... T2> requires requires {TRemoveReference<T1>::TagGenOpAssign::True;}
constexpr auto operator%=(T1&& lhs, T2&&... rhs) noexcept(noexcept(lhs = INTRA_FWD(lhs) % (INTRA_FWD(rhs) + ...)))
	-> decltype(lhs = INTRA_FWD(lhs) % (INTRA_FWD(rhs) + ...)) {return lhs = INTRA_FWD(lhs) % (INTRA_FWD(rhs) + ...);}

template<typename T1, typename... T2> requires requires {TRemoveReference<T1>::TagGenOpAssign::True;}
constexpr auto operator&=(T1&& lhs, T2&&... rhs) noexcept(noexcept(lhs = INTRA_FWD(lhs) & (INTRA_FWD(rhs) + ...)))
	-> decltype(lhs = INTRA_FWD(lhs) & (INTRA_FWD(rhs) + ...)) {return lhs = INTRA_FWD(lhs) & (INTRA_FWD(rhs) + ...);}

template<typename T1, typename... T2> requires requires {TRemoveReference<T1>::TagGenOpAssign::True;}
constexpr auto operator|=(T1&& lhs, T2&&... rhs) noexcept(noexcept(lhs = INTRA_FWD(lhs) | (INTRA_FWD(rhs) + ...)))
	-> decltype(lhs = INTRA_FWD(lhs) | (INTRA_FWD(rhs) + ...)) {return lhs = INTRA_FWD(lhs) | (INTRA_FWD(rhs) + ...);}

template<typename T1, typename... T2> requires requires {TRemoveReference<T1>::TagGenOpAssign::True;}
constexpr auto operator^=(T1&& lhs, T2&&... rhs) noexcept(noexcept(lhs = INTRA_FWD(lhs) ^ (INTRA_FWD(rhs) + ...)))
	-> decltype(lhs = INTRA_FWD(lhs) ^ (INTRA_FWD(rhs) + ...)) {return lhs = INTRA_FWD(lhs) ^ (INTRA_FWD(rhs) + ...);}

template<typename T1, typename... T2> requires requires {TRemoveReference<T1>::TagGenOpAssign::True;}
constexpr auto operator<<=(T1&& lhs, T2&&... rhs) noexcept(noexcept(lhs = INTRA_FWD(lhs) << (INTRA_FWD(rhs) + ...)))
	-> decltype(lhs = INTRA_FWD(lhs) << (INTRA_FWD(rhs) + ...)) {return lhs = INTRA_FWD(lhs) << (INTRA_FWD(rhs) + ...);}

template<typename T1, typename... T2> requires requires {TRemoveReference<T1>::TagGenOpAssign::True;}
constexpr auto operator>>=(T1&& lhs, T2&&... rhs) noexcept(noexcept(lhs = INTRA_FWD(lhs) >> (INTRA_FWD(rhs) + ...)))
	-> decltype(lhs = INTRA_FWD(lhs) >> (INTRA_FWD(rhs) + ...)) {return lhs = INTRA_FWD(lhs) >> (INTRA_FWD(rhs) + ...);}



template<typename T1, typename T2> requires z_D::CGenCompareOp<decltype(Less), T1, T2>
[[nodiscard]] constexpr auto operator<(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(Less)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires z_D::CGenCompareOp<decltype(LEqual), T1, T2>
[[nodiscard]] constexpr auto operator<=(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(LEqual)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires z_D::CGenCompareOp<decltype(Greater), T1, T2>
[[nodiscard]] constexpr auto operator>(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(Greater)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires z_D::CGenCompareOp<decltype(GEqual), T1, T2>
[[nodiscard]] constexpr auto operator>=(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(GEqual)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires z_D::CGenCompareOp<decltype(Equal), T1, T2>
[[nodiscard]] constexpr auto operator==(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(Equal)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}

template<typename T1, typename T2> requires z_D::CGenCompareOp<decltype(NotEqual), T1, T2>
[[nodiscard]] constexpr auto operator!=(T1&& lhs, T2&& rhs)
{return z_D::ValueWrapperOpImpl<decltype(NotEqual)>(INTRA_FWD(lhs), INTRA_FWD(rhs));}
}

namespace z_D {
template<typename T> constexpr bool CIntegral_ = CBasicIntegral<T>;
template<typename T> requires CIntegral_<TWrappedType<T>> constexpr bool CIntegral_<T> = true;
template<typename T> constexpr bool CFloatingPoint_ = CBasicFloatingPoint<T>;
template<typename T> requires CFloatingPoint_<TWrappedType<T>> constexpr bool CFloatingPoint_<T> = true;
template<typename T> constexpr bool CSigned_ = CBasicSigned<T>;
template<typename T> requires CSigned_<TWrappedType<T>> constexpr bool CSigned_<T> = true;
}
template<typename T> concept CIntegral = z_D::CIntegral_<T>;
template<typename T> concept CFloatingPoint = z_D::CFloatingPoint_<T>;
template<typename T> concept CSigned = z_D::CSigned_<T>;

template<typename T, size_t N> struct Array
{
	T Elements[N];
	INTRA_FORCEINLINE constexpr T* Data() noexcept {return Elements;}
	INTRA_FORCEINLINE constexpr const T* Data() const noexcept {return Elements;}
	INTRA_FORCEINLINE constexpr index_t Length() const noexcept {return N;}
	template<CIntegral I> T& operator[](I index)
	{
		INTRA_PRECONDITION(index >= 0 && index < N);
		return Elements[size_t(index)];
	}
	template<CIntegral I> const T& operator[](I index) const
	{
		INTRA_PRECONDITION(index >= 0 && index < N);
		return Elements[size_t(index)];
	}
};
template<typename T, CSame<T>... Ts> Array(T, Ts...) -> Array<T, 1 + sizeof...(Ts)>;

} INTRA_END

