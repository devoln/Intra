#include "Platform/CppWarnings.h"
#include "Platform/Debug.h"
#include "Memory/Memory.h"
#include "Platform/Errors.h"
#include "IO/Stream.h"
#include "Thread/Atomic.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

#include <stdlib.h>

namespace Intra {

using namespace Math;

//size_t NumAllocations=0;

namespace Memory {

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
