#pragma once

#include "Core/Assert.h"
#include "Core/Range/Span.h"

INTRA_BEGIN
struct AStack
{
	AStack(Span<byte> buf, size_t allocatorAlignment):
		mStart(buf.Begin), mRest(buf), mAlignment(allocatorAlignment) {}
	
	size_t GetAlignment() const {return mAlignment;}

	AnyPtr Allocate(size_t bytes, SourceInfo sourceInfo)
	{
		(void)sourceInfo;

		// store the allocation offset right in front of the allocation
		bytes += sizeof(uint);
		const uint allocationOffset = uint(mRest.Begin - mStart);

		if(mRest.Length() < bytes) return null;

		byte* bufferPtr = Aligned(mRest.Begin, mAlignment, sizeof(uint));

		*reinterpret_cast<uint*>(bufferPtr) = allocationOffset;
		bufferPtr += sizeof(uint);

		mRest.Begin += bytes;
		return bufferPtr;
	}

	void Free(void* ptr, size_t size)
	{
		INTRA_DEBUG_ASSERT(ptr != null);
		INTRA_DEBUG_ASSERT(static_cast<byte*>(ptr) < mRest.Begin);
		(void)size;

		// grab the allocation offset from the 4 bytes right before the given pointer
		const uint allocationOffset = *--reinterpret_cast<uint*&>(ptr);
		mRest.Begin = mStart+allocationOffset;
	}

private:
	byte* mStart;
	Span<byte> mRest;
	size_t mAlignment;
};
INTRA_END
