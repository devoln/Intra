#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Meta/Type.h"

#include "Utils/Span.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Funal {

//! Арифметические операции
//!{

namespace detail {
struct TArith {constexpr forceinline TArith() noexcept {}};
struct TPred1 {constexpr forceinline TPred1() noexcept {}};
struct TPred2 {constexpr forceinline TPred2() noexcept {}};
struct TUnary {constexpr forceinline TUnary() noexcept {}};
}

#define INTRA_DEFINE_ARITH_OP(name, op) struct T ## name: detail::TArith {\
	constexpr forceinline T ## name() noexcept {}\
	template<typename T> constexpr forceinline T operator()(const T& a, const T& b) const {return op;}\
}; constexpr static const T ## name name{}

INTRA_DEFINE_ARITH_OP(Add, a + b);
INTRA_DEFINE_ARITH_OP(Sub, a - b);
INTRA_DEFINE_ARITH_OP(RSub, b - a);
INTRA_DEFINE_ARITH_OP(Mul, a * b);
INTRA_DEFINE_ARITH_OP(Div, a / b);
INTRA_DEFINE_ARITH_OP(RDiv, b / a);
INTRA_DEFINE_ARITH_OP(Mod, a % b);
INTRA_DEFINE_ARITH_OP(RMod, b % a);

INTRA_DEFINE_ARITH_OP(Min, a < b? a: b);
INTRA_DEFINE_ARITH_OP(Max, b < a? a: b);

INTRA_DEFINE_ARITH_OP(And, a & b);
INTRA_DEFINE_ARITH_OP(Or, a | b);
INTRA_DEFINE_ARITH_OP(Xor, a ^ b);

INTRA_DEFINE_ARITH_OP(LShift, a << b);
INTRA_DEFINE_ARITH_OP(RShift, a >> b);

#undef INTRA_DEFINE_ARITH_OP
//!}

//! Операции сравнения

#define INTRA_DEFINE_PREDICATE2(name, op) struct T ## name: detail::TPred2 {\
	constexpr forceinline T ## name() noexcept {}\
	template<typename T1, typename T2 = T1> constexpr forceinline bool operator()(const T1& a, const T2& b) const {return op;}\
}; constexpr static const T ## name name{}

INTRA_DEFINE_PREDICATE2(Less, a < b);
INTRA_DEFINE_PREDICATE2(LEqual, a <= b);
INTRA_DEFINE_PREDICATE2(Greater, a > b);
INTRA_DEFINE_PREDICATE2(GEqual, a >= b);
INTRA_DEFINE_PREDICATE2(Equal, a == b);
INTRA_DEFINE_PREDICATE2(NotEqual, a != b);

template<typename P> struct TNot1: P
{
	constexpr forceinline TNot1(P pred): P(Cpp::Move(pred)) {}
	template<typename T> constexpr forceinline bool operator()(const T& a) const {return !P::operator()(a);}
};

template<typename P> struct TNot2: P
{
	constexpr forceinline TNot2(P pred): P(Cpp::Move(pred)) {}
	template<typename T1, typename T2> constexpr forceinline bool operator()(const T1& a, const T2& b) const {return !P::operator()(a, b);}
};

template<typename P> constexpr forceinline P operator!(TNot1<P> p) {return p;}
template<typename P> constexpr forceinline P operator!(TNot2<P> p) {return p;}

template<typename P> constexpr forceinline Meta::EnableIf<
	Meta::IsInherited<P, detail::TPred1>::_,
TNot1<Meta::RemoveConstRef<P>>> operator!(P&& p) {return Cpp::Forward<P>(p);}

template<typename P> constexpr forceinline Meta::EnableIf<
	Meta::IsInherited<P, detail::TPred2>::_,
TNot2<Meta::RemoveConstRef<P>>> operator!(P&& p) {return Cpp::Forward<P>(p);}


template<typename P1, typename P2> struct TAnd1: P1, P2
{
	constexpr forceinline TAnd1(P1 pred1, P2 pred2): P1(Cpp::Move(pred1)), P2(Cpp::Move(pred2)) {}
	template<typename T> constexpr forceinline bool operator()(const T& a) const {return P1::operator()(a) && P2::operator()(a);}
};

template<typename P1, typename P2> struct TAnd2: P1, P2
{
	constexpr forceinline TAnd2(P1 pred1, P2 pred2): P1(Cpp::Move(pred1)), P2(Cpp::Move(pred2)) {}
	template<typename T1, typename T2> constexpr forceinline bool operator()(const T1& a, const T2& b) const {return P1::operator()(a, b) && P2::operator()(a, b);}
};

template<typename P1, typename P2> constexpr forceinline Meta::EnableIf<
	Meta::IsInherited<P1, detail::TPred1>::_ &&
	Meta::IsInherited<P2, detail::TPred1>::_,
TAnd1<Meta::RemoveConstRef<P1>, Meta::RemoveConstRef<P2>>> operator&&(P1&& p1, P2&& p2)
{return {Cpp::Forward<P1>(p1), Cpp::Forward<P2>(p2)};}

template<typename P1, typename P2> constexpr forceinline Meta::EnableIf<
	Meta::IsInherited<P1, detail::TPred2>::_ &&
	Meta::IsInherited<P2, detail::TPred2>::_,
TAnd2<Meta::RemoveConstRef<P1>, Meta::RemoveConstRef<P2>>> operator&&(P1&& p1, P2&& p2)
{return {Cpp::Forward<P1>(p1), Cpp::Forward<P2>(p2)};}


template<typename P1, typename P2> struct TOr1: P1, P2
{
	constexpr forceinline TOr1(P1 pred1, P2 pred2): P1(Cpp::Move(pred1)), P2(Cpp::Move(pred2)) {}
	template<typename T> constexpr forceinline bool operator()(const T& a) const {return P1::operator()(a) && P2::operator()(a);}
};

template<typename P1, typename P2> struct TOr2: P1, P2
{
	constexpr forceinline TOr2(P1 pred1, P2 pred2): P1(Cpp::Move(pred1)), P2(Cpp::Move(pred2)) {}
	template<typename T1, typename T2> constexpr forceinline bool operator()(const T1& a, const T2& b) const {return P1::operator()(a, b) && P2::operator()(a, b);}
};

template<typename P1, typename P2> constexpr forceinline Meta::EnableIf<
	Meta::IsInherited<Meta::RemoveConstRef<P1>, detail::TPred1>::_ &&
	Meta::IsInherited<Meta::RemoveConstRef<P2>, detail::TPred1>::_,
TOr1<Meta::RemoveConstRef<P1>, Meta::RemoveConstRef<P2>>> operator||(P1&& p1, P2&& p2)
{return {Cpp::Forward<P1>(p1), Cpp::Forward<P2>(p2)};}

template<typename P1, typename P2> constexpr forceinline Meta::EnableIf<
	Meta::IsInherited<Meta::RemoveConstRef<P1>, detail::TPred2>::_ &&
	Meta::IsInherited<Meta::RemoveConstRef<P2>, detail::TPred2>::_,
TOr2<Meta::RemoveConstRef<P1>, Meta::RemoveConstRef<P2>>> operator||(P1&& p1, P2&& p2)
{return {Cpp::Forward<P1>(p1), Cpp::Forward<P2>(p2)};}


//! Различные предикаты
#define INTRA_DEFINE_PREDICATE1(name, op) struct T ## name: detail::TPred1 {\
	constexpr forceinline T ## name() noexcept {}\
	template<typename T> constexpr forceinline bool operator()(const T& a) const {return op;}\
}; constexpr static const T ## name name{};

INTRA_DEFINE_PREDICATE1(IsEven, (a & 1) == 0);
INTRA_DEFINE_PREDICATE1(IsOdd, (a & 1) != 0);
INTRA_DEFINE_PREDICATE1(IsHorSpace, a == ' ' || a == '\t');
INTRA_DEFINE_PREDICATE1(IsLineSeparator, a == ' \r' || a == '\n');
constexpr static const auto IsSpace = IsHorSpace || IsLineSeparator;
INTRA_DEFINE_PREDICATE1(IsAnySlash, a == '\\' || a == '/');

struct TTrue
{
	constexpr forceinline TTrue() noexcept {}
	template<typename... Args> constexpr forceinline bool operator()(Args&&...) const {return true;}
};
constexpr static const TTrue True{};

struct TFalse
{
	constexpr forceinline TFalse() noexcept {}
	template<typename... Args> constexpr forceinline bool operator()(Args&&...) const {return false;}
};
constexpr static const TFalse False{};


#define INTRA_DEFINE_UNARY_OP(name, op) struct T ## name {\
	constexpr forceinline T ## name() noexcept {}\
	template<typename T> constexpr forceinline T operator()(const T& a) const {return op;}\
}; constexpr static const T ## name name{}

INTRA_DEFINE_UNARY_OP(ToLowerAscii, T((unsigned(a - 'A') > 'Z' - 'A')? a: a + ('a' - 'A')));
INTRA_DEFINE_UNARY_OP(ToUpperAscii, T((unsigned(a - 'a') > 'z' - 'a')? a: a - ('a' - 'A')));

INTRA_DEFINE_PREDICATE1(IsDigit, uint(a - '0') <= '9');

namespace Comparers {
	template<typename T> using Function = bool(*)(const T& a, const T& b);

	template<typename COMPARER, typename T, typename I> struct IndexedComparer
	{
		COMPARER comparer;
		CSpan<T> values;
		IndexedComparer(COMPARER c, CSpan<T> vals): comparer(c), values(vals) {}
		forceinline bool operator()(const I& a, const I& b) const {return comparer(values[a], values[b]);}
	};

	template<typename T, typename I> using IndexedFunctionComparer = IndexedComparer<Function<T>, T, I>;
}

}}

INTRA_WARNING_POP
