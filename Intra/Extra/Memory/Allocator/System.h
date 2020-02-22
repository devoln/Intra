#pragma once

#include "Core/Assert.h"
#include "Memory/VirtualMemory.h"
#include "Math/Math.h"

INTRA_BEGIN
struct NewAllocator
{
	static AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{(void)sourceInfo; return operator new(bytes);}
	
	static void Free(void* ptr, size_t size) {(void)size; operator delete(ptr);}
	
	static size_t GetAlignment() {return sizeof(void*)*2;}
};

namespace C {
extern "C" {
	void* INTRA_CRTDECL malloc(size_t bytes) noexcept;
	void* INTRA_CRTDECL realloc(void* oldPtr, size_t bytes) noexcept;
	void INTRA_CRTDECL free(void* ptr) noexcept;
}
}

struct MallocAllocator
{
	static AnyPtr Allocate(size_t bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{
		(void)sourceInfo;
		return C::malloc(bytes);
	}
	static void Free(void* ptr, size_t size) {(void)size; Free(ptr);}
	static void Free(void* ptr) {C::free(ptr);}
	static size_t GetAlignment() {return sizeof(void*)*2;}
};

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
struct SystemHeapAllocator
{
	static AnyPtr Allocate(size_t bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO);
	static void Free(void* ptr, size_t size) {(void)size; Free(ptr);}
	static void Free(void* ptr);
	static size_t GetAlignment() {return sizeof(void*)*2;}
};
#else
typedef MallocAllocator SystemHeapAllocator;
#endif

struct AlignedSystemHeapAllocator
{
	AlignedSystemHeapAllocator(size_t allocatorAlignment=16):
		alignment(Max(allocatorAlignment, sizeof(void*)*2)) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO);
	void Free(void* ptr, size_t size);
	size_t GetAlignment() const {return alignment;}

private:
	size_t alignment;
};

struct PageAllocator
{
	static AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO);
	static void Free(void* ptr, size_t size);
	size_t GetAlignment() const;
};
INTRA_END
