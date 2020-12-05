#include "IntraX/Memory/Allocator/System.h"
#include "IntraX/Memory/VirtualMemory.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
#include <stdlib.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif

#include <Windows.h>
#endif
INTRA_WARNING_POP

INTRA_BEGIN

#ifdef _WIN32
AnyPtr SystemHeapAllocator::Allocate(size_t bytes, SourceInfo sourceInfo)
{
	(void)sourceInfo;
	return HeapAlloc(GetProcessHeap(), 0, bytes);
}

void SystemHeapAllocator::Free(void* ptr)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}
#endif


AnyPtr AlignedSystemHeapAllocator::Allocate(size_t& bytes, SourceInfo sourceInfo)
{
	(void)sourceInfo;
#ifdef __ANDROID__ 
    return memalign(alignment, bytes);
#elif defined(__unix__)
	void* result;
	if(posix_memalign(&result, alignment, bytes)==0) return result;
	return null;
#else
	return _aligned_malloc(bytes, alignment);
#endif
}

void AlignedSystemHeapAllocator::Free(void* ptr, size_t size)
{
	(void)size;
#ifdef __unix__
	free(ptr);
#else
	_aligned_free(ptr);
#endif
}

AnyPtr PageAllocator::Allocate(size_t& bytes, SourceInfo sourceInfo)
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
INTRA_END
