#include "Platform/HardwareInfo.h"
#include "Platform/CppWarnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#pragma comment(lib, "Advapi32.lib")
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


ProcessorInfo ProcessorInfo::Get()
{
	ProcessorInfo result;
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	result.LogicalProcessorNumber = ushort(sysInfo.dwNumberOfProcessors);
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
		DWORD dwMHz;
		DWORD size = sizeof(dwMHz);
		lError = RegQueryValueExA(hKey, "~MHz", null, null, reinterpret_cast<LPBYTE>(&dwMHz), &size);
		if(lError==ERROR_SUCCESS) result.Frequency = dwMHz*1000000ull;

		char processorName[64] = {0};
		size = sizeof(processorName);
		lError = RegQueryValueExA(hKey, "ProcessorNameString", null, null, reinterpret_cast<LPBYTE>(processorName), &size);
		if(lError==ERROR_SUCCESS) result.BrandString = StringView(processorName, size).TrimRight('\0');
	}

	return result;
}

}

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Linux)

#include "IO/File.h"
#include "Algo/String/Parse.h"
#include "Algo/Search/Single.h"
#include "Algo/Search/Subrange.h"
#include "Range/Construction/TakeUntil.h"

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

ProcessorInfo ProcessorInfo::Get()
{
	ProcessorInfo result;

	String allCpuInfo = IO::DiskFile::ReadAsString("/proc/cpuinfo");

	StringView modelNameLine = Algo::Find(allCpuInfo(), StringView("\nmodel name"));
	result.BrandString = Range::TakeUntil(Algo::Find(modelNameLine, ':').Drop(2), '\n');

	StringView cpuCoresLine = Algo::Find(allCpuInfo(), StringView("\ncpu cores"));
	result.CoreNumber = Algo::ParseAdvance<ushort>(Range::TakeUntil(Algo::Find(cpuCoresLine, ':').Drop(2), '\n'));

	result.LogicalProcessorNumber = ushort(Algo::Count(allCpuInfo(), StringView("\nprocessor")));
	if(allCpuInfo().StartsWith(StringView("processor"))) result.LogicalProcessorNumber++;

	if(result.CoreNumber==0) result.CoreNumber = result.LogicalProcessorNumber;

	StringView cpuMHzLine = Algo::Find(allCpuInfo(), StringView("\ncpu MHz"));
	StringView cpuMHzStr = Range::TakeUntil(Algo::Find(cpuMHzLine, ':').Drop(2), '\n');
	result.Frequency = ulong64(1000000*Algo::ParseAdvance<double>(cpuMHzStr));

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

ProcessorInfo ProcessorInfo::Get()
{
	int mib[2] = {CTL_HW, HW_NCPU};
	int numCPU;
	size_t len = sizeof(numCPU);
	sysctl(mib, 2, &numCPU, &len, null, 0);
	if(numCPU<1) numCPU = 1;

	int freq;
	len = sizeof(freq);
	sysctlbyname("hw.clockrate", &freq, &len, null, 0);

	char brandString[64]={0};
	mib[1] = HW_MODEL;
	len=63;
	sysctl(mib, 2, brandString, &len, null, 0);


	ProcessorInfo result;
	result.BrandString = String(brandString, len-1);
	result.LogicalProcessorNumber = ushort(numCPU);
	result.CoreNumber = result.LogicalProcessorNumber; //TODO: разобраться, что из этого логические процессоры, а что - ядра, и исправить
	result.Frequency = ulong64(freq)*1000000;
	return result;
}

}

#else

namespace Intra {

SystemMemoryInfo SystemMemoryInfo::Get()
{
	SystemMemoryInfo result;
	return result;
}

ProcessorInfo ProcessorInfo::Get()
{
	ProcessorInfo result;
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
	result.BrandString = "Emscripten";
#endif
	return result;
}

}

#endif

INTRA_WARNING_POP
