#pragma once

#include "Core/Numeric.h"
#include "Core/Type.h"
#include "Core/EachField.h"

#include "Core/Assert.h"

#include "Core/Range/Concepts.h"

#include "Core/Range/Operations.h"
#include "Core/Range/Count.h"
#include "Core/Range/Repeat.h"
#include "Core/Range/Mutation/Copy.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_SIGN_CONVERSION
template<typename R, typename Char> Requires<
	COutputCharRange<R> &&
	CChar<Char> &&
	!CHasFull<R>
> ToString(R&& dst, const Char* str)
{while(*str != '\0') dst.Put(*str++);}

template<typename R, typename Char> Requires<
	COutputCharRange<R> &&
	CChar<Char> &&
	CHasFull<R>
> ToString(R&& dst, const Char* str)
{while(*str != '\0' && !dst.Full()) dst.Put(*str++);}

template<typename R, typename X> Requires<
	COutputCharRange<R> &&
	CUnsignedIntegral<X>
> ToString(R&& dst, X number, int minWidth, char filler=' ', uint base=10, char minus='\0')
{
	INTRA_DEBUG_ASSERT(base >= 2 && base <= 36);
	char reversed[64];
	char* rev = reversed;
	do *rev++ = "0123456789abcdefghijklmnopqrstuvwxyz"[number % base], number = X(number / base);
	while(number != 0);
	if(minus) minWidth--;
	for(int i=0, s=int(minWidth - (rev - reversed)); i<s; i++)
	{
		if(FullOr(dst)) return;
		dst.Put(filler);
	}
	if(FullOr(dst)) return;
	if(minus) dst.Put(minus);
	while(rev != reversed && !FullOr(dst)) dst.Put(*--rev);
}

template<typename R, typename X> Requires<
	COutputCharRange<R> &&
	CUnsignedIntegral<X> &&
	sizeof(X) >= sizeof(size_t)
> ToString(R&& dst, X number)
{
	char reversed[20];
	char* rev = reversed;
	do *rev++ = char(number % 10 + '0'), number /= 10;
	while(number != 0);
	while(rev != reversed && !FullOr(dst)) dst.Put(*--rev);
}

template<typename R, typename X> Requires<
	COutputCharRange<R> &&
	CUnsignedIntegral<X> &&
	sizeof(X) < sizeof(size_t)
> ToString(R&& dst, X number)
{ToString(dst, size_t(number));}


template<typename R, typename X> forceinline Requires<
	COutputCharRange<R> &&
	CSignedIntegral<X>
> ToString(R&& dst, X number, int minWidth, TValueTypeOf<R> filler=' ', uint base=10)
{
	ToString(dst, TMakeUnsigned<X>(number < 0? -number: number),
		minWidth, filler, base, number<0? '-': '\0');
}


template<typename R, typename X> Requires<
	COutputCharRange<R> &&
	CSignedIntegral<X> &&
	!CChar<X>
> ToString(R&& dst, X number)
{
	if(number < 0)
	{
		if(FullOr(dst)) return;
		dst.Put('-');
		number = X(-number);
	}
	ToString(dst, TMakeUnsigned<X>(number));
}

template<typename R, typename X, typename=Requires<
	COutputCharRange<R> &&
	CUnsignedIntegral<X>
>> void ToStringHexInt(R&& dst, X number)
{
	intptr digitPos = intptr(sizeof(X) * 2);
	while(digitPos --> 0 && !FullOr(dst))
	{
		int value = int(number >> (digitPos*4)) & 15;
		if(value > 9) value += 'A'-10;
		else value += '0';
		dst.Put(TValueTypeOf<R>(value));
	}
}


template<typename R, typename X> forceinline Requires<
	COutputCharRange<R> &&
	!CChar<X>
> ToString(R&& dst, X* pointer)
{ToStringHexInt(dst, size_t(pointer));}

template<typename R, typename X> forceinline Requires<
	COutputCharRange<R>
> ToString(R&& dst, null_t)
{ToString(dst, "null");}

template<typename R> Requires<
	COutputCharRange<R>
> ToStringReal(R&& dst, long double number, int preciseness=15,
	char dot='.', bool appendAllDigits=false)
{
	if(number == NaN)
	{
		WriteTo("NaN", dst);
		return;
	}
	if(number == Infinity)
	{
		WriteTo("Infinity", dst);
		return;
	}
	if(number == -Infinity)
	{
		WriteTo("-Infinity", dst);
		return;
	}

	if(number < 0)
	{
		if(FullOr(dst)) return;
		dst.Put('-');
		number = -number;
	}

	const uint64 integralPart = uint64(number);
	long double fractional = number - static_cast<long double>(integralPart);
	if(fractional > 0.99)
	{
		ToString(dst, integralPart+1);
		fractional = 0;
	}
	else ToString(dst, integralPart);

	if(preciseness == 0) return;

	if(FullOr(dst)) return;
	dst.Put(dot);
	do
	{
		if(FullOr(dst)) return;
		fractional *= 10;
		int digit = int(fractional);
		fractional -= digit;
		if(fractional > 0.99) fractional = 0, digit++;
		dst.Put(char('0' + digit));
	} while((fractional >= 0.01 || appendAllDigits) && --preciseness > 0);
}

template<typename R, typename X> Requires<
	COutputCharRange<R> &&
	CFloatingPoint<X>
> ToString(R&& dst, X number, int preciseness=sizeof(X)<=4? 7: 15,
	char dot='.', bool appendAllDigits=false)
{ToStringReal(dst, number, preciseness, dot, appendAllDigits);}


template<typename R, typename Char, typename = Requires<
	COutputCharRange<R> &&
	CChar<Char>
>> forceinline void ToString(R&& dst, Char character, size_t repeat=1)
{
	while(repeat --> 0) dst.Put(character);
}

template<typename R> Requires<
	COutputCharRange<R>
> ToString(R&& dst, bool value)
{
	INTRA_DEBUG_ASSERT(byte(value) <= 1);
	const char* str = value? "false": "true";
	while(*str != '\0' && !FullOr(dst)) dst.Put(*str++);
}
INTRA_END
