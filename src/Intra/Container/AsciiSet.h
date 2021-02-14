#pragma once

#include "Intra/Range.h"

namespace Intra { INTRA_BEGIN
/** Set of ASCII chars useful for fast belonging checks in parsers.

  Implementation note: this class could be implemented via SBitset<128> but this version is
  simpler and faster when compiler optimizations are disabled.
*/
class AsciiSet
{
	uint64 v[2]{};

	friend struct TAsciiSets;

	constexpr AsciiSet(uint64 v1, uint64 v2): v{v1, v2} {}
public:
	AsciiSet() = default;

	template<unsigned N> constexpr AsciiSet(const char(&chars)[N]) noexcept: v{0,0} {Set(Span<const char>(chars));}

	explicit constexpr AsciiSet(Span<const char> chars): v{0,0} {Set(chars);}

	[[nodiscard]] constexpr bool operator==(const AsciiSet& rhs) const noexcept {return ((v[0]^rhs.v[0])|(v[1]^rhs.v[1])) == 0;}

	/// Check if this set contains element c.
	/// c must be ASCII, otherwise the behavior is undefined.
	[[nodiscard]] constexpr bool operator[](char c) const
	{
		INTRA_PRECONDITION(unsigned(c) < 128);
		return (v[uint8(c)/64] & (1ull << (size_t(c) & 63))) != 0;
	}

	/// Check if the set contains element c. Returns false for all non-ASCII characters.
	/// This allows AsciiSet to be used as a predicate.
	template<CChar Char> [[nodiscard]] constexpr bool operator()(Char c) const {return byte(c) < 128 && operator[](c);}

	/// Check if this set contains subset.
	/// This allows AsciiSet to be used as a predicate.
	[[nodiscard]] constexpr bool operator()(AsciiSet subset) const {return subset == (*this & subset);}
	
	/// Add element from the set if it doesn't belong to it
	constexpr void Set(char c)
	{
		if(c & 0x80) return;
		v[c/64] |= 1ull << (size_t(c) & 63);
	}

	/// Remove element from the set if it belongs to it
	constexpr void Reset(char c)
	{
		if(c & 0x80) return;
		v[c/64] &= ~(1ull << (size_t(c) & 63));
	}

	/// Add elements to the set if they don't belong to it
	constexpr void Set(Span<const char> chars)
	{
		for(char c: chars) Set(c);
	}
	
	/// Remove elements from the set if they belong to it
	constexpr void Reset(Span<const char> chars)
	{
		while(!chars.Empty())
		{
			Reset(chars.First());
			chars.PopFirst();
		}
	}

	/// Set union
	[[nodiscard]] constexpr AsciiSet operator|(const AsciiSet& rhs) const
	{return {v[0] | rhs.v[0], v[1] | rhs.v[1]};}

	/// Set negation
	[[nodiscard]] constexpr AsciiSet operator~() const
	{return {~v[0], ~v[1]};}

	/// Set intersection
	[[nodiscard]] constexpr AsciiSet operator&(const AsciiSet& rhs) const
	{return {v[0] & rhs.v[0], v[1] & rhs.v[1]};}

	/// Add element to the set if it doesn't belong to it
	[[nodiscard]] constexpr AsciiSet operator|(char c) const
	{
		AsciiSet result = *this;
		result.Set(c);
		return result;
	}

	/// Add elements to the set if they don't belong to it
	[[nodiscard]] constexpr AsciiSet operator|(Span<const char> chars) const
	{
		AsciiSet result = *this;
		result.Set(chars);
		return result;
	}

	/// Add elements to the set if they don't belong to it
	template<size_t N> [[nodiscard]] constexpr AsciiSet operator|(const char(&chars)[N]) const {return operator|(Span(chars));}
};

/** Some standard ASCII sets.

  Prefer using similar functors from Core/Functional.h - they are about 3 times faster.
  These sets may be useful with non-templated functions or for reusing executable code.
*/
struct TAsciiSets
{
	constexpr TAsciiSets() {}
	AsciiSet None;
	AsciiSet Spaces = " \t\r\n"; // { (1ull << ' ') | (1ull << '\t') | (1ull << '\r') | (1ull << '\n'), 0 };
	AsciiSet Slashes = "/\\";// { 1ULL << 47, 1ULL << 28 };
	AsciiSet Digits = "0123456789";// { 0x03FF000000000000ULL, 0 };
	AsciiSet LatinLowercase = "abcdefghijklmnopqrstuvwxyz";// { 0, 0x7FFFFFE00000000ULL };
	AsciiSet LatinUppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"; //{0, 0x7FFFFFE};
	AsciiSet Latin = LatinLowercase|LatinUppercase;// { 0, 0x7FFFFFE07FFFFFEULL };
	AsciiSet LatinAndDigits = Latin|Digits;// { 0x03FF000000000000ULL, 0x7FFFFFE07FFFFFEULL };
	AsciiSet IdentifierChars = LatinAndDigits|"$_";// { 0x03FF000000000000ULL | (1ULL << '$'), 0x7FFFFFE07FFFFFEULL | (1ULL << ('_' - 64u)) };
	AsciiSet NotIdentifierChars = ~IdentifierChars;
};
constexpr const TAsciiSets AsciiSets;

#if INTRA_CONSTEXPR_TEST
static_assert(AsciiSets.Digits['5']);
static_assert(!AsciiSets.Digits['g']);
//static_assert(!AsciiSets.Digits['\x95']); //non-ASCII characters with operator[] cause UB or a compile-time error in constexpr context
static_assert(!AsciiSets.Digits('\x95')); //operator() returns false for all non-ASCII characters

//Check string literal constructor and magic constants defined above
static_assert(AsciiSets.None == "");
static_assert(AsciiSets.Spaces == " \t\r\n");
static_assert(AsciiSets.Slashes == "\\/");
static_assert(AsciiSets.Digits == "0123456789");
static_assert(AsciiSets.LatinLowercase == "abcdefghijklmnopqrstuvwxyz");
static_assert(AsciiSets.LatinUppercase == "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
static_assert(AsciiSets.Latin == "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
static_assert(AsciiSets.LatinAndDigits == "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
static_assert(AsciiSets.IdentifierChars == "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ$abcdefghijklmnopqrstuvwxyz_");
static_assert(AsciiSets.NotIdentifierChars == ~AsciiSets.IdentifierChars);

static_assert(AsciiSet("4562130987") == AsciiSets.Digits);
static_assert(AsciiSet("456d2130987") != AsciiSets.Digits);
static_assert(AsciiSet("452130987") != AsciiSets.Digits);
static_assert((AsciiSet("452130987") | '6') == AsciiSets.Digits);

static_assert(AsciiSets.IdentifierChars("_itI$AValidIdentifier"));
#endif

} INTRA_END
