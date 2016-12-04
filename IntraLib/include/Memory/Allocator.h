#pragma once

#include "Core/Core.h"
#include "Threading/Thread.h"
#include "Memory/SystemAllocators.h"
#include "Range/ArrayRange.h"

namespace Intra { namespace Memory {

inline size_t AlignmentBytes(size_t value, size_t alignment)
{
	INTRA_ASSERT(alignment!=0);
	size_t remainder = value % alignment;
	if(remainder==0) return 0;
	return alignment-remainder;
}

inline size_t Aligned(size_t value, size_t alignment, size_t offset=0)
{
	return value+AlignmentBytes(value+offset, alignment);
}

inline byte* Aligned(void* value, size_t alignment, size_t offset=0)
{
	return reinterpret_cast<byte*>(value)+AlignmentBytes(reinterpret_cast<size_t>(value)+offset, alignment);
}



INTRA_DEFINE_EXPRESSION_CHECKER(AllocatorHasGetAllocationSize,\
	static_cast<size_t>(Meta::Val<T>().GetAllocationSize(static_cast<void*>(null))));


template<typename Allocator> struct BreakpointOnError: Allocator
{
	template<typename... Args> explicit BreakpointOnError(Args&&... args): Allocator(core::forward<Args>(args)...) {}
	BreakpointOnError(const BreakpointOnError& rhs): Allocator(rhs) {}
	BreakpointOnError(BreakpointOnError&& rhs): Allocator(core::move(static_cast<Allocator&>(rhs))) {}

	AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo)
	{
		auto result = Allocator::Allocate(bytes, sourceInfo);
		if(result==null) INTRA_DEBUGGER_BREAKPOINT;
		return result;
	}
};

template<typename Allocator> struct AbortOnError: Allocator
{
	template<typename... Args> explicit AbortOnError(Args&&... args): Allocator(core::forward<Args>(args)...) {}
	AbortOnError(const AbortOnError& rhs) = default;

	AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo)
	{
		auto result = Allocator::Allocate(bytes, sourceInfo);
		if(result!=null) return result;

		char errorMsg[256];
		auto msgRange = ArrayRange<char>(errorMsg);
		ArrayRange<const char>("Недостаточно памяти!\nНе удаётся выделить блок ").DropBack().CopyToAdvance(msgRange);
		msgRange.AppendAdvance(bytes);
		ArrayRange<const char>(" байт памяти.").CopyToAdvance(msgRange);
		return INTRA_INTERNAL_ERROR(errorMsg), null;
	}
};

template<typename Allocator> struct BoundsChecked: Allocator
{
private:
	enum: uint {BoundValue = 0xbcbcbcbc};
public:
	template<typename... Args> explicit BoundsChecked(Args&&... args): Allocator(core::forward<Args>(args)...) {}
	BoundsChecked(const BoundsChecked& rhs) = default;

	size_t GetAlignment() const {return sizeof(uint);}

	AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo)
	{
		bytes = Aligned(bytes, 4);
		size_t totalBytes = bytes + 2*sizeof(uint);
		byte* plainMemory = Allocator::Allocate(totalBytes, sourceInfo);
		if(plainMemory!=null)
		{
			bytes = totalBytes - 2*sizeof(uint);
			*reinterpret_cast<uint*>(plainMemory) = BoundValue;
			*reinterpret_cast<uint*>(plainMemory+sizeof(uint)+bytes) = BoundValue;
			return plainMemory+sizeof(uint);
		}
		bytes = 0;
		return null;
	}

	void Free(void* ptr, size_t size)
	{
		if(ptr==null) return;
		size = Aligned(size, 4);
		byte* plainMemory = reinterpret_cast<byte*>(ptr)-sizeof(uint);

		uint leftBoundValue = *reinterpret_cast<uint*>(plainMemory);
		if(leftBoundValue!=BoundValue)
			INTRA_INTERNAL_ERROR("Allocator left bound check failed!");

		uint rightBoundValue = *reinterpret_cast<uint*>(plainMemory+size+sizeof(uint));
		if(rightBoundValue!=BoundValue)
			INTRA_INTERNAL_ERROR("Allocator right bound check failed!");

		Allocator::Free(plainMemory, size+2*sizeof(uint));
	}

	template<typename U=Allocator> Meta::EnableIf<
		AllocatorHasGetAllocationSize<U>::_,
	size_t> GetAllocationSize(void* ptr) const
	{
		byte* bptr = reinterpret_cast<byte*>(ptr);
		return Allocator::GetAllocationSize(bptr-sizeof(uint)) - sizeof(uint)*2;
	}
};



template<typename Allocator> struct AllocationCounted: Allocator
{
	size_t counter;
public:
	template<typename... Args> explicit AllocationCounted(Args&&... args): Allocator(core::forward<Args>(args)...), counter(0) {}
	AllocationCounted(const AllocationCounted& rhs) = default;


	AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo)
	{
		auto result = Allocator::Allocate(bytes, sourceInfo);
		if(result!=null) counter++;
		return result;
	}

	void Free(void* ptr, size_t size)
	{
		if(ptr==null) return;
		Allocator::Free(ptr, size);
		counter--;
	}

	size_t AllocationCount() {return counter;}
};


template<typename Allocator, typename SyncPrimitive> struct Synchronized: Allocator
{
	template<typename... Args> explicit Synchronized(Args&&... args): Allocator(core::forward<Args>(args)...) {}
	Synchronized(const Synchronized& rhs) = default;

	AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo)
	{
		primitive.Lock();
		auto result = Allocator::Allocate(bytes, sourceInfo);
		primitive.Unlock();
		return result;
	}

	void Free(void* ptr, size_t size)
	{
		if(ptr==null) return;
		primitive.Lock();
		Allocator::Free(ptr, size);
		primitive.Unlock();
	}

	Synchronized& operator=(const Synchronized& rhs) = default;
	Synchronized& operator=(Synchronized&& rhs) {Allocator::operator=(core::move(rhs)); primitive=rhs.primitive; return *this;}

private:
	SyncPrimitive primitive;
};


template<class Allocator> struct SizedAllocator: Allocator
{
	size_t GetAlignment() const {return sizeof(size_t);}

	template<typename... Args> explicit SizedAllocator(Args&&... args): Allocator(core::forward<Args>(args)...) {}
	SizedAllocator(const SizedAllocator& rhs) = default;

	AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo)
	{
		INTRA_ASSERT(bytes!=0);
		size_t totalBytes = bytes+sizeof(size_t);
		size_t* data = Allocator::Allocate(totalBytes, sourceInfo);
		if(data!=null)
		{
			bytes = totalBytes-sizeof(size_t);
			*data++ = bytes;
		}
		return data;
	}
 
	/*AnyPtr Reallocate(void* ptr, size_t newBytes)
	{
		size_t* newData = Allocator::Reallocate(ptr!=null? (size_t*)ptr-1: null, newBytes+sizeof(size_t));
		if(newData!=null) *newData = newBytes;
		return newData+1;
	}*/

	void Free(void* ptr, size_t size)
	{
		INTRA_ASSERT(GetAllocationSize(ptr)==size);
		size_t* originalPtr = reinterpret_cast<size_t*>(ptr)-1;
		Allocator::Free(originalPtr, size+sizeof(size_t));
	}

	forceinline size_t GetAllocationSize(void* ptr) const
	{
		return *(reinterpret_cast<size_t*>(ptr)-1);
	}
};

template<typename Allocator> struct StaticAllocator
{
	size_t GetAlignment() const {return Get().GetAlignment();}

	static Allocator& Get()
	{
		static Allocator allocator;
		return allocator;
	}

	static AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo)
	{
		return Get().Allocate(bytes, sourceInfo);
	}

	static void Free(void* ptr, size_t size)
	{
		Get().Free(ptr, size);
	}

	template<typename U=Allocator> static forceinline Meta::EnableIf<
		AllocatorHasGetAllocationSize<U>::_,
	size_t> GetAllocationSize(void* ptr)
	{
		return Get().GetAllocationSize(ptr);
	}
};



struct LinearAllocator
{
	LinearAllocator(ArrayRange<byte> buf, size_t allocatorAlignment=16):
		start(buf.Begin), rest(buf), alignment(allocatorAlignment) {}

	size_t GetAlignment() const {return alignment;}

	AnyPtr Allocate(size_t bytes, const SourceInfo& sourceInfo)
	{
		(void)sourceInfo;

		byte* userPtr = Aligned(rest.Begin, alignment);
		if(rest.Begin + bytes >= rest.End) return null;
		rest.Begin += bytes;

		return userPtr;
	}

	void Free(void* ptr, size_t size) {(void)ptr; (void)size;}

	void Reset() {rest.Begin = start;}

private:
	byte* start;
	ArrayRange<byte> rest;
	size_t alignment;
};


struct StackAllocator
{
	StackAllocator(ArrayRange<byte> buf, size_t allocatorAlignment):
		start(buf.Begin), rest(buf), alignment(allocatorAlignment) {}
	
	size_t GetAlignment() const {return alignment;}
	AnyPtr Allocate(size_t size, const SourceInfo& sourceInfo);
	void Free(void* ptr, size_t size);

private:
	byte* start;
	ArrayRange<byte> rest;
	size_t alignment;
};





struct FreeList
{
	FreeList(null_t=null): next(null) {}
	FreeList(ArrayRange<byte> buf, size_t elementSize, size_t alignment): next(null)
	{
		InitBuffer(buf, elementSize, alignment);
	}

	void InitBuffer(ArrayRange<byte> buf, size_t elementSize, size_t alignment);

	AnyPtr Allocate()
	{
		if(next==null) return null;
		FreeList* head = next;
		next = head->next;
		return head;
	}

	void Free(void* ptr)
	{
		FreeList* head = reinterpret_cast<FreeList*>(ptr);
		head->next = next;
		next = head;
	}

	bool HasFree() const {return next!=null;}

private:
	FreeList* next;
};

template<typename Allocator> struct GrowingFreeList: Allocator
{
	GrowingFreeList(null_t=null): first_block(null), list(), capacity(0), element_size(0), element_alignment(0) {}
	GrowingFreeList(size_t initialSize, size_t elementSize, size_t alignment, const SourceInfo& sourceInfo)
	{
		Init(initialSize, elementSize, alignment, sourceInfo);
	}

	~GrowingFreeList()
	{
		while(next_block() != null)
		{
			void* next = next_block();
			Allocator::Free(first_block);
			first_block = next;
		}
		Allocator::Free(first_block);
	}

	void Init(size_t initialSize, size_t elementSize, size_t alignment, const SourceInfo& sourceInfo)
	{
		if(first_block!=null)
		{
			INTRA_ASSERT(element_size==elementSize);
			INTRA_ASSERT(element_alignment==alignment);
			return;
		}
		first_block = Allocator::Allocate(block_size(initialSize), sourceInfo);
		list.InitBuffer(ArrayRange<byte>(reinterpret_cast<byte*>(first_block)+block_size(0), initialSize), elementSize, alignment),
		capacity = initialSize;
		element_size = ushort(elementSize);
		element_alignment = ushort(alignment);
		next_block() = null;
	}


	size_t GetAlignment() const {return element_alignment;}


	AnyPtr Allocate()
	{
		if(!list.HasFree())
		{
			auto newBlock = Allocator::Allocate(block_size(capacity), INTRA_SOURCE_INFO);
			*reinterpret_cast<void**>(newBlock) = first_block;
			first_block = newBlock;
			new(&list) FreeList(ArrayRange<byte>(reinterpret_cast<byte*>(first_block)+block_size(0), capacity), element_size, element_alignment);
			capacity*=2;
		}
		return list.Allocate();
	}

	AnyPtr Allocate(size_t bytes, const SourceInfo& sourceInfo)
	{
		(void)sourceInfo; (void)bytes;
		INTRA_ASSERT(bytes<=element_size);
		return Allocate();
	}

	void Free(void* ptr)
	{
		list.Free(ptr);
	}

	forceinline size_t GetAllocationSize(void* ptr) const {(void)ptr; return element_size;}

private:
	void* first_block;
	FreeList list;
	size_t capacity;
	ushort element_size, element_alignment;

	void*& next_block() {return *reinterpret_cast<void**>(first_block);}
	static size_t block_size(size_t bytes) {return bytes+sizeof(void**);}
};

struct PoolAllocator
{
	PoolAllocator(ArrayRange<byte> buf, size_t elementSize, size_t allocatorAlignment):
		list(buf, elementSize, allocatorAlignment),
		element_size(ushort(elementSize)),
		alignment(ushort(allocatorAlignment)) {}


	size_t GetAlignment() const {return alignment;}

	AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo)
	{
		(void)sourceInfo;
		INTRA_ASSERT(bytes <= element_size);
		bytes = element_size;
		return list.Allocate();
	}

	void Free(void* ptr, size_t size)
	{
		(void)size;
		INTRA_ASSERT(size == ElementSize());
		list.Free(ptr);
	}

	size_t ElementSize() const {return element_size;}

	forceinline size_t GetAllocationSize(void* ptr) const {(void)ptr; return element_size;}

private:
	FreeList list;
	ushort element_size, alignment;
};

struct SegregatedTraits
{
    static size_t GetSizeClass(size_t size)
    {
		auto Log = Math::Log2i(uint(size));
        return size_t(Log>5? Log-5u: 0u);
    }
 
    static size_t GetSizeClassMaxSize(size_t sizeClass)
    {
        return size_t(32u << sizeClass);
    }
};

template<size_t NumBins, typename Traits, typename LittleAllocator, typename BigAllocator>
struct SegregatedAllocator: BigAllocator
{
	template<typename... Args> explicit SegregatedAllocator(Args&&... args):
		BigAllocator(core::forward<Args>(args)...)
	{
		alignment = BigAllocator::GetAlignment();
		for(auto& allocator: little_allocators)
			alignment = Math::Min(alignment, allocator.GetAlignment());
	}

	SegregatedAllocator(const SegregatedAllocator& rhs) = delete;

	SegregatedAllocator(SegregatedAllocator&& rhs):
		BigAllocator(core::move(static_cast<BigAllocator&>(rhs))),
		little_allocators(core::move(rhs.little_allocators)), alignment(rhs.alignment) {}

	size_t GetSizeClass(size_t size)
	{
		if(size>Traits::GetSizeClassMaxSize(NumBins-1))
			return NumBins;
		return Traits::GetSizeClass(size);
	}


	size_t GetAlignment() const {return alignment;}

    AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo)
    {
        size_t sizeClass = GetSizeClass(bytes);
 
        if(sizeClass >= NumBins)
            return BigAllocator::Allocate(bytes, sourceInfo);
 
        size_t maxSizeClass = Traits::GetSizeClassMaxSize(sizeClass);
        return little_allocators[sizeClass].Allocate(maxSizeClass, sourceInfo);
    }

#if INTRA_DISABLED
	void* Reallocate(void* ptr, size_t bytes)
    {
        if(ptr==null) return Allocate(bytes);

        if(bytes==0)
		{
			Free(ptr);
			return null;
		}

		size_t elementSize = GetAllocationSize(ptr);
		size_t elementSizeClass = GetSizeClass(elementSize);
 
		if(elementSizeClass >= NumBins) return BigAllocator::Reallocate(ptr, bytes);
 
		if(bytes<=elementSize && bytes>=elementSize/2) return ptr;

		void* newPtr = Allocate(bytes);
 
		if(newPtr!=null)
		{
			size_t copySize = Math::Min(elementSize, bytes);
			core::memcpy(newPtr, ptr, copySize);
		}
		little_allocators[elementSizeClass].Free(ptr);
		return newPtr;
    }
#endif

	void Free(void* ptr, size_t size)
	{
		if(ptr==null) return;
		size_t elementSizeClass = GetSizeClass(size);
		if(elementSizeClass >= NumBins) BigAllocator::Free(ptr, size);
		else little_allocators[elementSizeClass].Free(ptr, size);
	}

private:
	LittleAllocator little_allocators[NumBins];
	size_t alignment;
};



#ifdef INTRA_DEBUG
using SizedHeapType = SizedAllocator<BoundsChecked<AbortOnError<SystemHeapAllocator>>>;
using GlobalHeapType = BoundsChecked<AbortOnError<SystemHeapAllocator>>;
#else
using SizedHeapType = SizedAllocator<AbortOnError<SystemHeapAllocator>>;
using GlobalHeapType = AbortOnError<SystemHeapAllocator>;
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
		if(sizeCategory<core::numof(free_lists))
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
		if(sizeCategory<core::numof(free_lists) && space_in_lists[sizeCategory]>0)
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

	forceinline AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo)
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

	static forceinline BufferAllocator& Instance() {INTRA_OPTIONAL_THREAD_LOCAL static BufferAllocator allocator; return allocator;}

private:
	void init_space_in_lists()
	{
		for(size_t i=0; i<core::numof(space_in_lists); i++)
			space_in_lists[i] = ushort(1024/(i+1));
	}

	BufferAllocator& operator=(const BufferAllocator&) = delete;
};

//typedef StaticAllocator<SizedAllocator<GlobalHeapType>> StaticBufferAllocator;
typedef StaticAllocator<BufferAllocator> StaticBufferAllocator;



}

}

#define INTRA_NEW(type, allocator) new((allocator).Allocate(sizeof(type), INTRA_SOURCE_INFO)) type
#define INTRA_DELETE(ptr, allocator) (Intra::Memory::DestructObj(*ptr), allocator.Free(ptr))

#define INTRA_NEW_ARRAY(type, n, allocator) Intra::Memory::AllocateRange<type>((allocator), (n), INTRA_SOURCE_INFO)
#define INTRA_DELETE_ARRAY(range, allocator) Intra::Memory::FreeRange((allocator), (range))

