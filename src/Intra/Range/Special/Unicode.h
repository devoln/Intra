#pragma once

#include "Intra/Range/Span.h"

namespace Intra { INTRA_BEGIN
namespace Unicode {

namespace z_D {
template<typename Char> constexpr index_t ArrayCountValidAsciiBlocks16(Span<const Char> range)
{
	const auto* ptr = range.Data();
	for(index_t i = range.Length() >> 4; i--; ptr += 16)
	{
		auto a = BinaryDeserializePlatformSpecific<uint64>(Unsafe, ptr);
		a |= BinaryDeserializePlatformSpecific<uint64>(Unsafe, ptr+8);
		if constexpr(sizeof(void*) == 8)
		{
			if(a & 0x8080808080808080ull) break;
		}
		else
		{
			const uint32 b = uint32(a)|uint32(a >> 32);
			if(b & 0x80808080u) break;
		}
	}
	return ptr - range.Data();
}

template<typename Char> size_t ArrayCountValidAsciiBlocks32BranchlessSimd(Span<const Char> range)
{
	const auto* ptr = range.Data();
	//TODO: write branchless SIMD loop for 32-byte blocks. 
	//for(size_t i = range.Length() >> 5; i--; ptr += 32)
	{
		
	}
	//TODO: use _mm_movemask_epi8 to extract MSBs
	return size_t(ptr - range.Data());
}
}

template<CCharRange R> constexpr bool PopWhileValidAscii(R& range)
{
	if constexpr(!Config::DisableAllOptimizations && CArrayList<R> && sizeof(TArrayListValue<R>) == 1)
	{
		const auto span = ConstSpanOf(range);
		const auto numAsciiCharsToSkip = z_D::ArrayCountValidAsciiBlocks16(span);
		range|PopFirstExactly(size_t(numAsciiCharsToSkip));
	}
	while(!range.Empty() && IsAsciiChar(range.First())) range.PopFirst();
	return range.Empty();
}

constexpr auto IsValidAscii = []<CCharList L>(L&& list)
{
	auto range = RangeOf(INTRA_FWD(list));
	if constexpr(!Config::DisableAllOptimizations && CArrayList<L> && sizeof(TListValue<L>) == 1)
	{
		if(!IsConstantEvaluated())
		{
			const auto span = ConstSpanOf(range);
			const size_t numAsciiCharsToSkip = z_D::ArrayCountValidAsciiBlocks32BranchlessSimd(span);
			range|PopFirstExactly(numAsciiCharsToSkip);
		}
	}
	return PopWhileValidAscii(range);
};

constexpr char32_t
	LeadingSurrogateStart = 0xD800,
	LeadingSurrogateEnd = 0xDBFF,
	TrailingSurrogateStart = 0xDC00,
	TrailingSurrogateEnd = 0xDFFF,
	ReplacementChar = 0xFFFD,
	MaxLegalCharCode = 0x10FFFF;

constexpr const char Utf8BOM[] = "\xef\xbb\xbf";

[[nodiscard]] constexpr byte Utf8SequenceBytes(byte i)
{
	const auto shift = unsigned(i & 0xF0) >> 2; //index (0-15) of the digit in the magic constant below, multiplied by 4
	if constexpr(sizeof(size_t) < 8) return byte((shift < 32? 1: (0x43220000u >> (shift - 32))) & 0xF);
	else return byte((0x4322000011111111ULL >> shift) & 0xF);
}

[[nodiscard]] constexpr int Utf8ContinuationBytes(int i)
{
	return int((0xE5000000U >> (unsigned(i & 0xF0) >> 3)) & 3);
}

[[nodiscard]] constexpr auto IsUtf8ContinuationByte = [](byte i) {return (i >> 6) == 2;};

/// @return a mask to extract significant bits from a leading byte assuming that it is non-ASCII and valid
[[nodiscard]] constexpr uint32 LeadingByteMaskNotAscii(byte leadingByte)
{
	INTRA_PRECONDITION(!IsAsciiChar(leadingByte));
	INTRA_PRECONDITION(!IsUtf8ContinuationByte(leadingByte));
	const auto shift = uint32(leadingByte & 0x30) >> (4-3); //0, 8, 16, 24
	return 0x070F1F1Fu >> shift;
}


constexpr auto IsUtf16Surrogate = [](char16_t c) {return (c & 0xF800) == 0xD800;};
constexpr auto IsUtf16LeadingSurrogate = [](char16_t c) {return (c & 0xFC00) == 0xD800;};
constexpr auto IsUtf16TrailingSurrogate = [](char16_t c) {return (c & 0xFC00) == 0xDC00;};

template<CCharRange R> [[nodiscard]] constexpr bool PopWhileValidUtf16(R& range)
{
	for(; !range.Empty(); range.PopFirst())
	{
		const char16_t codeUnit1 = char16_t(range.First());
		if(!IsUtf16Surrogate(codeUnit1)) continue;
		if(IsUtf16TrailingSurrogate(codeUnit1)) return false;
		range.PopFirst();
		if(range.Empty()) return false;
	}
	return true;
}

constexpr auto IsValidUtf16 = []<CCharList L>(L&& list)
{
	auto range = RangeOf(INTRA_FWD(list));
	return PopWhileValidUtf16(range);
};

// Generic UTF-8 decode function reading from any input range, checks all errors.
// Decodes one code point from the range.
template<CCharRange R> constexpr char32_t DecodeUtf8Codepoint(R&& range, Optional<uint32&> oError = Undefined)
{
	byte c = byte(Next(range));
	if(c < 0x80)
	{
		if(oError) oError.Unwrap() = 0;
		return c;
	}
	if(c < 0xC0)
	{
		if(oError) oError.Unwrap() = 0x200;
		return ReplacementChar;
	}
	uint32 res = c & LeadingByteMaskNotAscii(c); //extract significant bits from leading byte
	const auto continuationBytes = Utf8ContinuationBytes(c);
	unsigned e = 0;
	for(int i = 0; i < continuationBytes; i++)
	{
		if(range.Empty())
		{
			if(oError) oError.Unwrap() = uint32(i << 12);
			return ReplacementChar;
		}
		c = byte(Next(range));
		res <<= 6;
		res |= c & 0x3F;
		e |= c & 0xC0;
		e >>= 2;
	}
	e ^= 0x2a; // top two bits of each tail byte correct?
	e |= uint32(res < (1u << ((0x100B0700u >> uint8(continuationBytes*8)) & 0xFF))) << 6; // non-canonical encoding
	e |= uint32((res >> 11) == 0x1B) << 7;  // surrogate half?
	e |= uint32(res > MaxLegalCharCode) << 8;
	e >>= 6 - (continuationBytes << 1);

	if(oError) oError.Unwrap() = e;
	if(e)
	{
		INTRA_FATAL_ERROR("err");
		return ReplacementChar;
	}
	return char32_t(res);
}

template<CCharRange R> [[nodiscard]] constexpr bool PopWhileValidUtf8(R& range)
{
	//PopWhileValidAscii(range);
	while(!range.Empty())
	{
		uint32 error = 0;
		DecodeUtf8Codepoint(range, OptRef(error));
		if(error) return false;
	}
	return true;
}

constexpr auto IsValidUtf8 = []<CCharList L>(L&& str)
{
	if constexpr(CArrayList<L> && sizeof(TListValue<L>) == 1)
	{
		//const auto* const data = DataOf(r);
		//const auto len = LengthOf(r);
		if(!IsConstantEvaluated())
		{
			//Optimize with SIMD: https://github.com/cyb70289/utf8
		}
	}
	auto range = RangeOf(INTRA_FWD(str));
	return PopWhileValidUtf8(range);
}

constexpr auto IsValidUtf32 = []<CCharList R>(R&& r)
{
	return true;
}

constexpr auto IsValidUnicode = []<CCharList L>(L&& list)
{
	if constexpr(sizeof(TListValue<L>) == 1) return IsValidUtf8(INTRA_FWD(list));
	else if constexpr(sizeof(TListValue<L>) == 2) return IsValidUtf16(INTRA_FWD(list));
	else if constexpr(sizeof(TListValue<L>) == 4) return IsValidUtf32(INTRA_FWD(list));
	return false;
}


template<CCharRange R> constexpr char32_t DecodeUtf16Codepoint(R&& range)
{
	auto c = char16_t(Next(range));
	if(!IsUtf16LeadingSurrogate(c)) return char32_t(c);
	if(range.Empty()) return ReplacementChar;
	auto c2 = char16_t(Next(range));
	if(!IsUtf16TrailingSurrogate(c2)) return ReplacementChar;
	return (char32_t(c - LeadingSurrogateStart) << 10) + char32_t(c2 - TrailingSurrogateStart) + 0x10000;
}

template<CRange R> requires CChar<TRangeValue<R>> &&
	(sizeof(TRangeValue<R>) <= sizeof(char16_t))
constexpr char32_t DecodeCodepoint(R&& range)
{
	if constexpr(sizeof(TRangeValue<R>) == sizeof(char)) return DecodeUtf8Codepoint(range);
	return DecodeUtf16Codepoint(range);
}

template<COutputOf<char16_t> R> constexpr void EncodeCodepointAsUtf16(char32_t code, R&& range)
{
	if(code <= 0xFFFF && !IsUtf16Surrogate(char16_t(code)))
	{
		range|Put(char16_t(code));
		return;
	}
	if(code <= 0xFFFF || code > MaxLegalCharCode)
	{
		range|Put(char16_t(ReplacementChar));
		return;
	}
	const uint32 ch = code - 0x10000;
	range|Put(char16_t((ch >> 10) + LeadingSurrogateStart));
	range|Put(char16_t((ch & 0x3FF) + TrailingSurrogateStart));
}

template<COutputOf<char8_t> R> constexpr index_t EncodeCodepointAsUtf8(char32_t code, R&& range)
{
	if(code > MaxLegalCharCode) return "\xEF\xBF\xBD"_span|WriteTo(range);
	const size_t continuationBytes = (code >= 0x80) + (code >= 0x800) + (code >= 0x10000);
	unsigned shift = continuationBytes*6;
	const byte firstByteMark = byte(0xF0E0C000u >> (continuationBytes << 3));
	range|Put(char8_t(byte((code >> shift) | firstByteMark)));
	if(shift == 0) return 1;
	shift -= 6;
	range|Put(char8_t(byte(((code >> shift) | 0x80) & 0xBF)));
	if(shift == 0) return 2;
	shift -= 6;
	range|Put(char8_t(byte(((code >> shift) | 0x80) & 0xBF)));
	if(shift == 0) return 3;
	shift -= 6;
	range|Put(char8_t(byte(((code >> shift) | 0x80) & 0xBF)));
	return 4;
}
}

template<class R> class RUnicodeDecoder
{
	R mRange;
	char32_t mFirst = char32_t(0xFFFFFFFF);
public:
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	constexpr RUnicodeDecoder() = default;
	constexpr RUnicodeDecoder(R range): mRange(Move(range))
	{
		if(!mRange.Empty()) mFirst = DecodeCodepoint(mRange);
	}

	[[nodiscard]] constexpr char32_t First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mFirst;
	}
	constexpr void PopFirst()
	{
		mFirst = mRange.Empty()? char32_t(0xFFFFFFFF): Unicode::DecodeCodepoint(mRange);
	}
	[[nodiscard]] constexpr bool Empty() const {return mFirst == char32_t(0xFFFFFFFF);}
};

template<CAccessibleList L> constexpr RUnicodeDecoder<TRangeOf<L>> ToUtf32(L&& list) {return {RangeOf(INTRA_FWD(list))};}

template<class R> class RUtf8Encoder
{
	R mRange;
	uint32 mFirstFour = 0;
	constexpr void setEmptyState() noexcept {mFirstFour = 0xFFFFFF80;}
	void readNextCodePoint()
	{
		char8_t four[4] = {0x80, 0xFF, 0xFF, 0xFF};
		Unicode::EncodeCodepointAsUtf8(mRange|Next, Span(four));
		mFirstFour = BinaryDeserializeLE<uint32>(Unsafe, four);
	}
public:
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	constexpr RUtf8Encoder() noexcept = default;
	constexpr RUtf8Encoder(R range) noexcept: mRange(INTRA_MOVE(range))
	{
		if(mRange.Empty()) setEmptyState();
		else readNextCodePoint();
	}

	[[nodiscard]] constexpr char8_t First() const
	{
		INTRA_PRECONDITION(!Empty());
		return char8_t(mFirstFour & 0xFF);
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

template<CAccessibleList L> constexpr auto ToUtf8(L&& list) {return RUtf8Encoder{RangeOf(INTRA_FWD(list))};}

template<class R> class RUtf16Encoder
{
	R mRange;
	char16_t mFirstTwo[2] = {};
	constexpr void setEmptyState() noexcept {mFirstTwo[1] = char16_t(Unicode::LeadingSurrogateStart);}
public:
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	constexpr RUtf16Encoder() noexcept = default;
	constexpr RUtf16Encoder(R range) noexcept: mRange(Move(range))
	{
		if(mRange.Empty()) setEmptyState();
		else EncodeCodepointAsUtf16(Next(mRange), Span(mFirstTwo));
	}

	[[nodiscard]] constexpr char16_t First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mFirstTwo[0];
	}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		if(IsUtf16LeadingSurrogate(mFirstTwo[0]))
		{
			mFirstTwo[0] = mFirstTwo[1];
			return;
		}
		if(!mRange.Empty())
		{
			EncodeCodepointAsUtf16(Next(mRange), Span(mFirstTwo));
			return;
		}
		setEmptyState();
	}
	[[nodiscard]] constexpr bool Empty() const {return mFirstTwo[1] == char16_t(LeadingSurrogateStart);}
};

template<CAccessibleList R> constexpr RUtf16Encoder<TRangeOf<L>> ToUtf16(L&& list) {return {RangeOf(INTRA_FWD(list))};}

#if INTRA_CONSTEXPR_TEST
static_assert(Unicode::Utf8ContinuationBytes(208) == 1);
static_assert(Unicode::Utf8SequenceBytes(208) == 2);
static_assert(Unicode::IsValidUtf8(""));
static_assert(Unicode::IsValidUtf8("ascii-test"));
static_assert(Unicode::IsValidUtf16(L"ascii-test"));
static_assert(Unicode::IsValidUtf8(u8"тест"));
static_assert(!Unicode::IsValidUtf8("abc\x87xyz"));
static_assert(Unicode::IsValidUtf16(u"тестовая строка"));
static_assert(Unicode::IsValidUtf8(u8"тесто"));
static_assert(Span(u8"о").Length() == 3);
static_assert(Unicode::IsValidUtf8(u8"о"));
static_assert(Unicode::IsValidUtf8(u8"тестовая строка."));
#endif

} INTRA_END
