#pragma once

#include "Core/Core.h"
#include "Data/Reflection.h"
#include "Containers/String.h"

namespace Intra {

struct SystemMemoryInfo
{
	ulong64 TotalPhysicalMemory, FreePhysicalMemory;
	ulong64 TotalVirtualMemory, FreeVirtualMemory;
	ulong64 TotalSwapMemory, FreeSwapMemory;

	static SystemMemoryInfo Get();

	INTRA_ADD_REFLECTION(SystemMemoryInfo, TotalPhysicalMemory, FreePhysicalMemory,
		TotalVirtualMemory, FreeVirtualMemory, TotalSwapMemory, FreeSwapMemory);
};

struct ProcessorInfo
{
	String BrandString;

	static ProcessorInfo Get();

	INTRA_ADD_REFLECTION(ProcessorInfo, BrandString);
};

}
