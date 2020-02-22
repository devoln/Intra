#pragma once

#include "Core/Range/Span.h"
#include "Core/Assert.h"

INTRA_BEGIN
//! Set of ASCII chars useful for fast belonging checks in parsers.
/*!
  Implementation note: this class could be implemented via SBitset<128> but this version is
  simpler and faster when compiler optimizations are disabled.
*/
class AsciiSet
{
	uint64 v[2];

	friend struct TAsciiSets;

	constexpr forceinline AsciiSet(uint64 v1, uint64 v2): v{v1, v2} {}
public:
	constexpr forceinline AsciiSet(null_t=null) noexcept: v{0,0} {}

	template<uint N> constexpr forceinline AsciiSet(const char(&chars)[N]) noexcept: v{0,0} {Set(CSpan<char>(chars));}

	explicit constexpr forceinline AsciiSet(CSpan<char> chars): v{0,0} {Set(chars);}

	constexpr forceinline AsciiSet& operator=(null_t) {v[0] = v[1] = 0; return *this;}
	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const noexcept {return v[0] == 0 && v[1] == 0;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

	INTRA_NODISCARD constexpr forceinline bool operator==(const AsciiSet& rhs) const noexcept {return v[0] == rhs.v[0] && v[1] == rhs.v[1];}
	INTRA_NODISCARD constexpr forceinline bool operator!=(const AsciiSet& rhs) const noexcept {return !operator==(rhs);}

	//! Check if this set contains element c.
	//! c must be ASCII, otherwise the behavior is undefined.
	INTRA_NODISCARD constexpr bool operator[](char c) const
	{
		return INTRA_DEBUG_ASSERT(byte(c)<128),
			(v[byte(c)/64] & (1ull << (size_t(c) & 63))) != 0;
	}

	//! Check if the set contains element c. Returns false for all non-ASCII characters.
	//! This allows AsciiSet to be used as a predicate.
	template<typename Char> INTRA_NODISCARD constexpr forceinline Requires<
		CChar<Char>,
	bool> operator()(Char c) const {return byte(c) < 128 && operator[](c);}

	//! Check if this set contains subset.
	//! This allows AsciiSet to be used as a predicate.
	INTRA_NODISCARD constexpr forceinline bool operator()(AsciiSet subset) const {return subset == (*this & subset);}
	
	//! Add element from the set if it doesn't belong to it
	constexpr forceinline void Set(char c)
	{
		if(c & 0x80) return;
		v[c/64] |= 1ull << (size_t(c) & 63);
	}

	//! Remove element from the set if it belongs to it
	constexpr forceinline void Reset(char c)
	{
		if(c & 0x80) return;
		v[c/64] &= ~(1ull << (size_t(c) & 63));
	}

	//! Add elements to the set if they don't belong to it
	constexpr forceinline void Set(CSpan<char> chars)
	{
		for(char c: chars) Set(c);
	}
	
	//! Remove elements from the set if they belong to it
	constexpr void Reset(CSpan<char> chars)
	{
		while(!chars.Empty())
		{
			Reset(chars.First());
			chars.PopFirst();
		}
	}

	//! Set union
	INTRA_NODISCARD constexpr forceinline AsciiSet operator|(const AsciiSet& rhs) const
	{return {v[0] | rhs.v[0], v[1] | rhs.v[1]};}

	//! Set negation
	INTRA_NODISCARD constexpr forceinline AsciiSet operator~() const
	{return {~v[0], ~v[1]};}

	//! Set intersection
	INTRA_NODISCARD constexpr forceinline AsciiSet operator&(const AsciiSet& rhs) const
	{return {v[0] & rhs.v[0], v[1] & rhs.v[1]};}

	//! Add element to the set if it doesn't belong to it
	INTRA_NODISCARD constexpr AsciiSet operator|(char c) const
	{
		AsciiSet result = *this;
		result.Set(c);
		return result;
	}

	//! Add elements to the set if they don't belong to it
	INTRA_NODISCARD constexpr AsciiSet operator|(CSpan<char> chars) const
	{
		AsciiSet result = *this;
		result.Set(chars);
		return result;
	}

	//! Add elements to the set if they don't belong to it
	template<size_t N> INTRA_NODISCARD constexpr forceinline AsciiSet operator|(const char(&chars)[N]) const {return operator|(CSpan<char>(chars));}
};

/** Some standard ASCII sets.

  Prefer using similar functors from Core/Operations.h - they are about 3 times faster.
  These sets may be useful with non-templated functions.
*/
struct TAsciiSets
{
	constexpr TAsciiSets() {}
	AsciiSet None;
	AsciiSet Spaces{(1ull << ' ') | (1ull << '\t') | (1ull << '\r') | (1ull << '\n'), 0};
	AsciiSet Slashes{1ULL << 47, 1ULL << 28};
	AsciiSet Digits{0x03FF000000000000ULL, 0};
	AsciiSet LatinLowercase{0, 0x7FFFFFE00000000ULL};
	AsciiSet LatinUppercase{0, 0x7FFFFFE};
	AsciiSet Latin{0, 0x7FFFFFE07FFFFFEULL};
	AsciiSet LatinAndDigits{0x03FF000000000000ULL, 0x7FFFFFE07FFFFFEULL};
	AsciiSet IdentifierChars{0x03FF000000000000ULL | (1ULL << '$'), 0x7FFFFFE07FFFFFEULL | (1ULL << ('_' - 64u))};
	AsciiSet NotIdentifierChars{~(0x03FF000000000000ULL | (1ULL << '$')), ~(0x7FFFFFE07FFFFFEULL | (1ULL << ('_' - 64u)))};
};
constexpr const TAsciiSets AsciiSets;

#if INTRA_CONSTEXPR_TEST
static_assert(AsciiSets.Digits['5'], "TEST FAILED!");
static_assert(!AsciiSets.Digits['g'], "TEST FAILED!");
//static_assert(!AsciiSets.Digits['\x95'], "TEST FAILED!"); //non-ASCII characters with operator[] cause UB or a compile-time error in constexpr context
static_assert(!AsciiSets.Digits('\x95'), "TEST FAILED!"); //operator() returns false for all non-ASCII characters

//Check string literal constructor and magic constants defined above
static_assert(AsciiSets.None == "", "TEST FAILED!");
static_assert(AsciiSets.Spaces == " \t\r\n", "TEST FAILED!");
static_assert(AsciiSets.Slashes == "\\/", "TEST FAILED!");
static_assert(AsciiSets.Digits == "0123456789", "TEST FAILED!");
static_assert(AsciiSets.LatinLowercase == "abcdefghijklmnopqrstuvwxyz", "TEST FAILED!");
static_assert(AsciiSets.LatinUppercase == "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "TEST FAILED!");
static_assert(AsciiSets.Latin == "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", "TEST FAILED!");
static_assert(AsciiSets.LatinAndDigits == "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", "TEST FAILED!");
static_assert(AsciiSets.IdentifierChars == "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ$abcdefghijklmnopqrstuvwxyz_", "TEST FAILED!");
static_assert(AsciiSets.NotIdentifierChars == ~AsciiSets.IdentifierChars, "TEST FAILED!");

static_assert(AsciiSet("4562130987") == AsciiSets.Digits, "TEST FAILED!");
static_assert(AsciiSet("456d2130987") != AsciiSets.Digits, "TEST FAILED!");
static_assert(AsciiSet("452130987") != AsciiSets.Digits, "TEST FAILED!");
static_assert((AsciiSet("452130987") | '6') == AsciiSets.Digits, "TEST FAILED!");

static_assert(AsciiSets.IdentifierChars("_itI$AValidIdentifier"), "TEST FAILED!");
#endif

INTRA_END
