#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Warnings.h"
#include "Utils/AnyPtr.h"
#include "Utils/Debug.h"
#include "Meta/Type.h"
#include "Memory/Memory.h"
#include "Concepts.h"

namespace Intra { namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename A, typename PARENT=Meta::EmptyType, bool Stateless=Meta::IsEmptyClass<A>::_> struct AllocatorRef;

template<typename A, typename PARENT> struct AllocatorRef<A, PARENT, false>: PARENT
{
	forceinline AllocatorRef(null_t=null): mAllocator() {}
	forceinline AllocatorRef(A& allocatorRef): mAllocator(&allocatorRef) {}

	forceinline AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo) const
	{
		INTRA_DEBUG_ASSERT(mAllocator!=null);
		return mAllocator->Allocate(bytes, sourceInfo);
	}

	forceinline void Free(void* ptr, size_t size) const
	{
		INTRA_DEBUG_ASSERT(mAllocator!=null);
		mAllocator->Free(ptr, size);
	}

	template<typename U=A> forceinline Meta::EnableIf<
		HasGetAllocationSize<U>::_,
	size_t> GetAllocationSize(void* ptr) const
	{
		INTRA_DEBUG_ASSERT(mAllocator!=null);
		return mAllocator->GetAllocationSize(ptr);
	}

	A& GetRef() {return *mAllocator;}

	template<typename T> Span<T> AllocateRangeUninitialized(size_t& count, SourceInfo sourceInfo)
	{return Memory::AllocateRangeUninitialized<T>(*mAllocator, count, sourceInfo);}

	template<typename T> Span<T> AllocateRange(size_t& count, SourceInfo sourceInfo)
	{return Memory::AllocateRange<T>(*mAllocator, count, sourceInfo);}

protected:
	A* mAllocator;
};

template<typename A> struct AllocatorRef<A, Meta::EmptyType, true>: A
{
	forceinline AllocatorRef(null_t=null) {}
	forceinline AllocatorRef(A& allocator) {(void)allocator;}

	AllocatorRef& operator=(const AllocatorRef&) {return *this;}

	A& GetRef() const {return *const_cast<A*>(static_cast<const A*>(this));}

	template<typename T> Span<T> AllocateRangeUninitialized(size_t& count, SourceInfo sourceInfo)
	{return Memory::AllocateRangeUninitialized<T>(*this, count, sourceInfo);}

	template<typename T> Span<T> AllocateRange(size_t& count, SourceInfo sourceInfo)
	{return Memory::AllocateRange<T>(*this, count, sourceInfo);}
};

template<typename A, typename PARENT> struct AllocatorRef<A, PARENT, true>:
	PARENT, AllocatorRef<A, Meta::EmptyType, true>
{
	forceinline AllocatorRef(null_t=null) {}
	forceinline AllocatorRef(A& allocator) {(void)allocator;}
};

INTRA_WARNING_POP

}}
