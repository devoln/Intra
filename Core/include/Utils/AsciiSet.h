#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"
#include "Range/Generators/Span.h"
#include "Algo/Comparison/Equals.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Класс, содержащий множество символов из ASCII
class AsciiSet
{
	enum: byte {BytesPerElement = sizeof(ulong64), BitsPerElement = BytesPerElement*8,
		Mask = BitsPerElement-1, ElementCount = 16/BytesPerElement};
	ulong64 v[ElementCount];

public:
	constexpr forceinline AsciiSet(null_t=null) noexcept: v{0,0} {}

	template<uint N> forceinline AsciiSet(const char(&chars)[N]) noexcept: v{0,0}
	{
		Set(CSpan<char>(chars));
	}

	explicit forceinline AsciiSet(CSpan<char> chars): v{0,0}
	{
		Set(chars);
	}

	forceinline AsciiSet& operator=(null_t) {v[0]=v[1]=0; return *this;}
	constexpr forceinline bool operator==(null_t) const noexcept {return v[0]==0 && v[1]==0;}
	constexpr forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

	bool operator[](char c) const
	{
		INTRA_DEBUG_ASSERT(byte(c)<128);
		return (v[c/BitsPerElement] & (1ull << (c & Mask))) != 0;
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
		v[c/BitsPerElement] |= 1ull << (size_t(c) & Mask);
	}

	forceinline void Reset(char c)
	{
		if(c & 0x80) return;
		v[c/BitsPerElement] &= ~(1ull << (size_t(c) & Mask));
	}

	forceinline void Set(CSpan<char> chars)
	{
		for(char c: chars) Set(c);
	}
	
	template<typename CharRange> Meta::EnableIf<
		Range::IsCharRange<CharRange>::_
	> Reset(CharRange chars)
	{
		while(!chars.Empty())
		{
			Reset(chars.First());
			chars.PopFirst();
		}
	}

	AsciiSet operator|(const AsciiSet& rhs) const
	{
		AsciiSet result;
		for(int i=0; i<ElementCount; i++)
			result.v[i] = v[i]|rhs.v[i];
		return result;
	}

	AsciiSet operator~() const
	{
		AsciiSet result;
		for(int i=0; i<ElementCount; i++)
			result.v[i] = ~v[i];
		return result;
	}

	AsciiSet operator&(const AsciiSet& rhs) const
	{
		AsciiSet result;
		for(int i=0; i<ElementCount; i++)
			result.v[i] = v[i]&rhs.v[i];
		return result;
	}

	AsciiSet operator|(char c) const
	{
		AsciiSet result = *this;
		result.Set(c);
		return result;
	}

	template<typename CharRange> Meta::EnableIf<
		Range::IsCharRange<CharRange>::_,
	AsciiSet> operator|(CharRange chars) const
	{
		AsciiSet result = *this;
		result.Set(chars);
		return result;
	}

	template<size_t N> AsciiSet operator|(const char(&chars)[N]) const {return operator|(CSpan<char>(chars));}


	static const AsciiSet Null, Spaces, Slashes, Digits;
	static const AsciiSet LatinLowercase, LatinUppercase, Latin, LatinAndDigits;
	static const AsciiSet IdentifierChars, NotIdentifierChars;
};

INTRA_WARNING_POP

}
