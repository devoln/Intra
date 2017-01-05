#include "Core/Core.h"
#include "Memory/Memory.h"
#include "Core/Errors.h"
#include "IO/Stream.h"
#include "Threading/Atomic.h"

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
