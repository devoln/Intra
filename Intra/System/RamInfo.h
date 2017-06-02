#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"

namespace Intra { namespace System {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct RamInfo
{
	ulong64 TotalPhysicalMemory = 0;
	ulong64 FreePhysicalMemory = 0;
	ulong64 TotalVirtualMemory = 0;
	ulong64 FreeVirtualMemory = 0;
	ulong64 TotalSwapMemory = 0;
	ulong64 FreeSwapMemory = 0;

	static RamInfo Get();
};

INTRA_WARNING_POP

}}
