#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/StringView.h"
#include "Intra/Range/Operations.h"

#include "Intra/Range/Mutation/Copy.h"


INTRA_BEGIN

struct FormatParams
{
	char PaddingChar[4];
	char DecimalSeparator;
	char ThousandSeparator;
	uint8 Flags;
	uint8 MinDigits;
	uint8 MinWidth;
};

template<COutputCharRange R, CArithmetic X>
constexpr R&& operator<<(R&& dst, X x)
{
	ToString(dst, x);
	return INTRA_FWD(dst);
}

template<COutputCharRange R, CConsumableList R2> requires CChar<TListValue<R2>>
constexpr R&& operator<<(R&& dst, R2&& src)
{
	ForwardAsRange<R2>(src)|WriteTo(dst);
	return INTRA_FWD(dst);
}

template<COutputCharRange R, typename Collection, CCharList SR, CCharList LR, CCharList RR> requires
	CConsumableList<Collection> && (!CChar<TListValue<Collection>>) || CStaticLengthContainer<Collection>
constexpr void ToString(R&& dst, Collection&& collection, SR&& separator, LR&& lBracket, RR&& rBracket)
{
	ForwardAsRange<LR>(lBracket)|WriteTo(dst);
	if constexpr(CConsumableList<Collection> && !CChar<TListValue<Collection>>)
	{
		auto range = ForwardAsRange<Collection>(collection);
		if(!range.Empty()) dst << Next(range);
		while(!range.Empty())
		{
			separator|WriteTo(dst);
			dst << Next(range);
		}
	}
	else if constexpr(CStaticLengthContainer<Collection>)
	{
		INTRA_FWD(collection)|ForEachField([&, first = true](const auto& value) mutable {
			if(!first) separator|WriteTo(dst);
			dst << value;
			first = false;
		});
	}
	ForwardAsRange<RR>(rBracket)|WriteTo(dst);
}

template<COutputCharRange R, typename Collection>
requires CConsumableList<Collection> && (!CChar<TListValue<Collection>>) || CStaticLengthContainer<Collection>
constexpr R&& operator<<(R&& dst, Collection&& r)
{
	ToString(dst, INTRA_FWD(collection), ", "_v, "["_v, "]"_v);
	return INTRA_FWD(dst);
}

template<COutputCharRange OR, typename T> requires(!CArithmetic<TRemoveReference<T>>)
constexpr void ToString(OR&& dst, T&& v) {dst << INTRA_FWD(v);}

template<COutputCharRange R, CIntegral X> constexpr void ToString(R&& dst, X number, int minWidth, char filler=' ', unsigned base=10)
{
	if constexpr(CCeilCounter<R>)
	{
		index_t maxLog = sizeof(X)*2;
		if(base < 8) maxLog = sizeof(X)*8;
		else if(base < 16) maxLog = (sizeof(X)*8+2)/3;
		dst.PopFirstCount(Max(maxLog + CSigned<X>, index_t(minWidth)));
	}
	else if constexpr(CSigned<X>)
	{
		if(number < 0)
		{
			if(FullOpt(dst).GetOr(false)) return;
			dst.Put('-');
			minWidth--;
		}
		ToString(dst, TToUnsigned<X>(Abs(number)), minWidth, filler, base);
	}
	else
	{
		INTRA_PRECONDITION(base >= 2 && base <= 36);
		char reversed[64];
		char* rev = reversed;
		do *rev++ = "0123456789abcdefghijklmnopqrstuvwxyz"[number % base], number = X(number / base);
		while(number != 0);
		for(int i = 0, s = int(minWidth - (rev - reversed)); i < s; i++)
		{
			if(FullOpt(dst).GetOr(false)) return;
			dst.Put(filler);
		}
		while(rev != reversed && !FullOpt(dst).GetOr(false)) dst.Put(*--rev);
	}
}

template<COutputCharRange R, CIntegral X> constexpr void ToString(R&& dst, X x)
{
	if constexpr(CCeilCounter<R>)
	{
		dst.PopFirstCount(CChar<X>? 1:
			CSigned<X> + (sizeof(X) < 2? 3: sizeof(X) == 2? 5: sizeof(X) <= 4? 10: sizeof(X) <= 8? 18: 35));
	}
	else if constexpr(CChar<X>) dst.Put(x);
	else if constexpr(CSigned<X>)
	{
		if(x < 0)
		{
			if(FullOpt(dst).GetOr(false)) return;
			dst.Put('-');
			x = X(-x);
		}
		ToString(dst, TToUnsigned<X>(x));
	}
	else if constexpr(sizeof(X) >= sizeof(size_t))
	{
		char reversed[20];
		char* rev = reversed;
		do *rev++ = char(x % 10 + '0'), x /= 10;
		while(x != 0);
		while(rev != reversed && !FullOpt(dst).GetOr(false)) dst.Put(*--rev);
	}
	else ToString(dst, size_t(x));
}

template<COutputCharRange R, CUnsignedIntegral X> constexpr void ToStringHexInt(R&& dst, X number)
{
	if constexpr(CCeilCounter<R>)
	{
		dst.PopFirstCount(sizeof(X*)*2);
		return;
	}
	index_t digitPos = index_t(sizeof(X) * 2);
	while(digitPos-- && !FullOr(dst))
	{
		int value = int(number >> (digitPos*4)) & 15;
		if(value > 9) value += 'A'-10;
		else value += '0';
		dst.Put(TRangeValue<R>(value));
	}
}


template<COutputCharRange R, typename X> INTRA_FORCEINLINE void ToString(R&& dst, X* pointer)
{
	ToStringHexInt(dst, size_t(pointer));
}

template<COutputCharRange R> constexpr void ToString(R&& dst, decltype(null)) {ToString(dst, "null"_v);}

template<COutputCharRange R> void ToStringReal(R&& dst, long double number, int preciseness=15,
	char dot='.', bool appendAllDigits=false)
{
	if constexpr(CCeilCounter<R>)
	{
		dst.PopFirstCount(20 + 1 + (preciseness + 1));
		return;
	}

	if(number == NaN)
	{
		"NaN"_span|WriteTo(dst);
		return;
	}
	if(number == Infinity)
	{
		"Infinity"_span|WriteTo(dst);
		return;
	}
	if(number == -Infinity)
	{
		"-Infinity"_span|WriteTo(dst);
		return;
	}

	//TODO: use Ryu algorithm
	if(number < 0)
	{
		if(FullOpt(dst).GetOr(false)) return;
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

	if(FullOpt(dst).GetOr(false)) return;
	dst.Put(dot);
	do
	{
		if(FullOpt(dst).GetOr(false)) return;
		fractional *= 10;
		int digit = int(fractional);
		fractional -= digit;
		if(fractional > 0.99) fractional = 0, digit++;
		dst.Put(char('0' + digit));
	} while((fractional >= 0.01 || appendAllDigits) && --preciseness > 0);
}

template<COutputCharRange R, CFloatingPoint X> constexpr void ToString(R&& dst, X number,
	int preciseness = sizeof(X) <= 4? 7: 15, char dot = '.', bool appendAllDigits = false)
{
	ToStringReal(dst, number, preciseness, dot, appendAllDigits);
}

template<COutputCharRange R> constexpr void ToString(R&& dst, bool value)
{
	if constexpr(CCeilCounter<R>)
	{
		dst.PopFirstCount(5);
		return;
	}

	INTRA_PRECONDITION(byte(value) <= 1);
	const char* str = value? "true": "false";
	while(*str != '\0' && !FullOpt(dst).GetOr(false)) dst.Put(*str++);
}

INTRA_END
