#pragma once

#include "Platform/FundamentalTypes.h"
#include "Range/Generators/Span.h"

namespace Intra { namespace Memory {

struct FreeList
{
	FreeList(null_t=null): mNext(null) {}

	FreeList(Span<byte> buf, size_t elementSize, size_t alignment): mNext(null)
	{InitBuffer(buf, elementSize, alignment);}

	void InitBuffer(Span<byte> buf, size_t elementSize, size_t alignment);

	AnyPtr Allocate();
	void Free(void* ptr);

	bool HasFree() const {return mNext!=null;}

private:
	FreeList* mNext;
};


struct APool
{
	APool(null_t=null): mElementSize(0), mAlignment(0) {}
	APool(Span<byte> buf, size_t elementSize, size_t allocatorAlignment):
		mList(buf, elementSize, allocatorAlignment),
		mElementSize(ushort(elementSize)),
		mAlignment(ushort(allocatorAlignment)) {}

	size_t GetAlignment() const {return mAlignment;}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo);
	void Free(void* ptr, size_t size);

	size_t ElementSize() const {return mElementSize;}

	forceinline size_t GetAllocationSize(void* ptr) const {(void)ptr; return mElementSize;}

	bool operator==(null_t) const {return mElementSize==0;}
	bool operator!=(null_t) const {return !operator==(null);}

private:
	FreeList mList;
	ushort mElementSize, mAlignment;
};

}}
