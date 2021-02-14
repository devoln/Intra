﻿#pragma once

#include <Intra/Range.h>
#include <Intra/Math.h>
#include <Intra/Number.h>

namespace Intra { INTRA_BEGIN

constexpr auto IsHorSpace = [](auto&& a) {return a == ' ' || a == '\t';};
constexpr auto IsLineSeparator = [](auto&& a) {return a == '\r' || a == '\n';};
constexpr auto IsSpace = [](auto&& a) {return IsHorSpace(a) || IsLineSeparator(a);};
constexpr auto IsAnySlash = [](auto&& a) {return a == '\\' || a == '/';};
constexpr auto IsDigit = [](auto&& a) {return '0' <= a && a <= '9';};
constexpr auto IsUpperLatin = [](auto&& a) {return 'A' <= a && a <= 'Z';};
constexpr auto IsLowerLatin = [](auto&& a) {return 'a' <= a && a <= 'z';};
constexpr auto IsLatin = [](auto&& a) {return IsUpperLatin(a) || IsLowerLatin(a);};
constexpr auto IsAsciiChar = [](auto&& a) {return unsigned(a) <= 127;};
constexpr auto IsAsciiControlChar = [](auto& a) {return unsigned(a) <= 31 || a == 127;};

constexpr auto ToLowerAscii = [](auto a) {return decltype(a)(IsUpperLatin(a)? a + ('a' - 'A'): a);};
constexpr auto ToUpperAscii = [](auto a) {return decltype(a)(IsLowerLatin(a)? a - ('a' - 'A'): a);};

constexpr index_t UintToString100(unsigned x, char* dst)
{
	INTRA_PRECONDITION(x < 100);
	if(x < 10)
	{
		*dst = char((x | 0x30) & 0xFF);
		return 1;
	}
	uint32 low = x;
	uint32 ll = ((uint32(low) * 103) >> 9) & 0x1E;
	low += ll * 3;
	ll = ((low & 0xF0) >> 4) | ((low & 0x0F) << 8);
	ll |= 0x3030;
	BinarySerialize<uint16>(Unsafe, uint16(ll), dst);
	return 2;
}

constexpr index_t UintToString10k(unsigned x, char* dst)
{
	if(x < 100) return UintToString100(x, dst);

	uint32 low = x;
	uint32 ll = FastDiv<100>(low);
	low -= ll * 100;

	low = (low << 16) | ll;

	// Two divisions by 10 (14 bits needed)
	ll = ((low * 103) >> 9) & 0x1E001E;
	low += ll * 3;

	ll = ((low & 0x00F000F0) >> 4) | ((low & 0x000F000F) << 8); // move digits into correct spot
	ll |= 0x30303030; // digits -> ASCII digit codes

	BinarySerialize<uint32>(Unsafe, ll >> (x > 999? 0: 8), dst);
	return x > 999? 4: 3;
}

constexpr uint64 UintToString100mAsUInt64(uint32 x)
{
	const uint32 ll0 = FastDiv<10000>(x);
	uint64 low = FastMod<10000>(x) | (uint64(ll0) << 32);

	// Four divisions and remainders by 100
	uint64 ll = ((low * 5243) >> 19) & 0x000000FF000000FF;
	low -= ll * 100;
	low = (low << 16) | ll;

	// Eight divisions by 10 (14 bits needed)
	ll = ((low * 103) >> 9) & 0x001E001E001E001E;
	low += ll * 3;

	// move digits into correct spot
	ll = ((low & 0x00F000F000F000F0) >> 4) | (low & 0x000F000F000F000F) << 8;
	ll = (ll >> 32) | (ll << 32);

	ll |= 0x3030303030303030;  // digits -> ASCII digit codes
	return ll;
}

constexpr index_t UintToString(uint32 x, char* dst)
{
	if(x < 10'000) return UintToString10k(x, dst);
	index_t digits = 0;

	if(x < 100'000'000)
	{
		if(x >= 1'000'000) digits = x >= 10'000'000? 8: 7;
		else digits = x >= 100'000? 6: 5;
	}
	else
	{
		const uint32 high = FastDiv<100'000'000>(x);
		x -= high * 100'000'000;
		digits = UintToString100(high, dst); // two digit version since `high` <= 42
		dst += digits;
		digits += 8;
	}

	uint64 packedStr = UintToString100mAsUInt64(x);
	if(x < 100'000'000) packedStr >>= (8 - digits)*8; //skip leading zeros if any
	BinarySerialize<uint64>(Unsafe, packedStr, dst);
	return digits;
}

constexpr index_t UintToString(uint64 x, char* dst)
{
	if(x <= MaxValueOf<uint32>) return UintToString(uint32(x), dst); //boil down to 32-bit version without 64-bit divisions that are slow on 32-bit platforms
	const uint64 xdiv100m = FastDiv<100000000>(x);
	const uint64 xmod100m = x - xdiv100m * 100000000;
	const index_t d = UintToString(xdiv100m, dst); //recursion: print highest part first
	const uint64 packedStr = UintToString100mAsUInt64(xmod100m);
	BinarySerialize<uint64>(Unsafe, packedStr, dst + d);
	return d + 8;
}

static_assert([] {
	char buf[24]{};
	UintToString(1234567890123456, buf);
	return __builtin_strcmp(buf, "1234567890123456") == 0;
}());

constexpr void UintToHexString(uint32 num, char* dst, bool lowerAlpha)
{
	uint64 x = num;

	// isolate each hex-digit into its own byte and prepare them for little-endian copy
	// Ex: 0x1234FACE => 0x0E0C0A0F04030201
	x = ((x & 0xFFFF) << 32) | ((x & 0xFFFF0000) >> 16);
	x = ((x & 0x0000FF000000FF00) >> 8) | (x & 0x000000FF000000FF) << 16;
	x = ((x & 0x00F000F000F000F0) >> 4) | (x & 0x000F000F000F000F) << 8;

	// Create a bitmask of bytes containing alpha hex digits
	// - add 6 to each digit
	// - if the digit is a high alpha hex digit, then the addition
	//   will overflow to the high nibble of the byte
	// - shift the high nibble down to the low nibble and mask
	//   to create the relevant bitmask
	//
	// Using above example:
	// 0x0E0C0A0F04030201 + 0x0606060606060606 = 0x141210150a090807
	// >> 4 == 0x0141210150a09080 & 0x0101010101010101
	// == 0x0101010100000000
	const uint64 mask = ((x + 0x0606060606060606) >> 4) & 0x0101010101010101;

	// convert to ASCII numeral characters
	x |= 0x3030303030303030;

	// if there are high hexadecimal characters, need to adjust
	// for uppercase alpha hex digits, need to add 0x07
	//   to move 0x3A-0x3F to 0x41-0x46 (A-F)
	// for lowercase alpha hex digits, need to add 0x27
	//   to move 0x3A-0x3F to 0x61-0x66 (a-f)
	// it's actually more expensive to test if mask non-nullptr
	//   and then run the following stmt
	x += (lowerAlpha? 0x27: 0x07) * mask;

	BinarySerialize<uint64>(Unsafe, x, dst);
}

constexpr void UintToHexString(uint64 num, char* dst, bool lowerAlpha)
{
	UintToHexString(uint32(num >> 32), dst, lowerAlpha);
	UintToHexString(uint32(num), dst+8, lowerAlpha);
}

static_assert([] {
	char buf[24]{};
	UintToHexString(0x0123456789ABCDEF, buf);
	return __builtin_strcmp(buf, "0123456789ABCDEF") == 0;
}());



template<typename T> constexpr index_t FloatToStringScientific(GenericFloat<T, 10> x, char decimalSep, char e, char* dst)
{
	index_t len = UintToString(x.Mantissa, dst+1);
	dst[0] = dst[1];
	if(len > 1)
	{
		dst[1] = decimalSep;
		len++;
	}

	int exp = x.Exponent - int(len - 1);
	if(exp == 0) return len;
	dst[len++] = e;
	if(exp < 0)
	{
		exp = -exp;
		dst[len++] = '-';
	}
	len += UintToString(uint32(exp), dst+len);
	return len;
}

template<typename T> constexpr index_t FloatToString(GenericFloat<T, 10> x, char decimalSep, char e, char* dst)
{
	enum {
		MantissaDigits = int(sizeof(T)*5+1)/2,
		LeadingZerosBeforeSciFormat = 4
	};
	const bool fixedReprIsTooLong = MantissaDigits < x.Exponent || x.Exponent < -MantissaDigits - LeadingZerosBeforeSciFormat;
	if(fixedReprIsTooLong) return DecimalToStringScientific(x, decimalSep, e, dst);
	index_t len = UintToString(x.Mantissa, dst+1);
	index_t lenOfFixedFormat = len;
	if(x.Exponent >= 0) lenOfFixedFormat += x.Exponent;
	else
	{
		lenOfFixedFormat += 1;
		if(-x.Exponent >= len) lenOfFixedFormat += 1 - (len + x.Exponent);
	}
	dst[0] = dst[1];
	if(len > 1)
	{
		dst[1] = decimalSep;
		len++;
	}

	int exp = x.Exponent - int(len - 1);
	if(exp == 0) return len;
	dst[len++] = e;
	if(exp < 0)
	{
		exp = -exp;
		dst[len++] = '-';
	}
	len += UintToString(uint32(exp), dst+len);
	return len;
}


struct FormatParams
{
	char PaddingChar = ' ';
	char DecimalSeparator = '.';
	char ThousandSeparator = ' ';

	bool Exp: 1 = false;
	bool OtherFlags : 7;

	uint8 MinDigits = 0;
	uint8 MinWidth = 0;
};

template<COutputCharRange R, CBasicArithmetic X>
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

template<COutputCharRange OR, typename T> requires(!CBasicArithmetic<TRemoveReference<T>>)
constexpr void ToString(OR&& dst, T&& v) {dst << INTRA_FWD(v);}

template<COutputCharRange R, CBasicIntegral X> constexpr void ToString(R&& dst, X number, int minWidth, char filler=' ', unsigned base=10)
{
	if constexpr(CCeilCounter<R>)
	{
		index_t maxLog = sizeof(X)*2;
		if(base < 8) maxLog = sizeof(X)*8;
		else if(base < 16) maxLog = (sizeof(X)*8+2)/3;
		dst.PopFirstCount(Max(maxLog + CBasicSigned<X>, index_t(minWidth)));
	}
	else if constexpr(CBasicSigned<X>)
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

template<COutputCharRange R, CBasicIntegral X> constexpr void ToString(R&& dst, X x)
{
	if constexpr(CCeilCounter<R>)
	{
		dst.PopFirstCount(CChar<X>? 1:
			CBasicSigned<X> + (sizeof(X) < 2? 3: sizeof(X) == 2? 5: sizeof(X) <= 4? 10: sizeof(X) <= 8? 18: 35));
	}
	else if constexpr(CChar<X>) dst.Put(x);
	else if constexpr(CBasicSigned<X>)
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

template<COutputCharRange R, CBasicUnsignedIntegral X> constexpr void ToStringHexInt(R&& dst, X number)
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

template<COutputCharRange R> constexpr void ToString(R&& dst, decltype(nullptr)) {ToString(dst, "nullptr"_v);}

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

template<COutputCharRange R, CBasicFloatingPoint X> constexpr void ToString(R&& dst, X number,
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



template<CCharRange R> requires(!CConst<R>)
constexpr bool ParseSignAdvance(R& src)
{
	bool minus = false;
	while(!src.Empty())
	{
		if(src.First()=='-') minus = !minus;
		else if(src.First() != '+' && !IsSpace(src.First())) break;
		src.PopFirst();
	}
	return minus;
}

template<CBasicUnsignedIntegral X, CCharRange R> requires(!CConst<R>)
constexpr X ParseAdvance(R& src)
{
	TrimLeftAdvance(src, IsSpace);
	X result = 0;
	while(!src.Empty())
	{
		unsigned digit = unsigned(src.First())-'0';
		if(digit>9) break;
		result = X(result*10+digit);
		src.PopFirst();
	}
	return result;
}

template<CBasicSignedIntegral X, CCharRange R> requires(!CConst<R>)
constexpr X ParseAdvance(R& src)
{
	const bool minus = ParseSignAdvance(src);
	X result = X(ParseAdvance<TToUnsigned<X>>(src));
	return minus? X(-result): result;
}

template<CBasicFloatingPoint X, CCharRange R> constexpr X ParseAdvance(R& src, TRangeValue<R> decimalSeparator='.')
{
	X result = 0, pos = 1;
	bool waspoint = false;

	bool minus = ParseSignAdvance(src);

	for(; !src.Empty(); src.PopFirst())
	{
		TRangeValue<R> c = src.First();
		if(c==decimalSeparator && !waspoint)
		{
			waspoint = true;
			continue;
		}
		unsigned digit = unsigned(c)-'0';
		if(digit>9) break;

		if(!waspoint) result = X(result*10+X(digit));
		else pos*=10, result += X(digit)/X(pos);
	}
	return minus? -result: result;
}

template<CChar X, CCharRange R> requires(!CConst<R>)
constexpr X ParseAdvance(R& src)
{
	X result = src.First();
	src.PopFirst();
	return result;
}

/// Сопоставляет начало исходного потока src со stringToExpect, игнорируя пробелы в начале src и stringToExpect.
/// В случае совпадения сдвигает начало src в конец вхождения stringToExpect, а stringToExpect сдвигает в конец, делая его пустым.
/// В случае несовпадения удаляет только пробелы из начала обоих потоков.
/// @return Возвращает true в случае совпадения src и stringToExpect.
template<CCharRange R, CCharRange CR> requires (!CConst<R>) && (!CConst<CR>)
constexpr bool ExpectAdvance(R& src, CR& stringToExpect)
{
	TrimLeftAdvance(src, IsSpace);
	TrimLeftAdvance(stringToExpect, IsSpace);
	auto srcCopy = src;
	if(StartsAdvanceWith(srcCopy, stringToExpect))
	{
		src = srcCopy;
		return true;
	}
	return false;
}

template<CBasicIntegral X, CCharList L> requires CConsumableList<L>
[[nodiscard]] constexpr X Parse(R&& src)
{
	auto range = ForwardAsRange<R>(src);
	return ParseAdvance<X>(range);
}

template<CBasicFloatingPoint X, CCharList L> requires CConsumableList<L>
[[nodiscard]] constexpr X Parse(R&& src, TListValue<L> decimalSeparator = '.')
{
	auto range = ForwardAsRange<R>(src);
	return ParseAdvance<X>(range, decimalSeparator);
}

template<CCharRange R, CCallable<TRangeValue<R>> P1, CCallable<TRangeValue<R>> P2>
constexpr TTakeResult<R> ParseIdentifierAdvance(R& src, P1 isNotIdentifierFirstChar, P2 isNotIdentifierChar)
{
	TrimLeftAdvance(src, IsHorSpace);
	if(src.Empty() || isNotIdentifierFirstChar(src.First())) return {};
	auto result = src;
	src.PopFirst();
	while(!src.Empty() && !isNotIdentifierChar(src.First())) src.PopFirst();
	return Take(result, DistanceTo(result, src));
}

template<CCharRange R, typename X> requires
	(!CConst<TRemoveReference<X>>) &&
	CBasicArithmetic<TRemoveReference<X>> &&
	(!CConst<TRemoveReference<X>>)
R&& operator>>(R&& stream, X&& dst)
{
	dst = ParseAdvance<TRemoveReference<X>>(stream);
	return INTRA_FWD(stream);
}

template<CCharRange R, CCharList CR> requires(!CConst<R>)
constexpr R&& operator>>(R&& stream, CR&& stringToExpect)
{
	auto stringToExpectCopy = ForwardAsRange<CR>(stringToExpect);
	ExpectAdvance(stream, stringToExpectCopy);
	return Forward<R>(stream);
}

} INTRA_END
