#pragma once

#include "IntraX/Memory/VirtualMemory.h"

namespace Intra { INTRA_BEGIN

struct AlignedSystemHeapAllocator
{
	AlignedSystemHeapAllocator(size_t allocatorAlignment=16):
		alignment(Max(allocatorAlignment, sizeof(void*)*2)) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo::Current());
	void Free(void* ptr, size_t size);
	size_t GetAlignment() const {return alignment;}

private:
	size_t alignment;
};

struct PageAllocator
{
	static AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo::Current())
	{
		(void)sourceInfo;
		size_t pageSize = VirtualMemoryPageSize();
		size_t rem = bytes % pageSize;
		if(rem>0) bytes += pageSize - rem;
		return VirtualAlloc(bytes, Access::ReadWrite);
	}
	static void Free(void* ptr, size_t size) {VirtualFree(ptr, size);}
	size_t GetAlignment() const {return VirtualMemoryPageSize();}
};
} INTRA_END
