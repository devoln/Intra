#pragma once

#include "Container/Sequential/String.h"

INTRA_BEGIN
struct ProcessorInfo
{
	String BrandString;
	ushort CoreNumber = 1;
	ushort LogicalProcessorNumber = 1;
	uint64 Frequency = 0;

	static ProcessorInfo Get();
};
INTRA_END
