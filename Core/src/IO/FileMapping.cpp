#include "IO/FileMapping.h"
#include "IO/FileSystem.h"
#include "Memory/Allocator/Global.h"

#if(defined(INTRA_PLATFORM_IS_UNIX) || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

#include <unistd.h>
#include <sys/mman.h>

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

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

static GenericString<wchar_t> utf8ToWStringZ(StringView str)
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

BasicFileMapping::BasicFileMapping(StringView fileName, size_t startByte, size_t bytes, bool writeAccess):
	mData(null), mSize(0)
{
	String fullFileName = OS.GetFullFileName(fileName);
	auto size = OS.FileGetSize(fullFileName);
	
	if(bytes==Meta::NumericLimits<size_t>::Max())
		bytes = size_t(size-startByte);

	if(startByte+bytes>size) return;
	mSize = bytes;
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
	mData = Memory::GlobalHeap.Allocate(bytes, INTRA_SOURCE_INFO);
	//TODO исправить этот код, когда новый класс файла будет готов
	auto pos = GetPos();
	auto This = const_cast<Reader*>(this);
	This->SetPos(firstByte);
	This->ReadData(mapping.data, bytes);
	This->SetPos(pos);
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	HANDLE hFile = CreateFileW(utf8ToWStringZ("\\\\?\\"+fullFileName).Data(),
		GENERIC_READ|(writeAccess? GENERIC_WRITE: 0),
		FILE_SHARE_READ, null,
		writeAccess? OPEN_ALWAYS: OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, null);

	HANDLE fileMapping = CreateFileMappingW(hFile, null,
		writeAccess? PAGE_READWRITE: PAGE_READONLY,
		DWORD((mSize+startByte) >> 32), DWORD(mSize+startByte), null);

	if(fileMapping != null)
	{
		mData = MapViewOfFile(fileMapping,
			writeAccess? FILE_MAP_WRITE: FILE_MAP_READ,
			DWORD(startByte >> 32), DWORD(startByte), DWORD(mSize));
		CloseHandle(fileMapping);
	}
#else
	int fd = open(fullFileName.CStr(), writeAccess? O_RDONLY: O_RDWR);
	mData = mmap(null, bytes,
		writeAccess? PROT_WRITE: PROT_READ,
		MAP_SHARED, fd, long(firstByte));
	close(fd);
#endif
}

void BasicFileMapping::Close()
{
	if(mData==null) return;
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
	Memory::GlobalHeap.Free(mData, mSize);
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	UnmapViewOfFile(mData);
#else
	munmap(mData, mSize);
#endif
	mData = null;
	mSize = 0;
}

void WritableFileMapping::Flush()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	FlushViewOfFile(mData, 0);
#else
	msync(mData, mSize, MS_SYNC);
#endif
}


}}
