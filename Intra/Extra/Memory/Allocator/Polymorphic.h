#pragma once

#include "Core/Core.h"
#include "Core/Assert.h"

#include "Memory/Memory.h"

INTRA_BEGIN
class IAllocator
{
public:
	virtual AnyPtr Allocate(size_t bytes, const SourceInfo& sourceInfo = INTRA_DEFAULT_SOURCE_INFO) = 0;
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
	AnyPtr Allocate(size_t bytes, const SourceInfo& sourceInfo = INTRA_DEFAULT_SOURCE_INFO) final
	{
		return Allocator::Allocate(bytes, sourceInfo);
	}

	void Free(void* ptr) final
	{
		Allocator::Free(ptr);
	}
};

template<typename Allocator> class PolymorphicSizedAllocator: public ISizedAllocator, private Allocator
{
	AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo) final
	{
		return Allocator::Allocate(bytes, sourceInfo);
	}

	void Free(void* ptr) final
	{
		Allocator::Free(ptr);
	}

	size_t GetAllocationSize(void* ptr) const final { return Allocator::GetAllocationSize(ptr); }
};

template<typename Allocator> class PolymorphicAllocator: public TSelect<
	PolymorphicSizedAllocator<Allocator>,
	PolymorphicUnsizedAllocator<Allocator>,
	AllocatorHasGetAllocationSize<Allocator>::_>
{};

INTRA_END
