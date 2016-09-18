#include "Memory/Allocator.h"
#include "Containers/String.h"

namespace Intra { namespace Memory {

AnyPtr StackAllocator::Allocate(size_t bytes, const SourceInfo& sourceInfo)
{
	(void)sourceInfo;
	// store the allocation offset right in front of the allocation
	bytes += sizeof(uint);

	const uint allocationOffset = uint(rest.Begin - start);

	if(rest.Length()<bytes) return null;

	union
	{
		byte* as_byte;
		uint* as_uint;
	};

	as_byte = rest.Begin;
	as_byte = Aligned(as_byte, alignment, sizeof(uint));

	*as_uint++ = allocationOffset;

	rest.Begin += bytes;
	return as_byte;
}

void StackAllocator::Free(void* ptr, size_t size)
{
	(void)size;
	INTRA_ASSERT(ptr!=null);
	INTRA_ASSERT(reinterpret_cast<byte*>(ptr) < rest.Begin);

	// grab the allocation offset from the 4 bytes right before the given pointer
	const uint allocationOffset = *--reinterpret_cast<uint*&>(ptr);

	rest.Begin = start+allocationOffset;
}



void FreeList::InitBuffer(ArrayRange<byte> buf, size_t elementSize, size_t alignment)
{
	if(elementSize<sizeof(FreeList*))
		elementSize = sizeof(FreeList*);

	size_t numElements = buf.Length()/elementSize;

	union
	{
		byte* as_byte;
		FreeList* as_self;
	};
	as_byte = Aligned(buf.Begin, alignment);

	next = as_self;
	as_byte += elementSize;

	FreeList* runner = next;
	for(size_t i=1; i<numElements; i++)
	{
		runner->next = as_self;
		runner = as_self;
		as_byte += elementSize;
	}

	runner->next = null;
}

GlobalHeapType GlobalHeap;
SizedHeapType SizedHeap;

}}
