#include "FileMapping.h"
#include "FileSystem.h"

#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"

#include "Utils/Finally.h"

#include "System/detail/Common.h"

#include "Memory/Allocator/Global.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(defined(INTRA_PLATFORM_IS_UNIX) || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Android || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#elif INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <io.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
struct IUnknown;
#include <Windows.h>
#include <Shlwapi.h>
#undef GetCurrentDirectory

#ifdef _MSC_VER
#pragma comment(lib, "Shlwapi.lib")
#pragma warning(pop)
#endif

#else

#endif

namespace Intra { namespace IO {

BasicFileMapping::BasicFileMapping(StringView fileName, ulong64 startByte, size_t bytes, bool writeAccess, ErrorStatus& status):
	mData(null), mSize(0)
{
	if(status.WasError()) return;
	String fullFileName = OS.GetFullFileName(fileName);
	auto size = OS.FileGetSize(fullFileName, status);
	if(startByte > size || size == 0) return;

	if(bytes == ~size_t(0) && bytes > size - startByte)
		bytes = size_t(size - startByte);

	if(startByte + bytes > size) return;
	mSize = bytes;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
	mData = Memory::GlobalHeap.Allocate(mSize, INTRA_SOURCE_INFO);
	OS.FileOpen(fullFileName).ReadData(startByte, mData, mSize);
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

	const DWORD fileDesiredAccess = GENERIC_READ|(writeAccess? GENERIC_WRITE: 0);
	const DWORD creationDisposition = DWORD(writeAccess? OPEN_ALWAYS: OPEN_EXISTING);
	const auto wFullFileName = System::detail::Utf8ToWStringZ(fullFileName);

#if defined(WINAPI_FAMILY_PARTITION) && defined(WINAPI_PARTITION_DESKTOP)
#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) && _WIN32_WINNT >= 0x0602
#define WINSTORE_APP
	CREATEFILE2_EXTENDED_PARAMETERS params{};
	params.dwSize = sizeof(params);
	params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	const HANDLE hFile = CreateFile2(wFullFileName.Data(), fileDesiredAccess,
		FILE_SHARE_READ, creationDisposition, &params);
#endif
#endif

#ifndef WINSTORE_APP
	const HANDLE hFile = CreateFileW(wFullFileName.Data(),
		fileDesiredAccess, FILE_SHARE_READ, null, creationDisposition, FILE_ATTRIBUTE_NORMAL, null);
#endif

	if(hFile == INVALID_HANDLE_VALUE)
	{
		System::detail::ProcessLastError(status, "Cannot open file " + fileName + " for mapping: ", INTRA_SOURCE_INFO);
		mSize = 0;
		return;
	}
	INTRA_FINALLY_CALL(CloseHandle, hFile);

	const DWORD flProtect = DWORD(writeAccess? PAGE_READWRITE: PAGE_READONLY);

#ifndef WINSTORE_APP
	const DWORD lowSize = DWORD(mSize + startByte);
	const DWORD highSize = DWORD(ulong64(mSize + startByte) >> 32);
	const HANDLE fileMapping = CreateFileMappingW(hFile, null, flProtect, highSize, lowSize, null);
#else
	const HANDLE fileMapping = CreateFileMappingFromApp(hFile, null, flProtect, mSize, null);
#endif

	if(fileMapping == null)
	{
		System::detail::ProcessLastError(status, "Cannot CreateFileMapping " + fileName + ": ", INTRA_SOURCE_INFO);
		mSize = 0;
		return;
	}

	const DWORD mapDesiredAccess = DWORD(writeAccess? FILE_MAP_WRITE: FILE_MAP_READ);
#ifndef WINSTORE_APP
	const DWORD lowOffset = DWORD(startByte);
	const DWORD highOffset = DWORD(startByte >> 32);
	mData = MapViewOfFile(fileMapping, mapDesiredAccess, highOffset, lowOffset, mSize);
#else
	mData = MapViewOfFileFromApp(fileMapping, mapDesiredAccess, startByte);
#endif

	CloseHandle(fileMapping);
#else
	int fd = open(fullFileName.CStr(), writeAccess? O_RDONLY: O_RDWR);
	if(fd <= 0)
	{
		System::detail::ProcessLastError(status, "Cannot open file " + fileName + " for mapping: ", INTRA_SOURCE_INFO);
		mSize = 0;
		return;
	}
	INTRA_FINALLY_CALL(close, fd);

	mData = mmap(null, bytes,
		writeAccess? PROT_WRITE: PROT_READ,
		MAP_SHARED, fd, long(startByte));

	if(mData == MAP_FAILED || mData == null)
	{
		mData = null;
		mSize = 0;
		System::detail::ProcessLastError(status, "Cannot mmap " + fileName + ": ", INTRA_SOURCE_INFO);
	}
#endif
}

void BasicFileMapping::Close()
{
	if(mData == null) return;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
	Memory::GlobalHeap.Free(mData, mSize);
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	UnmapViewOfFile(mData);
#else
	munmap(mData, mSize);
#endif
	mData = null;
	mSize = 0;
}

void WritableFileMapping::Flush()
{
	if(mData == null) return;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	FlushViewOfFile(mData, 0);
#else
	msync(mData, mSize, MS_SYNC);
#endif
}


}}

INTRA_WARNING_POP
