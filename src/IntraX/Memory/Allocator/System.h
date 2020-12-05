#pragma once

#include "Intra/Assert.h"
#include "IntraX/Memory/VirtualMemory.h"
#include "Intra/Math/Math.h"

INTRA_BEGIN
struct NewAllocator
{
	static AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{(void)sourceInfo; return operator new(bytes);}
	
	static void Free(void* ptr, size_t size) {(void)size; operator delete(ptr);}
	
	static size_t GetAlignment() {return sizeof(void*)*2;}
};

namespace z_D {
extern "C" {
	INTRA_CRTIMP INTRA_CRTRESTRICT void* INTRA_CRTDECL malloc(size_t bytes);
	INTRA_CRTIMP INTRA_CRTRESTRICT void* INTRA_CRTDECL realloc(void* oldPtr, size_t bytes);
	INTRA_CRTIMP void INTRA_CRTDECL free(void* ptr);
}
}

struct MallocAllocator
{
	static AnyPtr Allocate(size_t bytes, SourceInfo sourceInfo = SourceInfo())
	{
		(void)sourceInfo;
		return z_D::malloc(bytes);
	}
	static void Free(void* ptr, size_t size) {(void)size; Free(ptr);}
	static void Free(void* ptr) {z_D::free(ptr);}
	static size_t GetAlignment() {return sizeof(void*)*2;}
};

#ifdef _WIN32
struct SystemHeapAllocator
{
	static AnyPtr Allocate(size_t bytes, SourceInfo sourceInfo = SourceInfo());
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

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo());
	void Free(void* ptr, size_t size);
	size_t GetAlignment() const {return alignment;}

private:
	size_t alignment;
};

struct PageAllocator
{
	static AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo());
	static void Free(void* ptr, size_t size);
	size_t GetAlignment() const;
};
INTRA_END
