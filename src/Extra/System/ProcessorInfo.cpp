#include "Extra/System/ProcessorInfo.h"

#include "Intra/Range/Search/Trim.h"
#include "Extra/IO/OsFile.h"

#include "Intra/Range/Stream/Parse.h"
#include "Intra/Range/Search/Single.h"
#include "Intra/Range/Search/Subrange.h"


#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

INTRA_PUSH_DISABLE_ALL_WARNINGS
#ifdef _MSC_VER
#pragma comment(lib, "Advapi32.lib")
#endif
#include <Windows.h>
INTRA_WARNING_POP

EXTRA_BEGIN
ProcessorInfo ProcessorInfo::Get()
{
	ProcessorInfo result;
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	result.LogicalProcessorNumber = uint16(sysInfo.dwNumberOfProcessors);
//#ifndef EXTRA_DROP_XP_SUPPORT
	result.CoreNumber = result.LogicalProcessorNumber;
/*#else
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalProcInfoEx[16];
	DWORD bufferLength = 16*sizeof(logicalProcInfoEx);
	GetLogicalProcessorInformationEx(RelationProcessorPackage, logicalProcInfoEx, &bufferLength);
	result.CoreNumber = uint16(bufferLength/sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX));
#endif*/
	
	HKEY hKey;
	long lError = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
		"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey);
    
	if(lError == ERROR_SUCCESS)
	{
		DWORD dwMHz;
		DWORD size = sizeof(dwMHz);
		lError = RegQueryValueExA(hKey, "~MHz", null, null, reinterpret_cast<LPBYTE>(&dwMHz), &size);
		if(lError == ERROR_SUCCESS) result.Frequency = dwMHz*1000000ull;

		char processorName[64] = {0};
		size = sizeof(processorName);
		lError = RegQueryValueExA(hKey, "ProcessorNameString", null, null, reinterpret_cast<LPBYTE>(processorName), &size);
		if(lError == ERROR_SUCCESS) result.BrandString = TrimRight(SpanOfPtr(processorName, index_t(size)), '\0');
	}

	return result;
}
EXTRA_END

#elif defined(__linux__)

#include <unistd.h>
#include <sys/sysinfo.h>

EXTRA_BEGIN
ProcessorInfo ProcessorInfo::Get()
{
	ProcessorInfo result;

	const String allCpuInfo = IO::OsFile::ReadAsString("/proc/cpuinfo", Error::Skip());

	const StringView modelNameLine = Find(allCpuInfo, "\nmodel name");
	result.BrandString = TakeUntil(modelNameLine.Find(':').Drop(2), '\n');

	const StringView cpuCoresLine = Find(allCpuInfo, "\ncpu cores");
	result.CoreNumber = Parse<uint16>(cpuCoresLine.Find(':').Drop(2).FindBefore('\n'));

	result.LogicalProcessorNumber = uint16(Count(allCpuInfo, "\nprocessor"));
	if(StartsWith(allCpuInfo, "processor")) result.LogicalProcessorNumber++;

	if(result.CoreNumber == 0) result.CoreNumber = result.LogicalProcessorNumber;

	const StringView cpuMHzLine = Find(allCpuInfo, "\ncpu MHz");
	const StringView cpuMHzStr = cpuMHzLine.Find(':').Drop(2).FindBefore('\n');
	result.Frequency = uint64(1000000 * Parse<double>(cpuMHzStr));

	return result;
}
EXTRA_END

#elif defined(__FreeBSD__)

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/limits.h>

EXTRA_BEGIN
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
	result.LogicalProcessorNumber = uint16(numCPU);
	result.CoreNumber = result.LogicalProcessorNumber; //TODO: разобраться, что из этого логические процессоры, а что - ядра, и исправить
	result.Frequency = uint64(freq)*1000000;
	return result;
}
EXTRA_END

#else

EXTRA_BEGIN
ProcessorInfo ProcessorInfo::Get()
{
	ProcessorInfo result;
#ifdef __EMSCRIPTEN__
	result.BrandString = "Emscripten";
#endif
	return result;
}
EXTRA_END

#endif
