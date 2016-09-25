#include "Platform/HardwareInfo.h"


#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <Windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

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

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Linux)

#include <unistd.h>
#include <sys/sysinfo.h>

namespace Intra {

SystemMemoryInfo SystemMemoryInfo::Get()
{
	SystemMemoryInfo result;
/*#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Linux || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_FreeBSD)
    result.TotalPhysicalMemory = ulong64(sysconf(_SC_PHYS_PAGES))*sysconf(_SC_PAGE_SIZE);
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_MacOS)
	ulong64 mem=0;
	size_t len = sizeof(mem);
	sysctlbyname("hw.memsize", &result.TotalPhysicalMemory, &len, null, 0);
#endif*/

	struct sysinfo info;
	sysinfo(&info);

	result.TotalPhysicalMemory = info.totalram;
	result.FreePhysicalMemory = info.freeram;
	result.TotalSwapMemory = info.totalswap;
	result.FreeSwapMemory = info.freeswap;
	result.TotalVirtualMemory = result.TotalPhysicalMemory+result.TotalSwapMemory;
	result.FreeVirtualMemory = result.FreePhysicalMemory+result.FreeSwapMemory;

	return result;
}

}

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_FreeBSD)

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <sys/limits.h>
#include <vm/vm_param.h>

static vmtotal getVMinfo()
{
  vmtotal vm_info;
  int mib[2] = {CTL_VM, VM_TOTAL};
  size_t len = sizeof(vm_info);
  sysctl(mib, 2, &vm_info, &len, nullptr, 0);
  return vm_info;
}

static unsigned long long getSysCtl(int top_level, int next_level)
{
	int mib[2] = {top_level, next_level};
	unsigned long long ctlvalue;
	size_t len = sizeof(ctlvalue);
	sysctl(mib, 2, &ctlvalue, &len, nullptr, 0);	
	return ctlvalue;
}

namespace Intra {

SystemMemoryInfo SystemMemoryInfo::Get()
{
	SystemMemoryInfo result;
		
	vmtotal vmsize = getVMinfo();
	uint pageSize = uint(getSysCtl(CTL_HW, HW_PAGESIZE));

	uint activePages, wirePages;
	uint cachePages, inactivePages, freePages;
	size_t len = sizeof(uint);
	sysctlbyname("vm.stats.vm.v_active_count", &activePages, &len, null, 0);
	sysctlbyname("vm.stats.vm.v_wire_count", &wirePages, &len, null, 0);
	sysctlbyname("vm.stats.vm.v_cache_count", &cachePages, &len, null, 0);
	sysctlbyname("vm.stats.vm.v_inactive_count", &inactivePages, &len, null, 0);
	sysctlbyname("vm.stats.vm.v_free_count", &freePages, &len, null, 0);

	result.TotalPhysicalMemory = getSysCtl(CTL_HW, HW_PHYSMEM);
	result.FreePhysicalMemory = result.TotalPhysicalMemory-ulong64(activePages+wirePages)*pageSize;
	result.TotalVirtualMemory = ulong64(vmsize.t_vm)*pageSize;
	result.FreeVirtualMemory = ulong64(vmsize.t_free)*pageSize;
	result.TotalSwapMemory = result.TotalVirtualMemory-result.TotalPhysicalMemory;
	result.FreeSwapMemory = result.FreeVirtualMemory-result.FreePhysicalMemory;

	return result;
}

}

#endif

#if((INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64) && !defined(__clang__) && defined(_MSC_VER))
#include <intrin.h>
#pragma comment(lib, "Advapi32.lib")

namespace Intra {

ProcessorInfo ProcessorInfo::Get()
{
	int cpuInfo[12] = {-1};
	__cpuid(cpuInfo, 0x80000002u);
	__cpuid(cpuInfo+4, 0x80000003u);
	__cpuid(cpuInfo+8, 0x80000004u);
	ProcessorInfo result;
	result.BrandString = String((const char*)cpuInfo);

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	result.LogicalProcessorNumber = (ushort)sysInfo.dwNumberOfProcessors;
//#ifdef INTRA_XP_SUPPORT
	result.CoreNumber = result.LogicalProcessorNumber;
/*#else
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalProcInfoEx[16];
	DWORD bufferLength = 16*sizeof(logicalProcInfoEx);
	GetLogicalProcessorInformationEx(RelationProcessorPackage, logicalProcInfoEx, &bufferLength);
	result.CoreNumber = ushort(bufferLength/sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX));
#endif*/
	
	HKEY hKey;
	long lError = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey);
    
	if(lError==ERROR_SUCCESS)
	{
		DWORD dwMHz, size=sizeof(dwMHz);
		lError = RegQueryValueExA(hKey, "~MHz", null, null, (LPBYTE)&dwMHz, &size);
		if(lError==ERROR_SUCCESS) result.Frequency = dwMHz*1000000ull;
	}

	return result;
}

}

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Linux)

#include <IO/File.h>

namespace Intra {

ProcessorInfo ProcessorInfo::Get()
{
	ProcessorInfo result;

	String allCpuInfo = IO::DiskFile::ReadAsString("/proc/cpuinfo");

	result.BrandString = allCpuInfo().Find(StringView("\nmodel name"))
		.Find(':').Drop(2).ReadUntil('\n');

	result.CoreNumber = allCpuInfo().Find(StringView("\ncpu cores"))
		.Find(':').Drop(2).ReadUntil('\n').ParseAdvance<ushort>();

	result.LogicalProcessorNumber = ushort(allCpuInfo().Count(StringView("\nprocessor")));
	if(allCpuInfo().StartsWith(StringView("processor"))) result.LogicalProcessorNumber++;

	result.Frequency = ulong64(1000000*allCpuInfo().Find(StringView("\ncpu MHz"))
		.Find(':').Drop(2).ReadUntil('\n').ParseAdvance<double>());

	return result;
}

}

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_FreeBSD)

#include <sys/sysctl.h>

namespace Intra {

ProcessorInfo ProcessorInfo::Get()
{
	int mib[2] = {CTL_HW, HW_NCPU};
	int numCPU;
	size_t len = sizeof(numCPU);
	sysctl(mib, 2, &numCPU, &len, null, 0);
	if(numCPU<1) numCPU=1;

	char brandString[64]={0};
	mib[1] = HW_MODEL;
	len=63;
	sysctl(mib, 2, brandString, &len, null, 0);

	ProcessorInfo result;
	result.BrandString = String(brandString, len);
	result.LogicalProcessorNumber = ushort(numCPU);
	result.CoreNumber = result.LogicalProcessorNumber; //TODO: разобраться, что из этого логические процессоры, а что - ядра, и исправить
	return result;
}

}
#endif
