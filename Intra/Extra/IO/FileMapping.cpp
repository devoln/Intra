#include "FileMapping.h"
#include "FileSystem.h"

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
#endif

#else

#endif
INTRA_WARNING_POP

INTRA_BEGIN
BasicFileMapping::BasicFileMapping(StringView fileName,
	uint64 startByte, size_t bytes, bool writeAccess, ErrorReporter err)
{
	String fullFileName = OS.GetFullFileName(fileName);
	auto size = OS.FileGetSize(fullFileName, err);
	if(startByte > size || size == 0) return;

	if(bytes == ~size_t(0) && bytes > size - startByte)
		bytes = size_t(size - startByte);

	if(startByte + bytes > size) return;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
	void* data = GlobalHeap.Allocate(bytes, INTRA_SOURCE_INFO);
	INTRA_ASSERT(startByte == 0);
	auto file = OS.FileOpen(fullFileName, status);
	file.RawReadTo(data, bytes);
	file = null;
	(void)writeAccess;
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

	const DWORD fileDesiredAccess = GENERIC_READ|(writeAccess? GENERIC_WRITE: 0);
	const DWORD creationDisposition = DWORD(writeAccess? OPEN_ALWAYS: OPEN_EXISTING);
	const auto wFullFileName = detail::Utf8ToWStringZ(fullFileName);

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
		detail::ProcessLastError(err, "Cannot open file " + fileName + " for mapping: ", INTRA_SOURCE_INFO);
		return;
	}
	INTRA_FINALLY{CloseHandle(hFile);};

	const DWORD flProtect = DWORD(writeAccess? PAGE_READWRITE: PAGE_READONLY);

#ifndef WINSTORE_APP
	const DWORD lowSize = DWORD(bytes + startByte);
	const DWORD highSize = DWORD(uint64(bytes + startByte) >> 32);
	const HANDLE fileMapping = CreateFileMappingW(hFile, null, flProtect, highSize, lowSize, null);
#else
	const HANDLE fileMapping = CreateFileMappingFromApp(hFile, null, flProtect, bytes, null);
#endif
	INTRA_FINALLY{CloseHandle(fileMapping);};

	if(fileMapping == null)
	{
		detail::ProcessLastError(err, "Cannot CreateFileMapping " + fileName + ": ", INTRA_SOURCE_INFO);
		return;
	}

	const DWORD mapDesiredAccess = DWORD(writeAccess? FILE_MAP_WRITE: FILE_MAP_READ);
#ifndef WINSTORE_APP
	const DWORD lowOffset = DWORD(startByte);
	const DWORD highOffset = DWORD(startByte >> 32);
	void* data = MapViewOfFile(fileMapping, mapDesiredAccess, highOffset, lowOffset, bytes);
#else
	void* data = MapViewOfFileFromApp(fileMapping, mapDesiredAccess, startByte);
#endif
	if(!data)
	{
		detail::ProcessLastError(err, "Cannot MapViewOfFile " + fileName + ": ", INTRA_SOURCE_INFO);
		return;
	}
#else
	int fd = open(fullFileName.CStr(), writeAccess? O_RDONLY: O_RDWR);
	if(fd <= 0)
	{
		detail::ProcessLastError(err, "Cannot open file " + fileName + " for mapping: ", INTRA_SOURCE_INFO);
		return;
	}
	INTRA_FINALLY_CALL(close, fd);

	void* data = mmap(null, bytes,
		writeAccess? PROT_WRITE: PROT_READ,
		MAP_SHARED, fd, long(startByte));

	if(data == MAP_FAILED || data == null)
	{
		detail::ProcessLastError(err, "Cannot mmap " + fileName + ": ", INTRA_SOURCE_INFO);
		return;
	}
#endif
	mData = SpanOfRaw(data, bytes);
#ifdef INTRA_DEBUG
	mFilePath = Move(fullFileName);
#endif
}

void BasicFileMapping::Close()
{
	if(mData == null) return;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
	GlobalHeap.Free(mData.Data(), mData.Length());
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	UnmapViewOfFile(mData.Data());
#else
	munmap(mData.Data(), mData.Length());
#endif
	mData = null;
#ifdef INTRA_DEBUG
	mFilePath = null;
#endif
}

void WritableFileMapping::Flush()
{
	if(mData == null) return;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	FlushViewOfFile(mData.Data(), 0);
#else
	msync(mData.Data(), mData.Length(), MS_SYNC);
#endif
}
INTRA_END
