#include "IntraX/System/RamInfo.h"

#include "Intra/Range/Search/Trim.h"
#include "IntraX/IO/OsFile.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma comment(lib, "Advapi32.lib")
#endif

INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <Windows.h>
INTRA_WARNING_POP

INTRA_BEGIN
RamInfo RamInfo::Get()
{
	RamInfo result;
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
INTRA_END

#elif defined(__linux__)

#include <sys/sysinfo.h>

INTRA_BEGIN
RamInfo RamInfo::Get()
{
	RamInfo result;
/*#if defined(__linux__) || defined(__FreeBSD__)
    result.TotalPhysicalMemory = uint64(sysconf(_SC_PHYS_PAGES))*sysconf(_SC_PAGE_SIZE);
#elif defined(__APPLE__)
	uint64 mem=0;
	size_t len = sizeof(mem);
	sysctlbyname("hw.memsize", &result.TotalPhysicalMemory, &len, null, 0);
#endif*/

	struct sysinfo info;
	sysinfo(&info);

	result.TotalPhysicalMemory = info.totalram;
	result.FreePhysicalMemory = info.freeram;
	result.TotalSwapMemory = info.totalswap;
	result.FreeSwapMemory = info.freeswap;
	result.TotalVirtualMemory = result.TotalPhysicalMemory + result.TotalSwapMemory;
	result.FreeVirtualMemory = result.FreePhysicalMemory + result.FreeSwapMemory;

	return result;
}
INTRA_END

#elif defined(__FreeBSD__)

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

INTRA_BEGIN
RamInfo RamInfo::Get()
{
	RamInfo result;
		
	vmtotal vmsize = getVMinfo();
	unsigned pageSize = unsigned(getSysCtl(CTL_HW, HW_PAGESIZE));

	unsigned activePages, wirePages;
	unsigned cachePages, inactivePages, freePages;
	size_t len = sizeof(unsigned);
	sysctlbyname("vm.stats.vm.v_active_count", &activePages, &len, null, 0);
	sysctlbyname("vm.stats.vm.v_wire_count", &wirePages, &len, null, 0);
	sysctlbyname("vm.stats.vm.v_cache_count", &cachePages, &len, null, 0);
	sysctlbyname("vm.stats.vm.v_inactive_count", &inactivePages, &len, null, 0);
	sysctlbyname("vm.stats.vm.v_free_count", &freePages, &len, null, 0);

	result.TotalPhysicalMemory = getSysCtl(CTL_HW, HW_PHYSMEM);
	result.FreePhysicalMemory = result.TotalPhysicalMemory-uint64(activePages+wirePages)*pageSize;
	result.TotalVirtualMemory = uint64(vmsize.t_vm)*pageSize;
	result.FreeVirtualMemory = uint64(vmsize.t_free)*pageSize;
	result.TotalSwapMemory = result.TotalVirtualMemory-result.TotalPhysicalMemory;
	result.FreeSwapMemory = result.FreeVirtualMemory-result.FreePhysicalMemory;

	return result;
}
INTRA_END

#else

INTRA_BEGIN
RamInfo RamInfo::Get()
{
	RamInfo result;
	return result;
}
INTRA_END

#endif
