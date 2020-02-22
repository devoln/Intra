#pragma once

#include "Core/Assert.h"
#include "Core/Type.h"
#include "Memory/Memory.h"
#include "Memory/Allocator/Concepts.h"

INTRA_BEGIN

template<typename A, typename PARENT=EmptyType, bool Stateless=CEmptyClass<A>> struct AllocatorRef;

template<typename A, typename PARENT> struct AllocatorRef<A, PARENT, false>: PARENT
{
	forceinline AllocatorRef(null_t=null): mAllocator() {}
	forceinline AllocatorRef(A& allocatorRef): mAllocator(&allocatorRef) {}

	forceinline AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO) const
	{
		INTRA_PRECONDITION(mAllocator != null);
		return mAllocator->Allocate(bytes, sourceInfo);
	}

	forceinline void Free(void* ptr, size_t size) const
	{
		INTRA_PRECONDITION(mAllocator != null);
		mAllocator->Free(ptr, size);
	}

	template<typename U=A> forceinline Requires<
		CHasGetAllocationSize<U>,
	size_t> GetAllocationSize(void* ptr) const
	{
		INTRA_PRECONDITION(mAllocator != null);
		return mAllocator->GetAllocationSize(ptr);
	}

	forceinline A& GetRef() {return *mAllocator;}

	template<typename T> forceinline Span<T> AllocateRangeUninitialized(size_t& count, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{return AllocateRangeUninitialized<T>(*mAllocator, count, sourceInfo);}

	template<typename T> forceinline Span<T> AllocateRange(size_t& count, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{return AllocateRange<T>(*mAllocator, count, sourceInfo);}

	template<typename T> forceinline void FreeRangeUninitialized(Span<T> range)
	{return FreeRangeUninitialized(*mAllocator, range);}

	template<typename T> forceinline void FreeRange(Span<T> range)
	{return FreeRange(*mAllocator, range);}

protected:
	A* mAllocator;
};

template<typename A> struct AllocatorRef<A, EmptyType, true>: A
{
	forceinline AllocatorRef(null_t=null) {}
	forceinline AllocatorRef(A& allocator) {(void)allocator;}

	AllocatorRef& operator=(const AllocatorRef&) {return *this;}

	forceinline A& GetRef() const {return *const_cast<A*>(static_cast<const A*>(this));}

	template<typename T> forceinline Span<T> AllocateRangeUninitialized(size_t& count, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{return AllocateRangeUninitialized<T>(*this, count, sourceInfo);}

	template<typename T> forceinline Span<T> AllocateRange(size_t& count, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{return AllocateRange<T>(*this, count, sourceInfo);}
	
	template<typename T> forceinline void FreeRangeUninitialized(Span<T> range)
	{return FreeRangeUninitialized(*this, range);}

	template<typename T> forceinline void FreeRange(Span<T> range)
	{return FreeRange(*this, range);}
};

template<typename A, typename PARENT> struct AllocatorRef<A, PARENT, true>:
	PARENT, AllocatorRef<A, EmptyType, true>
{
	forceinline AllocatorRef(null_t=null) {}
	forceinline AllocatorRef(A& allocator) {(void)allocator;}
};

INTRA_END
