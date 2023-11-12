#include "FileMapping.h"
#include "FileSystem.h"

#include "IntraX/System/detail/Common.h"

#include "IntraX/Memory/Allocator/Global.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
#ifdef __unix__

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#elif defined(_WIN32)

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

namespace Intra { INTRA_BEGIN
BasicFileMapping::BasicFileMapping(StringView fileName,
	uint64 startByte, size_t bytes, bool writeAccess, ErrorReporter err)
{
	String fullFileName = OS.GetFullFileName(fileName);
	auto size = OS.FileGetSize(fullFileName, err);
	if(startByte > size || size == 0) return;

	if(bytes == ~size_t(0) && bytes > size - startByte)
		bytes = size_t(size - startByte);

	if(startByte + bytes > size) return;
#ifdef __EMSCRIPTEN__
	void* data = GlobalHeap.Allocate(bytes, INTRA_SOURCE_INFO);
	INTRA_ASSERT(startByte == 0);
	auto file = OS.FileOpen(fullFileName, status);
	file.RawReadTo(data, bytes);
	file = nullptr;
	(void)writeAccess;
#elif defined(_WIN32)

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
		fileDesiredAccess, FILE_SHARE_READ, nullptr, creationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
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
	const HANDLE fileMapping = CreateFileMappingW(hFile, nullptr, flProtect, highSize, lowSize, nullptr);
#else
	const HANDLE fileMapping = CreateFileMappingFromApp(hFile, nullptr, flProtect, bytes, nullptr);
#endif
	INTRA_FINALLY{CloseHandle(fileMapping);};

	if(fileMapping == nullptr)
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
	int fd = open(fullFileName.CStr(), writeAccess? O_RDWR: O_RDONLY);
	if(fd <= 0)
	{
		detail::ProcessLastError(err, "Cannot open file " + fileName + " for mapping: ", INTRA_SOURCE_INFO);
		return;
	}
	INTRA_FINALLY_CALL(close, fd);

	void* data = mmap(nullptr, bytes,
		writeAccess? PROT_WRITE: PROT_READ,
		MAP_SHARED, fd, long(startByte));

	if(data == MAP_FAILED || data == nullptr)
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
	if(mData == nullptr) return;
#ifdef __EMSCRIPTEN__
	GlobalHeap.Free(mData.Data(), mData.Length());
#elif defined(_WIN32)
	UnmapViewOfFile(mData.Data());
#else
	munmap(mData.Data(), mData.Length());
#endif
	mData = nullptr;
	if constexpr(DebugCheckLevel >= 1)
		mFilePath = nullptr;
}

void WritableFileMapping::Flush()
{
	if(mData == nullptr) return;
#ifdef __EMSCRIPTEN__
#elif defined(_WIN32)
	FlushViewOfFile(mData.Data(), 0);
#else
	msync(mData.Data(), mData.Length(), MS_SYNC);
#endif
}
} INTRA_END
