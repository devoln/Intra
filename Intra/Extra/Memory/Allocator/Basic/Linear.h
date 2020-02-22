#pragma once

#include "Core/Assert.h"
#include "Core/Range/Span.h"

INTRA_BEGIN
struct ALinear
{
	ALinear(Span<byte> buf=null, size_t allocatorAlignment=16):
		mStart(buf.Begin), mRest(buf), mAlignment(allocatorAlignment) {}

	size_t GetAlignment() const {return mAlignment;}

	AnyPtr Allocate(size_t bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
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
	Span<byte> mRest;
	size_t mAlignment;
};
INTRA_END
