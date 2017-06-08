#include "FileMapping.h"
#include "FileSystem.h"

#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"

#include "Utils/Finally.h"

#include "Memory/Allocator/Global.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(defined(INTRA_PLATFORM_IS_UNIX) || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Android || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#elif INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows

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

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

static GenericString<wchar_t> utf8ToWStringZ_FM(StringView str)
{
	GenericString<wchar_t> wfn;
	wfn.SetLengthUninitialized(str.Length());
	int wlen = MultiByteToWideChar(CP_UTF8, 0, str.Data(),
		int(str.Length()), wfn.Data(), int(wfn.Length()));
	wfn.SetLengthUninitialized(size_t(wlen+1));
	wfn.Last() = 0;
	return wfn;
}

#endif

BasicFileMapping::BasicFileMapping(StringView fileName, ulong64 startByte, size_t bytes, bool writeAccess, ErrorStatus& status):
	mData(null), mSize(0)
{
	if(status.WasError()) return;
	String fullFileName = OS.GetFullFileName(fileName);
	auto size = OS.FileGetSize(fullFileName, status);
	if(status.WasError()) return;
	if(startByte > size) return;

	if(bytes == ~size_t(0) && bytes > size - startByte)
		bytes = size_t(size - startByte);

	if(startByte + bytes > size) return;
	mSize = bytes;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
	mData = Memory::GlobalHeap.Allocate(mSize, INTRA_SOURCE_INFO);
	OS.FileOpen(fullFileName).ReadData(startByte, mData, mSize);
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	const HANDLE hFile = CreateFileW(utf8ToWStringZ_FM(fullFileName).Data(),
		GENERIC_READ|(writeAccess? GENERIC_WRITE: 0),
		FILE_SHARE_READ, null,
		DWORD(writeAccess? OPEN_ALWAYS: OPEN_EXISTING),
		FILE_ATTRIBUTE_NORMAL, null);

	if(hFile == INVALID_HANDLE_VALUE)
	{
		char* s = null;
		FormatMessageA(DWORD(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS),
			null, GetLastError(),
			DWORD(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)),
			reinterpret_cast<char*>(&s), 0, null);
		status.Error("Cannot open file " + fileName + " for mapping: " + StringView(s) + "!", INTRA_SOURCE_INFO);
		LocalFree(s);
		return;
	}
	auto fileAutoClose = Finally(CloseHandle, hFile);

	const DWORD lowSize = DWORD(mSize+startByte);
	const DWORD highSize = DWORD(ulong64(mSize+startByte) >> 32);
	const DWORD flProtect = DWORD(writeAccess? PAGE_READWRITE: PAGE_READONLY);
	const HANDLE fileMapping = CreateFileMappingW(hFile, null, flProtect, highSize, lowSize, null);

	if(fileMapping == null)
	{
		char* s = null;
		FormatMessageA(DWORD(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS),
			null, GetLastError(),
			DWORD(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)),
			reinterpret_cast<char*>(&s), 0, null);
		status.Error("Cannot CreateFileMapping " + fileName + ": " + StringView(s) + "!", INTRA_SOURCE_INFO);
		LocalFree(s);
		return;
	}

	const DWORD lowOffset = DWORD(startByte);
	const DWORD highOffset = DWORD(ulong64(startByte) >> 32);
	const DWORD desiredAccess = DWORD(writeAccess? FILE_MAP_WRITE: FILE_MAP_READ);
	mData = MapViewOfFile(fileMapping, desiredAccess, highOffset, lowOffset, DWORD(mSize));
	CloseHandle(fileMapping);
#else
	int fd = open(fullFileName.CStr(), writeAccess? O_RDONLY: O_RDWR);
	if(fd <= 0)
	{
		status.Error("Cannot open file " + fileName + " for mapping: " + StringView(strerror(errno)) = "!", INTRA_SOURCE_INFO);
		return;
	}

	mData = mmap(null, bytes,
		writeAccess? PROT_WRITE: PROT_READ,
		MAP_SHARED, fd, long(startByte));

	if(mData == MAP_FAILED || mData == null)
	{
		mData = null;
		status.Error("Cannot mmap " + fileName + ": " + StringView(strerror(errno)) + "!", INTRA_SOURCE_INFO);
	}

	close(fd);
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
