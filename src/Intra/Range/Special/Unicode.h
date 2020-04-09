#pragma once

#include "Intra/Container/Optional.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/StringView.h"

INTRA_BEGIN
namespace Unicode {
constexpr char32_t
	LeadingSurrogateStart = 0xD800,
	LeadingSurrogateEnd = 0xDBFF,
	TrailingSurrogateStart = 0xDC00,
	TrailingSurrogateEnd = 0xDFFF,
	ReplacementChar = 0xFFFD,
	MaxLegalCharCode = 0x10FFFF;
constexpr const char Utf8BOM[] = "\xef\xbb\xbf";
[[nodiscard]] constexpr byte SequenceBytes(byte i)
{
	const auto shift = unsigned(i & 0xF0) >> 2; //index (0-15) of the digit in the magic constant below, multiplied by 4
	if(sizeof(size_t) < 8) return byte(shift < 32? 1: (0x43220000u >> (shift - 32)));
	return (0x4322000011111111ULL >> shift) & 0xF;
}

[[nodiscard]] constexpr unsigned ValidContinuationBytes(unsigned i)
{
	return byte(0xE3000000U >> ((i & 0xF0) >> 3)) & 0xF;
}

[[nodiscard]] constexpr bool IsValidUtf8LeadingByte(byte i)
{
	return (i >> 6) != 2;
}

// @return a mask to extract significant bits from a leading byte that assuming that it is non-ASCII and valid
[[nodiscard]] constexpr uint32 LeadingByteMaskNotAscii(byte leadingByte)
{
	INTRA_PRECONDITION((leadingByte & 0x80) != 0);
	INTRA_PRECONDITION(IsValidUtf8LeadingByte(leadingByte));
	const auto shift = uint32(leadingByte & 0x30) >> (4-3); //0, 8, 16, 24
	return 0x070F1F1Fu >> shift;
}

[[nodiscard]] constexpr bool IsLeadingSurrogate(char32_t c) {return (c & 0xFFFD00) == 0xD800;}

// Generic UTF-8 decode function reading from any input range, checks all errors.
// Decodes one code point from the range.
template<class R> constexpr Requires<
	CInputRange<R> &&
	CPlainIntegral<TValueTypeOf<R>>,
char32_t> DecodeUtf8Codepoint(R&& range)
{
	auto c = byte(Next(range));
	if(c < 0x80) return c;
	if(c < 0xC0) return ReplacementChar;
	uint32 res = c & LeadingByteMaskNotAscii(c); //extract significant bits from leading byte
	const auto continuationBytes = ValidContinuationBytes(c);
	unsigned e = 0;
	for(unsigned i = 0; i < continuationBytes; i++)
	{
		if(range.Empty()) return ReplacementChar;
		c = byte(Next(range));
		res <<= 6;
		res |= c & 0x3F;
		e |= c & 0xC0;
		e >>= 2;
	}
	e |= uint32(res < (1 << (0x100B0700 >> (continuationBytes << 3)))) << 6; // non-canonical encoding
    e |= uint32((res >> 11) == 0x1B) << 7;  // surrogate half?
    e |= uint32(res > MaxLegalCharCode) << 8;
    e ^= 0x2a; // top two bits of each tail byte correct?
    e >>= 6 - (continuationBytes << 1);
	if(e) return ReplacementChar;
	return char32_t(res);
}

template<class R> constexpr Requires<
	CInputRange<R> &&
	CPlainIntegral<TValueTypeOf<R>>,
char32_t> DecodeUtf16Codepoint(R&& range)
{
	auto c = uint16(Next(range));
	if(c < LeadingSurrogateStart || c > LeadingSurrogateEnd) return char32_t(c);
	if(range.Empty()) return ReplacementChar;
	auto c2 = uint16(Next(range));
	if(c2 < TrailingSurrogateStart || c2 > TrailingSurrogateEnd) return ReplacementChar;
	return (char32_t(c - LeadingSurrogateStart) << 10) + char32_t(c2 - TrailingSurrogateStart) + 0x10000;
}

template<class R> constexpr Requires<
	CInputRange<R> &&
	CPlainIntegral<TValueTypeOf<R>> &&
	sizeof(TValueTypeOf<R>) <= sizeof(char16_t),
char32_t> DecodeCodepoint(R&& range)
{
	if(sizeof(TValueTypeOf<R>) == sizeof(char)) return DecodeUtf8Codepoint(range);
	return DecodeUtf16Codepoint(range);
}

template<class R> constexpr Requires<
	CInputRange<R> &&
	CPlainIntegral<TValueTypeOf<R>> &&
	sizeof(TValueTypeOf<R>) == sizeof(char16_t)
> EncodeCodepointAsUtf16(char32_t code, R&& range)
{
	using Char = TValueTypeOf<R>;
	if(code < LeadingSurrogateStart || code > TrailingSurrogateEnd && code <= 0xFFFF)
	{
		range.Put(Char(code));
		return;
	}
	if(code <= 0xFFFF || code > MaxLegalCharCode)
	{
		range.Put(Char(ReplacementChar));
		return;
	}
	const uint32 ch = code - 0x10000;
	range.Put(Char((ch >> 10) + LeadingSurrogateStart));
	range.Put(Char((ch & 0x3FF) + TrailingSurrogateStart));
}

template<class R> constexpr Requires<
	CInputRange<R> &&
	CPlainIntegral<TValueTypeOf<R>>,
index_t> EncodeCodepointAsUtf8(char32_t code, R&& range)
{
	using Char = TValueTypeOf<R>;
	if(code > MaxLegalCharCode) return WriteTo("\xEF\xBF\xBD", range);
	const size_t continuationBytes = (code >= 0x80) + (code >= 0x800) + (code >= 0x10000);
	unsigned shift = continuationBytes*6;
	const byte firstByteMark = byte(0xF0E0C000u >> (continuationBytes << 3));
	range.Put(Char(byte((code >> shift) | firstByteMark)));
	if(shift == 0) return 1;
	shift -= 6;
	range.Put(Char(byte(((code >> shift) | 0x80) & 0xBF)));
	if(shift == 0) return 2;
	shift -= 6;
	range.Put(Char(byte(((code >> shift) | 0x80) & 0xBF)));
	if(shift == 0) return 3;
	shift -= 6;
	range.Put(Char(byte(((code >> shift) | 0x80) & 0xBF)));
	return 4;
}
}

template<class R> class RUnicodeDecoder
{
	R mRange;
	char32_t mFirst = char32_t(0xFFFFFFFF);
public:
	static constexpr bool
		IsAnyInstanceFinite = CFiniteInputRange<R>,
		IsAnyInstanceInfinite = CInfiniteInputRange<R>;

	constexpr RUnicodeDecoder() = default;
	constexpr RUnicodeDecoder(R range): mRange(Move(range)) {if(!mRange.Empty()) mFirst = Unicode::DecodeCodepoint(mRange);}
	[[nodiscard]] constexpr char32_t First() const {INTRA_PRECONDITION(!Empty()); return mFirst;}
	constexpr void PopFirst() {mFirst = mRange.Empty()? char32_t(0xFFFFFFFF): Unicode::DecodeCodepoint(mRange);}
	[[nodiscard]] constexpr bool Empty() const {return mFirst == char32_t(0xFFFFFFFF);}
};

template<typename R> constexpr Requires<
	CAsAccessibleRange<R>,
RUnicodeDecoder<TRangeOf<R>>> ToUtf32(R&& range) {return {ForwardAsRange<R>(range)};}

template<class R, typename Char> class RUtf8Encoder
{
	R mRange;
	uint32 mFirstFour = 0;
	constexpr void setEmptyState() noexcept {mFirstFour = 0xFFFFFF80;}
	void readNextCodePoint()
	{
		Char four[4] = {Char(0x80), Char(0xFF), Char(0xFF), Char(0xFF)};
		Unicode::EncodeCodepointAsUtf8(Next(mRange), SpanOf(four));
		mFirstFour = Misc::BinaryDeserializeLE<uint32, Char>(four);
	}
public:
	static constexpr bool
		IsAnyInstanceFinite = CFiniteRange<R>,
		IsAnyInstanceInfinite = CInfiniteRange<R>;

	constexpr RUtf8Encoder() noexcept = default;
	constexpr RUtf8Encoder(R range) noexcept: mRange(Move(range))
	{
		if(mRange.Empty()) setEmptyState();
		else readNextCodePoint();
	}

	[[nodiscard]] constexpr Char First() const
	{
		INTRA_PRECONDITION(!Empty());
		return Char(mFirstFour & 0xFF);
	}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		if((mFirstFour & 0xFF00) != 0xFF00) mFirstFour = (mFirstFour >> 8)|0xFF000000u;
		else if(mRange.Empty()) setEmptyState();
		else readNextCodePoint();
	}
	constexpr bool Empty() const {return (mFirstFour & 0xFF) == 0x80;}
};

template<typename Char = char, typename R> constexpr Requires<
	CAsAccessibleRange<R>,
RUtf8Encoder<TRangeOf<R>, Char>> ToUtf8(R&& range) {return {ForwardAsRange<R>(range)};}

template<class R, typename Char> class RUtf16Encoder
{
	R mRange;
	Char mFirstTwo[2] = {};
	constexpr void setEmptyState() noexcept {mFirstTwo[1] = char16_t(Unicode::LeadingSurrogateStart);}
public:
	static constexpr bool
		IsAnyInstanceFinite = CFiniteRange<R>,
		IsAnyInstanceInfinite = CInfiniteRange<R>;

	constexpr RUtf16Encoder() noexcept = default;
	constexpr RUtf16Encoder(R range) noexcept: mRange(Move(range))
	{
		if(mRange.Empty()) setEmptyState();
		else Unicode::EncodeCodepointAsUtf16(Next(mRange), SpanOf(mFirstTwo));
	}

	[[nodiscard]] constexpr char32_t First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mFirstTwo[0];
	}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		if(Unicode::IsLeadingSurrogate(mFirstTwo[0]))
		{
			mFirstTwo[0] = mFirstTwo[1];
			return;
		}
		if(!mRange.Empty())
		{
			Unicode::EncodeCodepointAsUtf16(Next(mRange), SpanOf(mFirstTwo));
			return;
		}
		setEmptyState();
	}
	constexpr bool Empty() const {return mFirstTwo[1] == Char(Unicode::LeadingSurrogateStart);}
};

template<typename Char = char16_t, typename R> constexpr Requires<
	CAsAccessibleRange<R>,
RUtf16Encoder<TRangeOf<R>, Char>> ToUtf16(R&& range) {return {ForwardAsRange<R>(range)};}
INTRA_END
