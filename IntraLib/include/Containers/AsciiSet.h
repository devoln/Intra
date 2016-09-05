#pragma once

#include "Core/Core.h"
#include "Meta/Type.h"
#include "Algorithms/Range.h"

namespace Intra {

//! Класс, содержащий множество символов из ASCII
class AsciiSet
{
	enum: byte {BytesPerElement = sizeof(size_t), BitsPerElement = BytesPerElement*8,
		Mask = BitsPerElement-1, ElementCount = 16/BytesPerElement};
	size_t v[ElementCount];

public:
	AsciiSet(null_t=null)
	{
		core::memset(v, 0, sizeof(v));
	}

	template<uint N> forceinline AsciiSet(const char(&chars)[N])
	{
		core::memset(v, 0, sizeof(v));
		Set(ArrayRange<const char>(chars).DropBack());
	}

	template<typename CharRange> explicit AsciiSet(CharRange chars)
	{
		core::memset(v, 0, sizeof(v));
		Set(chars);
	}

	AsciiSet& operator=(const AsciiSet& rhs) = default;
	AsciiSet& operator=(null_t) {core::memset(v, 0, sizeof(v)); return *this;}
	bool operator==(null_t) const {return core::memcmp(v, Null.v, sizeof(v))==0;}
	bool operator!=(null_t) const {return !operator==(null);}

	bool operator[](char c) const
	{
		INTRA_ASSERT(byte(c)<128);
		return (v[c/BitsPerElement] & (size_t(1) << (c & Mask))) != 0;
	}

	bool Contains(char c) const
	{
		if(byte(c)>=128) return false;
		return operator[](c);
	}

	template<typename Char> Meta::EnableIf<
		Meta::IsCharType<Char>::_,
	bool> operator()(Char c) const
	{
		return Contains(c);
	}
	
	forceinline void Set(char c)
	{
		if(c & 0x80) return;
		v[c/BitsPerElement] |= size_t(1) << (size_t(c) & Mask);
	}

	forceinline void Reset(char c)
	{
		if(c & 0x80) return;
		v[c/BitsPerElement] &= ~(size_t(1) << (size_t(c) & Mask));
	}

	template<typename CharRange> Meta::EnableIf<
		Range::IsCharRange<CharRange>::_
	> Set(CharRange chars)
	{
		while(!chars.Empty())
		{
			Set(chars.First());
			chars.PopFirst();
		}
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

	template<size_t N> AsciiSet operator|(const char(&chars)[N]) const {return operator|(ArrayRange<const char>(chars).DropBack());}


	static const AsciiSet Null, Spaces, Slashes, Digits;
	static const AsciiSet LatinLowercase, LatinUppercase, Latin, LatinAndDigits;
	static const AsciiSet IdentifierChars, NotIdentifierChars;
};

}
