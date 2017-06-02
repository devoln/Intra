#include "System/ProcessorInfo.h"
#include "Cpp/Warnings.h"
#include "Range/Search/Trim.h"
#include "IO/OsFile.h"

#include "Range/Stream/Parse.h"
#include "Range/Search/Single.h"
#include "Range/Search/Subrange.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#pragma comment(lib, "Advapi32.lib")
#endif

#include <Windows.h>

namespace Intra { namespace System {

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
		if(lError == ERROR_SUCCESS) result.BrandString = TrimRight(StringView(processorName, size), '\0');
	}

	return result;
}

}}

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux)

#include <unistd.h>
#include <sys/sysinfo.h>

namespace Intra { namespace System {

ProcessorInfo ProcessorInfo::Get()
{
	ProcessorInfo result;

	const String allCpuInfo = IO::OsFile::ReadAsString("/proc/cpuinfo", Error::Skip());

	const StringView modelNameLine = Range::Find(allCpuInfo, "\nmodel name");
	result.BrandString = TakeUntil(modelNameLine.Find(':').Drop(2), '\n');

	const StringView cpuCoresLine = Range::Find(allCpuInfo, "\ncpu cores");
	result.CoreNumber = Range::Parse<ushort>(cpuCoresLine.Find(':').Drop(2).FindBefore('\n'));

	result.LogicalProcessorNumber = ushort(Range::Count(allCpuInfo, "\nprocessor"));
	if(Range::StartsWith(allCpuInfo, "processor")) result.LogicalProcessorNumber++;

	if(result.CoreNumber == 0) result.CoreNumber = result.LogicalProcessorNumber;

	const StringView cpuMHzLine = Range::Find(allCpuInfo, "\ncpu MHz");
	const StringView cpuMHzStr = cpuMHzLine.Find(':').Drop(2).FindBefore('\n');
	result.Frequency = ulong64(1000000 * Range::Parse<double>(cpuMHzStr));

	return result;
}

}}

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_FreeBSD)

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/limits.h>

namespace Intra { namespace System {

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

}}

#else

namespace Intra { namespace System {

ProcessorInfo ProcessorInfo::Get()
{
	ProcessorInfo result;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
	result.BrandString = "Emscripten";
#endif
	return result;
}

}}

#endif

INTRA_WARNING_POP
