#pragma once

#include "Intra/Range/Span.h"

INTRA_BEGIN
struct FreeList
{
	FreeList(decltype(null)=null) {}

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
		const auto numElements = size_t(buf.Length()) / elementSize;
		for(size_t i = 1; i < numElements; i++)
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
	APool(decltype(null)=null): mElementSize(0), mAlignment(0) {}
	APool(Span<byte> buf, size_t elementSize, size_t allocatorAlignment):
		mList(buf, elementSize, allocatorAlignment),
		mElementSize(uint16(elementSize)),
		mAlignment(uint16(allocatorAlignment)) {}

	size_t GetAlignment() const {return mAlignment;}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = {})
	{
		(void)sourceInfo;
		INTRA_PRECONDITION(bytes <= mElementSize);
		if(bytes > mElementSize) return null;
		bytes = mElementSize;
		return mList.Allocate();
	}

	void Free(void* ptr, size_t size)
	{
		(void)size;
		INTRA_PRECONDITION(size == mElementSize);
		mList.Free(ptr);
	}

	size_t ElementSize() const {return mElementSize;}

	size_t GetAllocationSize(void* ptr) const {(void)ptr; return mElementSize;}

	bool operator==(decltype(null)) const {return mElementSize == 0;}
	bool operator!=(decltype(null)) const {return !operator==(null);}

private:
	FreeList mList;
	uint16 mElementSize, mAlignment;
};
INTRA_END
