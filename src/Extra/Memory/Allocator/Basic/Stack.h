#pragma once

#include "Intra/Assert.h"
#include "Intra/Range/Span.h"

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
		bytes += sizeof(unsigned);
		const unsigned allocationOffset = unsigned(mRest.Begin - mStart);

		if(size_t(mRest.Length()) < bytes) return null;

		byte* bufferPtr = Aligned(mRest.Begin, mAlignment, sizeof(unsigned));

		*reinterpret_cast<unsigned*>(bufferPtr) = allocationOffset;
		bufferPtr += sizeof(unsigned);

		mRest.Begin += bytes;
		return bufferPtr;
	}

	void Free(void* ptr, size_t size)
	{
		INTRA_DEBUG_ASSERT(ptr != null);
		INTRA_DEBUG_ASSERT(static_cast<byte*>(ptr) < mRest.Begin);
		(void)size;

		// grab the allocation offset from the 4 bytes right before the given pointer
		const unsigned allocationOffset = *--reinterpret_cast<unsigned*&>(ptr);
		mRest.Begin = mStart+allocationOffset;
	}

private:
	byte* mStart;
	Span<byte> mRest;
	size_t mAlignment;
};
INTRA_END
