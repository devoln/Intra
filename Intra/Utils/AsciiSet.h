#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Meta/Type.h"
#include "Span.h"
#include "Utils/Debug.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Utils {

//! Класс, содержащий множество символов из ASCII
class AsciiSet
{
	ulong64 v[2];

	friend struct TAsciiSets;

	constexpr forceinline AsciiSet(ulong64 v1, ulong64 v2): v{v1, v2} {}
public:
	constexpr forceinline AsciiSet(null_t=null) noexcept: v{0,0} {}

	template<uint N> forceinline AsciiSet(const char(&chars)[N]) noexcept: v{0,0} {Set(CSpan<char>(chars));}

	explicit forceinline AsciiSet(CSpan<char> chars): v{0,0} {Set(chars);}

	forceinline AsciiSet& operator=(null_t) {v[0]=v[1]=0; return *this;}
	constexpr forceinline bool operator==(null_t) const noexcept {return v[0]==0 && v[1]==0;}
	constexpr forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

	bool operator[](char c) const
	{
		INTRA_DEBUG_ASSERT(byte(c)<128);
		return (v[c/64] & (1ull << (size_t(c) & 63))) != 0;
	}

	bool Contains(char c) const
	{
		if(byte(c)>=128) return false;
		return operator[](c);
	}

	template<typename Char> forceinline Meta::EnableIf<
		Meta::IsCharType<Char>::_,
	bool> operator()(Char c) const {return Contains(c);}
	
	forceinline void Set(char c)
	{
		if(c & 0x80) return;
		v[c/64] |= 1ull << (size_t(c) & 63);
	}

	forceinline void Reset(char c)
	{
		if(c & 0x80) return;
		v[c/64] &= ~(1ull << (size_t(c) & 63));
	}

	forceinline void Set(CSpan<char> chars)
	{
		for(char c: chars) Set(c);
	}
	
	void Reset(CSpan<char> chars)
	{
		while(!chars.Empty())
		{
			Reset(chars.First());
			chars.PopFirst();
		}
	}

	constexpr forceinline AsciiSet operator|(const AsciiSet& rhs) const
	{return {v[0] | rhs.v[0], v[1] | rhs.v[1]};}

	constexpr forceinline AsciiSet operator~() const
	{return {~v[0], ~v[1]};}

	constexpr forceinline AsciiSet operator&(const AsciiSet& rhs) const
	{return {v[0] & rhs.v[0], v[1] & rhs.v[1]};}

	AsciiSet operator|(char c) const
	{
		AsciiSet result = *this;
		result.Set(c);
		return result;
	}

	AsciiSet operator|(CSpan<char> chars) const
	{
		AsciiSet result = *this;
		result.Set(chars);
		return result;
	}

	template<size_t N> AsciiSet operator|(const char(&chars)[N]) const {return operator|(CSpan<char>(chars));}
};

struct TAsciiSets
{
	constexpr TAsciiSets() {}
	AsciiSet None{0,0};
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

}
using Utils::AsciiSet;
using Utils::AsciiSets;

}

INTRA_WARNING_POP
