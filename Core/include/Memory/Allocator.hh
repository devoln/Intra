#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"
#include "Thread/Thread.h"
#include "Memory/Allocator/System.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/Generators/StringView.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Stream.h"
#include "Memory/PlacementNew.h"

#include "Allocator/Decorators.hh"
#include "Allocator/Basic.hh"
#include "Allocator/Compositors.hh"


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Memory {



#ifdef INTRA_DEBUG
using SizedHeapType = ASized<ABoundsChecked<ACallOnFail<SystemHeapAllocator, NoMemoryAbort>>>;
using GlobalHeapType = ABoundsChecked<ACallOnFail<SystemHeapAllocator, NoMemoryAbort>>;
#else
using SizedHeapType = ASized<ACallOnFail<SystemHeapAllocator, NoMemoryAbort>>;
using GlobalHeapType = ACallOnFail<SystemHeapAllocator, NoMemoryAbort>;
#endif

extern SizedHeapType SizedHeap;
extern GlobalHeapType GlobalHeap;




struct Buffer
{
	Buffer() = default;

	const byte* Data() const {return reinterpret_cast<const byte*>(this+1);}
	byte* Data() {return reinterpret_cast<byte*>(this+1);}
	const byte* End() const {return Data()+Size();}
	byte* End() {return Data()+Size();}
	size_t Size() const {return size;}
	template<typename T> ArrayRange<T> AsRange() {return {reinterpret_cast<T*>(Data()), size/sizeof(T)};}
	template<typename T> ArrayRange<const T> AsRange() const {return {reinterpret_cast<T*>(Data()), size/sizeof(T)};}

private:
	size_t size;
	
	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	friend struct BufferAllocator;
};

//TODO: объединить с SegregatedAllocator
struct BufferAllocator
{
	FreeList free_lists[32];
	ushort space_in_lists[32];
	size_t allocationCount, heapAllocationCount;

	BufferAllocator(): allocationCount(0), heapAllocationCount(0)
	{
		init_space_in_lists();
	}

	BufferAllocator(const BufferAllocator& rhs) = delete;

	static forceinline ushort GetBufferSizeCategory(size_t size)
	{
		return ushort(Aligned(size, 32)/32-1);
	}

	static forceinline size_t GetBufferSizeFromCategory(ushort category)
	{
		return 32u*(category+1u);
	}

	Buffer* AllocateBuffer(size_t& bytes, const SourceInfo& sourceInfo)
	{
		allocationCount++;
		size_t totalBytes = bytes+sizeof(Buffer);
		const ushort sizeCategory = GetBufferSizeCategory(totalBytes);
		Buffer* result;
		if(sizeCategory<Meta::NumOf(free_lists))
		{
			totalBytes = GetBufferSizeFromCategory(sizeCategory);
			bytes = totalBytes-sizeof(Buffer);
			result = free_lists[sizeCategory].Allocate();
			if(result!=null)
			{
				space_in_lists[sizeCategory]++;
				result->size = bytes;
				return result;
			}
		}
		result = GlobalHeap.Allocate(totalBytes, sourceInfo);
		heapAllocationCount++;
		result->size = bytes;
		return result;
	}

	void FreeBuffer(Buffer* buf)
	{
		if(buf==null) return;
		allocationCount--;
		const ushort sizeCategory = GetBufferSizeCategory(buf->size+sizeof(Buffer));
		if(sizeCategory<Meta::NumOf(free_lists) && space_in_lists[sizeCategory]>0)
		{
			free_lists[sizeCategory].Free(buf);
			space_in_lists[sizeCategory]--;
		}
		else
		{
			GlobalHeap.Free(buf, buf->Size()+sizeof(Buffer));
			heapAllocationCount--;
		}
	}

	forceinline AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo)
	{
		Buffer* result = AllocateBuffer(bytes, sourceInfo);
		return result->Data();
	}

	void Free(void* ptr, size_t size)
	{
		(void)size;
		if(ptr==null) return;
		Buffer* buf = reinterpret_cast<Buffer*>(ptr)-1;
		INTRA_ASSERT(size == buf->Size());
		FreeBuffer(buf);
	}

	forceinline size_t GetAllocationSize(void* ptr) const {return (reinterpret_cast<Buffer*>(ptr)-1)->Size();}

	void ClearFreeLists()
	{
		ushort i=0;
		for(auto& freeList: free_lists)
		{
			for(;;)
			{
				void* buffer = freeList.Allocate();
				if(buffer==null) break;
				GlobalHeap.Free(buffer, GetBufferSizeFromCategory(i));
				heapAllocationCount--;
				allocationCount--;
			}
			i++;
		}
		init_space_in_lists();
	}

	static forceinline BufferAllocator& Instance()
	{INTRA_OPTIONAL_THREAD_LOCAL static BufferAllocator allocator; return allocator;}

private:
	void init_space_in_lists()
	{
		for(size_t i=0; i<Meta::NumOf(space_in_lists); i++)
			space_in_lists[i] = ushort(1024/(i+1));
	}

	BufferAllocator& operator=(const BufferAllocator&) = delete;
};

//typedef AStatic<ASized<GlobalHeapType>> StaticBufferAllocator;
typedef AStatic<BufferAllocator> StaticBufferAllocator;



}

INTRA_WARNING_POP

}

#define INTRA_NEW(type, allocator) new((allocator).Allocate(sizeof(type), INTRA_SOURCE_INFO)) type
#define INTRA_DELETE(ptr, allocator) (Intra::Memory::DestructObj(*ptr), allocator.Free(ptr))

#define INTRA_NEW_ARRAY(type, n, allocator) Intra::Memory::AllocateRange<type>((allocator), (n), INTRA_SOURCE_INFO)
#define INTRA_DELETE_ARRAY(range, allocator) Intra::Memory::FreeRange((allocator), (range))

