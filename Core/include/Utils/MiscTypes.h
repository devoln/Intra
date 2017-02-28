#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/Debug.h"
#include "Platform/CppWarnings.h"
#include "Platform/Endianess.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#pragma pack(push, 1)
struct uint24LE
{
	uint24LE() = default;
	uint24LE(const uint24LE& rhs) = default;

	uint24LE(uint v) {operator=(v);}

	uint24LE& operator=(uint v)
	{
		INTRA_DEBUG_ASSERT(v<=0xFFFFFF);
		data[0] = byte(v & 0xFF);
		data[1] = byte((v & 0xFF00) >> 8);
		data[2] = byte((v & 0xFF0000) >> 16);
		return *this;
	}

	uint24LE& operator++()
	{
		INTRA_DEBUG_ASSERT(data[0]!=255 || data[1]!=255 || data[2]!=255);
		if(++data[0]==0 && ++data[1]==0) data[2]++;
		return *this;
	}

	uint24LE& operator=(const uint24LE&) = default;

	operator uint() const {return uint(data[0]) | (uint(data[1]) << 8u) | (uint(data[2]) << 16u);}

private:
	byte data[3];
};

typedef AnotherEndian<uint24LE> uint24BE;

struct uint40LE
{
	uint40LE() = default;
	uint40LE(uint v) {operator=(v);}

	uint40LE& operator=(ulong64 v)
	{
		data[0] = byte(v & 0xFF);
		data[1] = byte((v & 0xFF00) >> 8);
		data[2] = byte((v & 0xFF0000) >> 16);
		data[3] = byte((v & 0xFF000000) >> 24);
		data[4] = byte((v & 0xFF00000000) >> 32);
		return *this;
	}

	uint40LE& operator=(uint v)
	{
		data[0] = byte(v & 0xFF);
		data[1] = byte((v & 0xFF00) >> 8);
		data[2] = byte((v & 0xFF0000) >> 16);
		data[3] = byte((v & 0xFF000000) >> 24);
		return *this;
	}

	uint40LE& operator++()
	{
		INTRA_DEBUG_ASSERT(data[0]!=255 || data[1]!=255 || data[2]!=255 || data[3]!=255 || data[4]!=255);
		if(++data[0]==0 && ++data[1]==0 && ++data[2]==0 && ++data[3]==0) data[4]++;
		return *this;
	}

	uint40LE& operator=(const uint40LE&) = default;

	operator ulong64() const
	{
		return data[0] |
			(uint(data[1]) << 8) |
			(uint(data[2]) << 16) |
			(uint(data[3]) << 24) |
			(ulong64(data[4]) << 32);
	}

private:
	byte data[5];
};

typedef AnotherEndian<uint40LE> uint40BE;
#pragma pack(pop)

INTRA_WARNING_POP

}
