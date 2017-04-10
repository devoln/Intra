#include "Memory/Allocator/Basic/Pool.h"
#include "Memory/Align.h"
#include "Range/Generators/Span.h"

namespace Intra { namespace Memory {

void FreeList::InitBuffer(Span<byte> buf, size_t elementSize, size_t alignment)
{
	if(elementSize<sizeof(FreeList*))
		elementSize = sizeof(FreeList*);


	union
	{
		byte* asByte;
		FreeList* asSelf;
	};
	asByte = Aligned(buf.Begin, alignment);

	mNext = asSelf;
	asByte += elementSize;

	FreeList* runner = mNext;
	const size_t numElements = buf.Length()/elementSize;
	for(size_t i=1; i<numElements; i++)
	{
		runner->mNext = asSelf;
		runner = asSelf;
		asByte += elementSize;
	}

	runner->mNext = null;
}

AnyPtr FreeList::Allocate()
{
	if(mNext==null) return null;
	FreeList* head = mNext;
	mNext = head->mNext;
	return head;
}

void FreeList::Free(void* ptr)
{
	FreeList* head = static_cast<FreeList*>(ptr);
	head->mNext = mNext;
	mNext = head;
}


AnyPtr APool::Allocate(size_t& bytes, SourceInfo sourceInfo)
{
	(void)sourceInfo;
	INTRA_DEBUG_ASSERT(bytes <= mElementSize);
	if(bytes>mElementSize) return null;
	bytes = mElementSize;
	return mList.Allocate();
}

void APool::Free(void* ptr, size_t size)
{
	(void)size;
	INTRA_DEBUG_ASSERT(size == mElementSize);
	mList.Free(ptr);
}

}}

