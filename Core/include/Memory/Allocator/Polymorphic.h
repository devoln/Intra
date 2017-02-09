#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/Debug.h"
#include "Platform/CppWarnings.h"
#include "Memory/Memory.h"

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

	size_t GetAllocationSize(void* ptr) const override final { return Allocator::GetAllocationSize(ptr); }
};

template<typename Allocator> class PolymorphicAllocator: public Meta::SelectType<
	PolymorphicSizedAllocator<Allocator>,
	PolymorphicUnsizedAllocator<Allocator>,
	AllocatorHasGetAllocationSize<Allocator>::_>
{};

INTRA_WARNING_POP

}}
