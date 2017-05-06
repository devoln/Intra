#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/InfNan.h"

#include "Meta/Type.h"
#include "Meta/EachField.h"

#include "Utils/Op.h"
#include "Utils/Debug.h"

#include "Concepts/Range.h"

#include "Range/Operations.h"
#include "Range/Generators/Count.h"
#include "Range/Generators/Repeat.h"
#include "Range/Mutation/Copy.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION

template<typename R, typename Char, size_t N> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_
> ToString(R&& dst, const Char(&str)[N])
{CopyToAdvance(str, dst);}

template<typename R, typename Char> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_
> ToString(R&& dst, const Char* str)
{while(*str!='\0') dst.Put(*str++);}

template<typename R, typename X> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::IsUnsignedIntegralType<X>::_
> ToString(R&& dst, X number, int minWidth, char filler=' ', uint base=10, char minus='\0')
{
	INTRA_DEBUG_ASSERT(base>=2 && base<=36);
	char reversed[64];
	char* rev = reversed;
	do *rev++ = "0123456789abcdefghijklmnopqrstuvwxyz"[number%base], number = X(number/base);
	while(number!=0);
	if(minus) minWidth--;
	for(int i=0, s=int(minWidth-(rev-reversed)); i<s; i++)
		dst.Put(filler);
	if(minus) dst.Put(minus);
	while(rev != reversed) dst.Put(*--rev);
}

template<typename R, typename X> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::IsUnsignedIntegralType<X>::_ &&
	sizeof(X) >= sizeof(size_t)
> ToString(R&& dst, X number)
{
	char reversed[20];
	char* rev = reversed;
	do *rev++ = char(number%10+'0'), number/=10;
	while(number!=0);
	while(rev!=reversed) dst.Put(*--rev);
}

template<typename R, typename X> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::IsUnsignedIntegralType<X>::_ &&
	sizeof(X)<sizeof(size_t)
> ToString(R&& dst, X number)
{
	ToString(dst, size_t(number));
}


template<typename R, typename X> forceinline Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::IsSignedIntegralType<X>::_
> ToString(R&& dst, X number, int minWidth, Concepts::ValueTypeOf<R> filler=' ', uint base=10)
{
	ToString(dst, Meta::MakeUnsignedType<X>(number<0? -number: number),
		minWidth, filler, base, number<0? '-': '\0');
}


template<typename R, typename X> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_&&
	Meta::IsSignedIntegralType<X>::_
> ToString(R&& dst, X number)
{
	if(number<0)
	{
		dst.Put('-');
		number = X(-number);
	}
	ToString(dst, Meta::MakeUnsignedType<X>(number));
}

template<typename R, typename X, typename=Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::IsUnsignedIntegralType<X>::_
>> void ToStringHexInt(R&& dst, X number)
{
	intptr digitPos = intptr(sizeof(X)*2);
	while(digitPos --> 0)
	{
		int value = int(number >> (digitPos*4)) & 15;
		if(value>9) value += 'A'-10;
		else value += '0';
		dst.Put(Concepts::ValueTypeOf<R>(value));
	}
}


template<typename R, typename X> forceinline Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	!Meta::IsCharType<X>::_
> ToString(R&& dst, X* pointer)
{ToStringHexInt(dst, reinterpret_cast<size_t>(pointer));}


template<typename R> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_
> ToStringReal(R&& dst, real number, int preciseness=15,
	char dot='.', bool appendAllDigits=false)
{
	if(number == Cpp::NaN)
	{
		CopyToAdvance("NaN", dst);
		return;
	}
	if(number == Cpp::Infinity)
	{
		CopyToAdvance("Infinity", dst);
		return;
	}
	if(number == -Cpp::Infinity)
	{
		CopyToAdvance("-Infinity", dst);
		return;
	}

	if(number < 0)
	{
		dst.Put('-');
		number = -number;
	}

	const ulong64 integralPart = ulong64(number);
	real fractional = number-real(integralPart);
	if(fractional>0.99)
	{
		ToString(dst, integralPart+1);
		fractional=0;
	}
	else ToString(dst, integralPart);

	if(preciseness==0) return;

	dst.Put(dot);
	do
	{
		fractional *= 10;
		int digit = int(fractional);
		fractional -= digit;
		if(fractional>0.99) fractional=0, digit++;
		dst.Put(char('0'+digit));
	} while((fractional>=0.01 || appendAllDigits) && --preciseness>0);
}

template<typename R, typename X> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::IsFloatType<X>::_
> ToString(R&& dst, X number, int preciseness=sizeof(X)<=4? 7: 15,
	char dot='.', bool appendAllDigits=false)
{ToStringReal(dst, number, preciseness, dot, appendAllDigits);}


static_assert(!Meta::IsCharType<float>::_, "ERROR!");
template<typename R, typename Char, typename = Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_ &&
	Meta::IsCharType<Char>::_
>> forceinline void ToString(R&& dst, Char character, size_t repeat=1)
{
	while(repeat --> 0) dst.Put(character);
}

template<typename R> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_
> ToString(R&& dst, bool value)
{
	INTRA_DEBUG_ASSERT(byte(value)<=1);
	static const char* const boolStr[2] = {"false", "true"};
	const char* str = boolStr[value!=false];
	while(*str!='\0') dst.Put(*str++);
}

template<typename R> Meta::EnableIf<
	Concepts::IsOutputCharRange<R>::_
> ToString(R&& dst, null_t)
{
	const char* str = "null";
	while(*str!='\0') dst.Put(*str++);
}


INTRA_WARNING_POP

}}
