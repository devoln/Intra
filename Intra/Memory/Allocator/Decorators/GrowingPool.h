#pragma once

#include "Core/Core.h"
#include "Core/Assert.h"
#include "Memory/Allocator/Basic/Pool.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

INTRA_BEGIN
namespace Memory {

template<typename A> struct AGrowingPool: A
{
	AGrowingPool(null_t=null):
		mFirstBlock(null), mList(), mCapacity(0),
		mElementSize(0), mElementAlignment(0) {}

	AGrowingPool(size_t initialSize, size_t elementSize, size_t alignment, SourceInfo sourceInfo)
	{Init(initialSize, elementSize, alignment, sourceInfo);}

	~AGrowingPool()
	{
		while(nextBlock() != null)
		{
			void* next = nextBlock();
			A::Free(mFirstBlock);
			mFirstBlock = next;
		}
		A::Free(mFirstBlock);
	}

	void Init(size_t initialSize, size_t elementSize, size_t alignment, SourceInfo sourceInfo)
	{
		if(mFirstBlock!=null)
		{
			INTRA_DEBUG_ASSERT(mElementSize==elementSize);
			INTRA_DEBUG_ASSERT(mElementAlignment==alignment);
			return;
		}
		mFirstBlock = A::Allocate(blockSize(initialSize), sourceInfo);
		Span<byte> buf = {reinterpret_cast<byte*>(mFirstBlock)+blockSize(0), initialSize};
		mList.InitBuffer(buf, elementSize, alignment),
		mCapacity = initialSize;
		mElementSize = ushort(elementSize);
		mElementAlignment = ushort(alignment);
		nextBlock() = null;
	}


	size_t GetAlignment() const {return mElementAlignment;}


	AnyPtr Allocate()
	{
		if(!mList.HasFree())
		{
			auto newBlock = A::Allocate(blockSize(mCapacity), INTRA_SOURCE_INFO);
			*reinterpret_cast<void**>(newBlock) = mFirstBlock;
			mFirstBlock = newBlock;
			Span<byte> newBuf(reinterpret_cast<byte*>(mFirstBlock)+blockSize(0), mCapacity);
			new(&mList) FreeList(newBuf, mElementSize, mElementAlignment);
			mCapacity *= 2;
		}
		return mList.Allocate();
	}

	AnyPtr Allocate(size_t bytes, SourceInfo sourceInfo)
	{
		(void)sourceInfo; (void)bytes;
		INTRA_DEBUG_ASSERT(bytes <= mElementSize);
		return Allocate();
	}

	void Free(void* ptr) {mList.Free(ptr);}

	forceinline size_t GetAllocationSize(void* ptr) const {(void)ptr; return mElementSize;}

private:
	void* mFirstBlock;
	FreeList mList;
	size_t mCapacity;
	ushort mElementSize, mElementAlignment;

	void*& nextBlock() {return *reinterpret_cast<void**>(mFirstBlock);}
	static size_t blockSize(size_t bytes) {return bytes+sizeof(void**);}
};

}}

INTRA_WARNING_POP

