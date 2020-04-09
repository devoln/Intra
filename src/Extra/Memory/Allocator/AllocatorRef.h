#pragma once

#include "Intra/Assert.h"
#include "Intra/Type.h"
#include "Extra/Memory/Memory.h"
#include "Extra/Memory/Allocator/Concepts.h"

INTRA_BEGIN

template<typename A, typename PARENT = EmptyType, bool Stateless = CEmptyClass<A>> struct AllocatorRef;

template<typename A, typename PARENT> struct AllocatorRef<A, PARENT, false>: PARENT
{
	AllocatorRef(decltype(null)=null): mAllocator() {}
	AllocatorRef(A& allocatorRef): mAllocator(&allocatorRef) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo()) const
	{
		INTRA_PRECONDITION(mAllocator != null);
		return mAllocator->Allocate(bytes, sourceInfo);
	}

	void Free(void* ptr, size_t size) const
	{
		INTRA_PRECONDITION(mAllocator != null);
		mAllocator->Free(ptr, size);
	}

	template<typename U=A> Requires<
		CHasGetAllocationSize<U>,
	size_t> GetAllocationSize(void* ptr) const
	{
		INTRA_PRECONDITION(mAllocator != null);
		return mAllocator->GetAllocationSize(ptr);
	}

	A& GetRef() {return *mAllocator;}

	template<typename T> Span<T> AllocateRangeUninitialized(size_t& count, SourceInfo sourceInfo = SourceInfo())
	{return AllocateRangeUninitialized<T>(*mAllocator, count, sourceInfo);}

	template<typename T> Span<T> AllocateRange(size_t& count, SourceInfo sourceInfo = SourceInfo())
	{return AllocateRange<T>(*mAllocator, count, sourceInfo);}

	template<typename T> void FreeRangeUninitialized(Span<T> range)
	{return FreeRangeUninitialized(*mAllocator, range);}

	template<typename T> void FreeRange(Span<T> range)
	{return FreeRange(*mAllocator, range);}

protected:
	A* mAllocator;
};

template<typename A> struct AllocatorRef<A, EmptyType, true>: A
{
	AllocatorRef(decltype(null)=null) {}
	AllocatorRef(A& allocator) {(void)allocator;}

	AllocatorRef& operator=(const AllocatorRef&) {return *this;}

	A& GetRef() const {return *const_cast<A*>(static_cast<const A*>(this));}

	template<typename T> Span<T> AllocateRangeUninitialized(size_t& count, SourceInfo sourceInfo = SourceInfo())
	{return AllocateRangeUninitialized<T>(*this, count, sourceInfo);}

	template<typename T> Span<T> AllocateRange(size_t& count, SourceInfo sourceInfo = SourceInfo())
	{return AllocateRange<T>(*this, count, sourceInfo);}
	
	template<typename T> void FreeRangeUninitialized(Span<T> range)
	{return FreeRangeUninitialized(*this, range);}

	template<typename T> void FreeRange(Span<T> range)
	{return FreeRange(*this, range);}
};

template<typename A, typename PARENT> struct AllocatorRef<A, PARENT, true>:
	PARENT, AllocatorRef<A, EmptyType, true>
{
	AllocatorRef(decltype(null)=null) {}
	AllocatorRef(A& allocator) {(void)allocator;}
};

INTRA_END
