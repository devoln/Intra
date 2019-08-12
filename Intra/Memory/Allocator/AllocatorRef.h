#pragma once

#include "Core/Core.h"

#include "Utils/AnyPtr.h"
#include "Core/Assert.h"
#include "Core/Type.h"
#include "Memory/Memory.h"
#include "Core.h"

INTRA_BEGIN
namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename A, typename PARENT=Core::EmptyType, bool Stateless=CEmptyClass<A>::_> struct AllocatorRef;

template<typename A, typename PARENT> struct AllocatorRef<A, PARENT, false>: PARENT
{
	forceinline AllocatorRef(null_t=null): mAllocator() {}
	forceinline AllocatorRef(A& allocatorRef): mAllocator(&allocatorRef) {}

	forceinline AnyPtr Allocate(size_t& bytes, const Utils::SourceInfo& sourceInfo = INTRA_DEFAULT_SOURCE_INFO) const
	{
		INTRA_DEBUG_ASSERT(mAllocator != null);
		return mAllocator->Allocate(bytes, sourceInfo);
	}

	forceinline void Free(void* ptr, size_t size) const
	{
		INTRA_DEBUG_ASSERT(mAllocator != null);
		mAllocator->Free(ptr, size);
	}

	template<typename U=A> forceinline Requires<
		HasGetAllocationSize<U>::_,
	size_t> GetAllocationSize(void* ptr) const
	{
		INTRA_DEBUG_ASSERT(mAllocator!=null);
		return mAllocator->GetAllocationSize(ptr);
	}

	forceinline A& GetRef() {return *mAllocator;}

	template<typename T> forceinline Span<T> AllocateRangeUninitialized(size_t& count, const Utils::SourceInfo& sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{return Memory::AllocateRangeUninitialized<T>(*mAllocator, count, sourceInfo);}

	template<typename T> forceinline Span<T> AllocateRange(size_t& count, const Utils::SourceInfo& sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{return Memory::AllocateRange<T>(*mAllocator, count, sourceInfo);}

	template<typename T> forceinline void FreeRangeUninitialized(Span<T> range)
	{return Memory::FreeRangeUninitialized(*mAllocator, range);}

	template<typename T> forceinline void FreeRange(Span<T> range)
	{return Memory::FreeRange(*mAllocator, range);}

protected:
	A* mAllocator;
};

template<typename A> struct AllocatorRef<A, Core::EmptyType, true>: A
{
	forceinline AllocatorRef(null_t=null) {}
	forceinline AllocatorRef(A& allocator) {(void)allocator;}

	AllocatorRef& operator=(const AllocatorRef&) {return *this;}

	forceinline A& GetRef() const {return *const_cast<A*>(static_cast<const A*>(this));}

	template<typename T> forceinline Span<T> AllocateRangeUninitialized(size_t& count, const Utils::SourceInfo& sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{return Memory::AllocateRangeUninitialized<T>(*this, count, sourceInfo);}

	template<typename T> forceinline Span<T> AllocateRange(size_t& count, const Utils::SourceInfo& sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{return Memory::AllocateRange<T>(*this, count, sourceInfo);}
	
	template<typename T> forceinline void FreeRangeUninitialized(Span<T> range)
	{return Memory::FreeRangeUninitialized(*this, range);}

	template<typename T> forceinline void FreeRange(Span<T> range)
	{return Memory::FreeRange(*this, range);}
};

template<typename A, typename PARENT> struct AllocatorRef<A, PARENT, true>:
	PARENT, AllocatorRef<A, Core::EmptyType, true>
{
	forceinline AllocatorRef(null_t=null) {}
	forceinline AllocatorRef(A& allocator) {(void)allocator;}
};

INTRA_WARNING_POP

}}
