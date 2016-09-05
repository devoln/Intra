#include "Platform/HardwareInfo.h"


#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace Intra {

SystemMemoryInfo SystemMemoryInfo::Get()
{
	SystemMemoryInfo result;
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);
	GlobalMemoryStatusEx(&statex);
	result.TotalPhysicalMemory = statex.ullTotalPhys;
	result.FreePhysicalMemory = statex.ullAvailPhys;
	result.TotalSwapMemory = statex.ullTotalPageFile;
	result.FreeSwapMemory = statex.ullAvailPageFile;
	result.TotalVirtualMemory = statex.ullTotalVirtual;
	result.FreeVirtualMemory = statex.ullAvailVirtual;
	return result;
}

}

#else

namespace Intra {

SystemMemoryInfo SystemMemoryInfo::Get()
{
	return {0}
}

}


#endif

#if((INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64) && !defined(__clang__) && defined(_MSC_VER))
#include <intrin.h>

namespace Intra {

ProcessorInfo ProcessorInfo::Get()
{
	int cpuInfo[12] = {-1};
	__cpuid(cpuInfo, 0x80000002u);
	__cpuid(cpuInfo+4, 0x80000003u);
	__cpuid(cpuInfo+8, 0x80000004u);
	ProcessorInfo result;
	result.BrandString = String((const char*)cpuInfo);
	return result;
}

}

#else

namespace Intra {

ProcessorInfo ProcessorInfo::Get() {return {};}

}

#endif
