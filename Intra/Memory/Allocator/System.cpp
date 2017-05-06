#include "Memory/Allocator/System.h"
#include "Memory/VirtualMemory.h"
#include "Cpp/Warnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include <stdlib.h>

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif

#include <Windows.h>

#endif

namespace Intra { namespace Memory {

AnyPtr MallocAllocator::Allocate(size_t bytes, const SourceInfo& sourceInfo)
{
	(void)sourceInfo;
	return malloc(bytes);
}

void MallocAllocator::Free(void* ptr) {free(ptr);}

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

AnyPtr SystemHeapAllocator::Allocate(size_t bytes, const SourceInfo& sourceInfo)
{
	(void)sourceInfo;
	return HeapAlloc(GetProcessHeap(), 0, bytes);
}

void SystemHeapAllocator::Free(void* ptr)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}

#endif


AnyPtr AlignedSystemHeapAllocator::Allocate(size_t& bytes, const SourceInfo& sourceInfo)
{
	(void)sourceInfo;
#if(defined(INTRA_PLATFORM_IS_UNIX) && INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Android)
	void* result;
	if(posix_memalign(&result, alignment, bytes)==0) return result;
	return null;
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)
    return memalign(alignment, bytes);
#else
	return _aligned_malloc(bytes, alignment);
#endif
}

void AlignedSystemHeapAllocator::Free(void* ptr, size_t size)
{
	(void)size;
#ifdef INTRA_PLATFORM_IS_UNIX
	free(ptr);
#else
	_aligned_free(ptr);
#endif
}

AnyPtr PageAllocator::Allocate(size_t& bytes, const SourceInfo& sourceInfo)
{
	(void)sourceInfo;
	size_t pageSize = VirtualMemoryPageSize();
	size_t rem = bytes % pageSize;
	if(rem>0) bytes += pageSize - rem;
	return VirtualAlloc(bytes, Access::ReadWrite);
}

void PageAllocator::Free(void* ptr, size_t size)
{
	VirtualFree(ptr, size);
}

size_t PageAllocator::GetAlignment() const {return VirtualMemoryPageSize();}

}}

INTRA_WARNING_POP
