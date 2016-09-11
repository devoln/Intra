#pragma once

#include "Core/Core.h"
#include "Data/Reflection.h"
#include "Containers/String.h"

namespace Intra {

struct SystemMemoryInfo
{
	ulong64 TotalPhysicalMemory=0, FreePhysicalMemory=0;
	ulong64 TotalVirtualMemory=0, FreeVirtualMemory=0;
	ulong64 TotalSwapMemory=0, FreeSwapMemory=0;

	static SystemMemoryInfo Get();

	INTRA_ADD_REFLECTION(SystemMemoryInfo, TotalPhysicalMemory, FreePhysicalMemory,
		TotalVirtualMemory, FreeVirtualMemory, TotalSwapMemory, FreeSwapMemory);
};

struct ProcessorInfo
{
	String BrandString;
	ushort CoreNumber=1;
	ushort LogicalProcessorNumber=1;
	ulong64 Frequency=0;

	static ProcessorInfo Get();

	INTRA_ADD_REFLECTION(ProcessorInfo, BrandString);
};

}
