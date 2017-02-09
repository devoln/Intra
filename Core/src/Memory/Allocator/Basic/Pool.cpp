#include "Memory/Allocator/Basic/Pool.h"
#include "Range/Generators/ArrayRange.h"

namespace Intra { namespace Memory {

void FreeList::InitBuffer(ArrayRange<byte> buf, size_t elementSize, size_t alignment)
{
	if(elementSize<sizeof(FreeList*))
		elementSize = sizeof(FreeList*);

	size_t numElements = buf.Length()/elementSize;

	union
	{
		byte* asByte;
		FreeList* asSelf;
	};
	asByte = Aligned(buf.Begin, alignment);

	mNext = asSelf;
	asByte += elementSize;

	FreeList* runner = mNext;
	for(size_t i = 1; i<numElements; i++)
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
	FreeList* head = reinterpret_cast<FreeList*>(ptr);
	head->mNext = mNext;
	mNext = head;
}


AnyPtr APool::Allocate(size_t& bytes, SourceInfo sourceInfo)
{
	(void)sourceInfo;
	INTRA_ASSERT(bytes <= mElementSize);
	if(bytes>mElementSize) return null;
	bytes = mElementSize;
	return mList.Allocate();
}

void APool::Free(void* ptr, size_t size)
{
	(void)size;
	INTRA_ASSERT(size == mElementSize);
	mList.Free(ptr);
}

}}

