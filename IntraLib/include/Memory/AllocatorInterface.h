#pragma once

#include "Core/FundamentalTypes.h"
#include "Core/Debug.h"
#include "Platform/CppWarnings.h"
#include "Memory/Memory.h"
#include "Memory/Allocator.h"


namespace Intra { namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class IAllocator
{
public:
	virtual AnyPtr Allocate(size_t bytes, const SourceInfo& sourceInfo) = 0;
	virtual void Free(void* ptr) = 0;
	virtual ~IAllocator() {}
};

class ISizedAllocator: public IAllocator
{
public:
	virtual size_t GetAllocationSize() const = 0;
};

template<typename Allocator> class PolymorphicUnsizedAllocator: public IAllocator, private Allocator
{
	AnyPtr Allocate(size_t bytes, const SourceInfo& sourceInfo) override final
	{
		return Allocator::Allocate(bytes, sourceInfo);
	}

	void Free(void* ptr) override final
	{
		Allocator::Free(ptr);
	}
};

template<typename Allocator> class PolymorphicSizedAllocator: public ISizedAllocator, private Allocator
{
	AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo) override final
	{
		return Allocator::Allocate(bytes, sourceInfo);
	}

	void Free(void* ptr) override final
	{
		Allocator::Free(ptr);
	}

	size_t GetAllocationSize(void* ptr) const override final {return Allocator::GetAllocationSize(ptr);}
};

template<typename Allocator> class PolymorphicAllocator: public Meta::SelectType<
	PolymorphicSizedAllocator<Allocator>,
	PolymorphicUnsizedAllocator<Allocator>,
	AllocatorHasGetAllocationSize<Allocator>::_> {};

template<typename Allocator, typename PARENT=Meta::EmptyType, bool Stateless=Meta::IsEmptyClass<Allocator>::_> struct AllocatorRef;

template<typename Allocator, typename PARENT> struct AllocatorRef<Allocator, PARENT, false>: PARENT
{
	forceinline explicit AllocatorRef(null_t): mAllocator(null) {}
	forceinline AllocatorRef(Allocator& allocatorRef): mAllocator(&allocatorRef) {}

	forceinline AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo) const
	{
		INTRA_ASSERT(mAllocator!=null);
		return mAllocator->Allocate(bytes, sourceInfo);
	}

	forceinline void Free(void* ptr, size_t size) const
	{
		INTRA_ASSERT(mAllocator!=null);
		mAllocator->Free(ptr, size);
	}

	template<typename U=Allocator> forceinline Meta::EnableIf<
		AllocatorHasGetAllocationSize<U>::_,
	size_t> GetAllocationSize(void* ptr) const
	{
		INTRA_ASSERT(mAllocator!=null);
		return mAllocator->GetAllocationSize(ptr);
	}

	Allocator& GetRef() {return *mAllocator;}

	template<typename T> ArrayRange<T> AllocateRangeUninitialized(size_t& count, const SourceInfo& sourceInfo)
	{return Memory::AllocateRangeUninitialized<T>(*mAllocator, count, sourceInfo);}

	template<typename T> ArrayRange<T> AllocateRange(size_t& count, const SourceInfo& sourceInfo)
	{return Memory::AllocateRange<T>(*mAllocator, count, sourceInfo);}

protected:
	Allocator* mAllocator;
};

template<typename Allocator> struct AllocatorRef<Allocator, Meta::EmptyType, true>: Allocator
{
	forceinline AllocatorRef(null_t=null) {}
	forceinline AllocatorRef(Allocator& allocator) {(void)allocator;}

	AllocatorRef& operator=(const AllocatorRef&) {return *this;}

	Allocator& GetRef() const {return *const_cast<Allocator*>(static_cast<const Allocator*>(this));}

	template<typename T> ArrayRange<T> AllocateRangeUninitialized(size_t& count, const SourceInfo& sourceInfo)
	{return Memory::AllocateRangeUninitialized<T>(*this, count, sourceInfo);}

	template<typename T> ArrayRange<T> AllocateRange(size_t& count, const SourceInfo& sourceInfo)
	{return Memory::AllocateRange<T>(*this, count, sourceInfo);}
};

template<typename Allocator, typename PARENT> struct AllocatorRef<Allocator, PARENT, true>: PARENT, AllocatorRef<Allocator, Meta::EmptyType, true>
{
	forceinline AllocatorRef(null_t=null) {}
	forceinline AllocatorRef(Allocator& allocator) {(void)allocator;}
};

INTRA_WARNING_POP

}}
