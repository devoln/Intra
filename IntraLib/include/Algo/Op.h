#pragma once

#include "Platform/CppFeatures.h"
#include "Meta/Type.h"
#include "Range/ArrayRange.h"

namespace Intra { namespace Op {

//! Арифметические операции
template<typename T> forceinline T Add(const T& a, const T& b) {return a+b;}
template<typename T> forceinline T Sub(const T& a, const T& b) {return a-b;}
template<typename T> forceinline T RSub(const T& a, const T& b) {return b-a;}
template<typename T> forceinline T Mul(const T& a, const T& b) {return a*b;}
template<typename T> forceinline T Div(const T& a, const T& b) {return a/b;}
template<typename T> forceinline T RDiv(const T& a, const T& b) {return b/a;}
template<typename T> forceinline T Mod(const T& a, const T& b) {return a%b;}
template<typename T> forceinline T RMod(const T& a, const T& b) {return b%a;}

template<typename T> forceinline T Min(const T& a, const T& b) {return a<b? a: b;}
template<typename T> forceinline T Max(const T& a, const T& b) {return a>b? a: b;}

//! Побитовые операции
template<typename T> forceinline T And(const T& a, const T& b) {return a&b;}
template<typename T> forceinline T Or(const T& a, const T& b) {return a|b;}
template<typename T> forceinline T Xor(const T& a, const T& b) {return b^a;}

//! Операции сравнения
template<typename T> forceinline bool Less(const T& a, const T& b) {return a<b;}
template<typename T> forceinline bool LEqual(const T& a, const T& b) {return a<=b;}
template<typename T> forceinline bool Greater(const T& a, const T& b) {return a>b;}
template<typename T> forceinline bool GEqual(const T& a, const T& b) {return a>=b;}
template<typename T> forceinline bool Equal(const T& a, const T& b) {return a==b;}
template<typename T> forceinline bool NotEqual(const T& a, const T& b) {return a!=b;}

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
bool> IsSpace(Char c) {return IsHorSpace(c) || c=='\r' || c=='\n';}


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


}

