#pragma once

#include "Core/Range/Span.h"

INTRA_BEGIN
inline namespace Memory {

struct FreeList
{
	FreeList(null_t=null) {}

	FreeList(Span<byte> buf, size_t elementSize, size_t alignment)
	{InitBuffer(buf, elementSize, alignment);}

	void InitBuffer(Span<byte> buf, size_t elementSize, size_t alignment)
	{
		if(elementSize < sizeof(FreeList*))
			elementSize = sizeof(FreeList*);

		byte* bufPtr = Aligned(buf.Begin, alignment);

		mNext = reinterpret_cast<FreeList*>(bufPtr);
		bufPtr += elementSize;

		FreeList* runner = mNext;
		const size_t numElements = buf.Length()/elementSize;
		for(size_t i = 1; i<numElements; i++)
		{
			runner = runner->mNext = reinterpret_cast<FreeList*>(bufPtr);
			bufPtr += elementSize;
		}

		runner->mNext = null;
	}

	AnyPtr Allocate()
	{
		if(mNext == null) return null;
		FreeList* head = mNext;
		mNext = head->mNext;
		return head;
	}

	void Free(void* ptr)
	{
		FreeList* head = static_cast<FreeList*>(ptr);
		head->mNext = mNext;
		mNext = head;
	}

	bool HasFree() const {return mNext != null;}

private:
	FreeList* mNext = null;
};


struct APool
{
	APool(null_t=null): mElementSize(0), mAlignment(0) {}
	APool(Span<byte> buf, size_t elementSize, size_t allocatorAlignment):
		mList(buf, elementSize, allocatorAlignment),
		mElementSize(ushort(elementSize)),
		mAlignment(ushort(allocatorAlignment)) {}

	forceinline size_t GetAlignment() const {return mAlignment;}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = {})
	{
		(void)sourceInfo;
		INTRA_DEBUG_ASSERT(bytes <= mElementSize);
		if(bytes > mElementSize) return null;
		bytes = mElementSize;
		return mList.Allocate();
	}

	void Free(void* ptr, size_t size)
	{
		(void)size;
		INTRA_DEBUG_ASSERT(size == mElementSize);
		mList.Free(ptr);
	}

	forceinline size_t ElementSize() const {return mElementSize;}

	forceinline size_t GetAllocationSize(void* ptr) const {(void)ptr; return mElementSize;}

	forceinline bool operator==(null_t) const {return mElementSize == 0;}
	forceinline bool operator!=(null_t) const {return !operator==(null);}

private:
	FreeList mList;
	ushort mElementSize, mAlignment;
};

}
INTRA_END
