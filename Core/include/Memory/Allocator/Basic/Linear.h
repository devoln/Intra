#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/CppWarnings.h"
#include "Platform/Debug.h"
#include "Range/Generators/ArrayRange.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Memory {

struct ALinear
{
	ALinear(ArrayRange<byte> buf=null, size_t allocatorAlignment=16):
		mStart(buf.Begin), mRest(buf), mAlignment(allocatorAlignment) {}

	size_t GetAlignment() const {return mAlignment;}

	AnyPtr Allocate(size_t bytes, SourceInfo sourceInfo)
	{
		(void)sourceInfo;

		byte* userPtr = Aligned(mRest.Begin, mAlignment);
		if(mRest.Begin + bytes >= mRest.End) return null;
		mRest.Begin += bytes;

		return userPtr;
	}

	void Free(void* ptr, size_t size) {(void)ptr; (void)size;}

	void Reset() {mRest.Begin = mStart;}

private:
	byte* mStart;
	ArrayRange<byte> mRest;
	size_t mAlignment;
};

}}

INTRA_WARNING_POP
