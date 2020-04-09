#pragma once

#include "Extra/Container/Sequential/String.h"

INTRA_BEGIN
struct ProcessorInfo
{
	String BrandString;
	uint16 CoreNumber = 1;
	uint16 LogicalProcessorNumber = 1;
	uint64 Frequency = 0;

	static ProcessorInfo Get();
};
INTRA_END
