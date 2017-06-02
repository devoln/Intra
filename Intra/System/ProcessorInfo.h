#pragma once

#include "Cpp/Warnings.h"
#include "Container/Sequential/String.h"

namespace Intra { namespace System {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct ProcessorInfo
{
	String BrandString;
	ushort CoreNumber = 1;
	ushort LogicalProcessorNumber = 1;
	ulong64 Frequency = 0;

	static ProcessorInfo Get();
};

INTRA_WARNING_POP

}}
