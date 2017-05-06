#include "Cpp/Warnings.h"
#include "Memory/Memory.h"
#include "Memory/Allocator/Global.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Memory {

AnyPtr Allocate(size_t bytes, size_t alignment)
{
	(void)alignment;
	return GlobalHeap.Allocate(bytes, INTRA_SOURCE_INFO);
}

void Free(void* data)
{
	GlobalHeap.Free(data, 0);
}

}}
