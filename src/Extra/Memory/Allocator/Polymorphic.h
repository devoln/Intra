#pragma once

#include "Intra/Assert.h"
#include "Extra/Memory/Memory.h"

INTRA_BEGIN
class IAllocator
{
public:
	virtual AnyPtr Allocate(size_t bytes, const SourceInfo& sourceInfo = SourceInfo()) = 0;
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
	AnyPtr Allocate(size_t bytes, const SourceInfo& sourceInfo = SourceInfo()) final
	{
		return Allocator::Allocate(bytes, sourceInfo);
	}

	void Free(void* ptr) final
	{
		Allocator::Free(ptr);
	}
};

template<typename Allocator> class INTRA_EMPTY_BASES PolymorphicSizedAllocator: public ISizedAllocator, private Allocator
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
	CHasGetAllocationSize<Allocator>>
{};

INTRA_END
