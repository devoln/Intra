#pragma once

#include "IntraX/Container/Sequential/String.h"

namespace Intra { INTRA_BEGIN
struct ProcessorInfo
{
	String BrandString;
	uint16 CoreNumber = 1;
	uint16 LogicalProcessorNumber = 1;
	uint64 Frequency = 0;

	static ProcessorInfo Get();
};
} INTRA_END
