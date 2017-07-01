#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"
#include "MurmurCT.h"
#include "Murmur.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct StringHash
{
	uint hash;
#ifdef _DEBUG
	union
	{
		char strEnd[12];
		const char* strLiteral;
	};
#endif

	template<int len> constexpr StringHash(const char(&str)[len]);
	constexpr StringHash(uint val): hash(val)
#ifdef _DEBUG
		, strLiteral(null)
#endif
	{}
	constexpr StringHash(null_t=null): hash(0)
#ifdef _DEBUG
		, strLiteral(null)
#endif
	{}

	StringHash(StringView sv);

	constexpr bool operator==(const StringHash& rhs) const
	{return hash==rhs.hash;}

	bool operator!=(const StringHash& rhs) const {return !operator==(rhs);}
};

template<int len> constexpr inline StringHash::StringHash(const char(&str)[len]):
	hash(HashCT::Murmur3_32(str, 0))
#ifdef _DEBUG
	, strLiteral(str)
#endif
{}

inline StringHash::StringHash(StringView sv): hash(Hash::Murmur3_32(sv, 0))
{
#ifdef _DEBUG
	Span<char> dst = strEnd;
	Range::WriteTo(sv.Tail(dst.Length()), dst);
	Range::FillZeros(dst);
#endif
}

INTRA_WARNING_POP

}}
