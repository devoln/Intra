#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Meta/Type.h"
#include "Range/Generators/ArrayRange.h"

namespace Intra { namespace Op {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Арифметические операции
//!{
template<typename T> forceinline T Add(const T& a, const T& b) {return a+b;}
template<typename T> forceinline T Sub(const T& a, const T& b) {return a-b;}
template<typename T> forceinline T RSub(const T& a, const T& b) {return b-a;}
template<typename T> forceinline T Mul(const T& a, const T& b) {return a*b;}
template<typename T> forceinline T Div(const T& a, const T& b) {return a/b;}
template<typename T> forceinline T RDiv(const T& a, const T& b) {return b/a;}
template<typename T> forceinline T Mod(const T& a, const T& b) {return a%b;}
template<typename T> forceinline T RMod(const T& a, const T& b) {return b%a;}
//!}

template<typename T> forceinline T Min(const T& a, const T& b) {return a<b? a: b;}
template<typename T> forceinline T Max(const T& a, const T& b) {return a>b? a: b;}

//! Побитовые операции
template<typename T> forceinline T And(const T& a, const T& b) {return a&b;}
template<typename T> forceinline T Or(const T& a, const T& b) {return a|b;}
template<typename T> forceinline T Xor(const T& a, const T& b) {return b^a;}

//! Операции сравнения
template<typename T1, typename T2=T1> forceinline bool Less(const T1& a, const T2& b) {return a<b;}
template<typename T1, typename T2=T1> forceinline bool LEqual(const T1& a, const T2& b) {return a<=b;}
template<typename T1, typename T2=T1> forceinline bool Greater(const T1& a, const T2& b) {return a>b;}
template<typename T1, typename T2=T1> forceinline bool GEqual(const T1& a, const T2& b) {return a>=b;}
template<typename T1, typename T2=T1> forceinline bool Equal(const T1& a, const T2& b) {return a==b;}
template<typename T1, typename T2=T1> forceinline bool NotEqual(const T1& a, const T2& b) {return a!=b;}

//! Различные предикаты
template<typename T> forceinline Meta::EnableIf<
	Meta::IsArithmeticType<T>::_,
bool> IsEven(const T& value) {return (value & 1) == 0;}

template<typename T> forceinline Meta::EnableIf<
	Meta::IsArithmeticType<T>::_,
bool> IsOdd(const T& value) {return (value & 1) != 0;}

template<typename Char> forceinline Meta::EnableIf<
	Meta::IsCharType<Char>::_,
bool> IsHorSpace(Char c) {return c==' ' || c=='\t';}

template<typename Char> forceinline Meta::EnableIf<
	Meta::IsCharType<Char>::_,
bool> IsLineSeparator(Char c) {return c=='\r' || c=='\n';}

template<typename Char> forceinline Meta::EnableIf<
	Meta::IsCharType<Char>::_,
bool> IsSpace(Char c) {return IsHorSpace(c) || IsLineSeparator(c);}

template<typename T> forceinline bool TruePredicate(const T&) {return true;}
template<typename T> forceinline bool FalsePredicate(const T&) {return false;}


//! Операции с символами
template<typename Char> forceinline Meta::EnableIf<
	Meta::IsCharType<Char>::_,
Char> ToLowerAscii(Char c)
{
	if(unsigned(c-'A')>'Z'-'A') return c;
	return c+('a'-'A');
}

template<typename Char> forceinline Meta::EnableIf<
	Meta::IsCharType<Char>::_,
Char> ToUpperAscii(Char c)
{
	if(unsigned(c-'a')>'z'-'a') return c;
	return Char(int(c)-('a'-'A'));
}

template<typename Char> forceinline Meta::EnableIf<
	Meta::IsCharType<Char>::_,
Char> IsDigit(Char c)
{return uint(c-'0')<='9';}

}

namespace Comparers
{
	template<typename T> using Function = bool(*)(const T& a, const T& b);

	template<typename COMPARER, typename T, typename I> struct Indexed
	{
		COMPARER comparer;
		ArrayRange<T> values;
		Indexed(COMPARER c, ArrayRange<T> vals): comparer(c), values(vals) {}
		bool operator()(const I& a, const I& b) const { return comparer(values[a], values[b]); }
	};

	template<typename T, typename I> using IndexedFunction = Indexed<Function<T>, T, I>;
}

INTRA_WARNING_POP

}
