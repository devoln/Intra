#include "Memory/Allocator/Basic/Stack.h"
#include "Memory/Align.h"

namespace Intra { namespace Memory {

AnyPtr AStack::Allocate(size_t bytes, const Utils::SourceInfo& sourceInfo)
{
	(void)sourceInfo;

	// store the allocation offset right in front of the allocation
	bytes += sizeof(uint);
	const uint allocationOffset = uint(mRest.Begin - mStart);

	if(mRest.Length()<bytes) return null;

	union
	{
		byte* asByte;
		uint* asUint;
	};

	asByte = mRest.Begin;
	asByte = Aligned(asByte, mAlignment, sizeof(uint));

	*asUint++ = allocationOffset;

	mRest.Begin += bytes;
	return asByte;
}

void AStack::Free(void* ptr, size_t size)
{
	(void)size;
	INTRA_DEBUG_ASSERT(ptr!=null);
	INTRA_DEBUG_ASSERT(reinterpret_cast<byte*>(ptr) < mRest.Begin);

	// grab the allocation offset from the 4 bytes right before the given pointer
	const uint allocationOffset = *--reinterpret_cast<uint*&>(ptr);
	mRest.Begin = mStart+allocationOffset;
}

}}
