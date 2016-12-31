#include "IO/File.h"
#include "Algo/Mutation/Copy.h"
#include "Algo/Search.h"
#include "Algo/String/Path.h"
#include "Range/ArrayRange.h"
#include "Containers/String.h"
#include "Memory/Allocator.h"
#include "Range/Iteration/Retro.h"
#include "Platform/Compatibility.h"

#include <stddef.h>

#include <stdio.h>
#include <sys/stat.h>


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


#ifdef _MSC_VER
#define fileno _fileno


#endif

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 0

void* mmap(void* addr, size_t length, int prot, int flags, int fd, long offset)
{
	(void)addr; (void)flags;

	HANDLE fileHandle = HANDLE(_get_osfhandle(fd));
	DWORD flProtect=0, dwDesiredAccess=0;
	if(prot==PROT_READ)
	{
		flProtect = PAGE_READONLY;
		dwDesiredAccess = FILE_MAP_READ;
	}
	else if(prot==PROT_WRITE)
	{
		flProtect = PAGE_READWRITE;
		dwDesiredAccess = FILE_MAP_WRITE;
	}
	else return reinterpret_cast<void*>(-1);

	HANDLE hnd = CreateFileMappingW(fileHandle, nullptr, flProtect, 0, DWORD(length), nullptr);
	void* map = nullptr;
    if(hnd!=nullptr)
    {
        map = MapViewOfFile(hnd, dwDesiredAccess, 0, DWORD(offset), length);
        CloseHandle(hnd);
    }

    return map!=nullptr? map: reinterpret_cast<void*>(-1);
}

int munmap(void* addr, size_t length)
{
	(void)length;
    return UnmapViewOfFile(addr)!=0? 0: -1;
}

#endif

#undef EOF

namespace Intra { namespace IO
{
namespace DiskFile {

bool Exists(StringView fileName)
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	GenericString<wchar_t> wfn;
	wfn.SetLengthUninitialized(fileName.Length());
	int wlen = MultiByteToWideChar(CP_UTF8, 0, fileName.Data(),
		int(fileName.Length()), wfn.Data(), int(wfn.Length()));
	wfn.SetLengthUninitialized(size_t(wlen+1));
	wfn.Last() = 0;
	return PathFileExistsW(wfn.Data())!=0;
#else
	String fn = fileName;
	return access(fn.CStr(), 0)!=-1;
#endif
}

bool Delete(StringView fileName) {String fn=fileName; return remove(fn.CStr()) == 0;}

bool MoveOrRename(StringView oldFileName, StringView newFileName)
{
	if(Exists(newFileName) || !Exists(oldFileName)) return false;
	return rename(String(oldFileName).CStr(), String(newFileName).CStr() ) == 0;
}


Info GetInfo(StringView fileName)
{
	Info result;
	String fn = fileName;
#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
	wchar_t buf[MAX_PATH+1];
	int wstrLength = MultiByteToWideChar(CP_UTF8, 0, fileName.Data(), int(fileName.Length()), buf, MAX_PATH);
	buf[wstrLength]=0;

	WIN32_FILE_ATTRIBUTE_DATA fad;
	if(!GetFileAttributesExW(buf, GetFileExInfoStandard, &fad)) return {false, 0, 0};
	result.Exist = Exists(fileName);
	result.Size = (ulong64(fad.nFileSizeHigh) << 32)|fad.nFileSizeLow;
	result.LastModified = (ulong64(fad.ftLastWriteTime.dwHighDateTime) << 32)|fad.ftLastWriteTime.dwLowDateTime;

#else
	struct stat attrib;
	result.Exist = stat(fn.CStr(), &attrib)==0;
	result.LastModified = result.Exist? ulong64(attrib.st_mtime): 0ull;
	result.Size = result.Exist? ulong64(attrib.st_size): 0ull;
#endif
	return result;
}

ulong64 GetFileTime(StringView fileName)
{return GetInfo(fileName).LastModified;}

String GetCurrentDirectory()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	wchar_t wpath[MAX_PATH];
	uint wlength = GetCurrentDirectoryW(MAX_PATH, reinterpret_cast<LPWSTR>(wpath));
	char path[MAX_PATH*3];
	int length = WideCharToMultiByte(CP_UTF8, 0u, wpath, int(wlength), path, int(Meta::NumOf(path)), null, null);
	const String result = StringView(path, size_t(length));
#else
	char path[2048];
	path[0] = '\0';
	String result = StringView(getcwd(path, sizeof(path)));
#endif
	return Algo::Path::AddTrailingSlash(result);
}



String ReadAsString(StringView fileName, bool* fileOpened)
{
	Reader file(fileName);
	if(fileOpened!=null) *fileOpened = (file!=null);
	if(file==null) return null;
	size_t size = size_t(DiskFile::GetInfo(fileName).Size);
	if(size==0)
	{
		String result;
		while(!file.EndOfStream())
			result += file.Read<char>();
		return result;
	}
	return file.ReadNChars(size);
}


ulong64 CommonFileImpl::GetFileTime() const
{return DiskFile::GetFileTime(name);}

void CommonFileImpl::open(StringView fileName, bool readAccess, bool writeAccess, bool append, Error* oError)
{
	close();
	if(fileName==null) return;
	name = fileName;
	const char* const modes[2][2][2]={{{null, "rb"}, {"wb", "w+b"}}, {{null, null}, {"ab", "a+b"}}};
	if(modes[append][writeAccess][readAccess]==null)
	{
		if(oError!=null) *oError = Error::InvalidArguments;
		return;
	}
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows) //Поддержка UTF-8 путей в Windows
	wchar_t utf16Name[MAX_PATH+1];
	int wnameLength = MultiByteToWideChar(CP_UTF8, 0, name.Data(), int(name.Length()), utf16Name, int(Meta::NumOf(utf16Name)-1));
	utf16Name[wnameLength] = L'\0';
	wchar_t utf16Mode[4] = {0};
	Algo::CopyTo(StringView(modes[append][writeAccess][readAccess]), ArrayRange<wchar_t>(utf16Mode));
	hndl = _wfopen(utf16Name, utf16Mode);
#else
	hndl = fopen(name.CStr(), modes[append][writeAccess][readAccess]);
#endif
	if(hndl==null)
	{
		if(oError!=null) *oError = Error::NotFound;
	}
	if(oError!=null) *oError = Error::NoError;
}

void CommonFileImpl::close()
{
	if(hndl==null) return;
	fclose(reinterpret_cast<FILE*>(hndl));
	hndl=null;
}

int CommonFileImpl::GetFileDescriptor() const
{return fileno(reinterpret_cast<FILE*>(hndl));}


void Reader::map(ulong64 firstByte, size_t bytes) const
{
	auto size = GetSize();
	if(bytes==Meta::NumericLimits<size_t>::Max() || firstByte+bytes>size)
		bytes = size_t(size-firstByte);
#if(INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Emscripten)
	mapping.data = reinterpret_cast<byte*>(mmap(null, bytes, PROT_READ, MAP_SHARED, GetFileDescriptor(), long(firstByte)));
#else
	mapping.data = Memory::GlobalHeap.Allocate(bytes, INTRA_SOURCE_INFO);
	auto pos = GetPos();
	auto This = const_cast<Reader*>(this);
	This->SetPos(firstByte);
	This->ReadData(mapping.data, bytes);
	This->SetPos(pos);
#endif
	mapping.hndl = mapping.data;
}

void Reader::Unmap() const
{
	if(mapping.hndl==null) return;
#if(INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Emscripten)
	munmap(mapping.data, mapping.Size);
#else
	Memory::GlobalHeap.Free(mapping.data, mapping.Size);
#endif
	mapping.data = null;
	mapping.hndl = null;
	mapping.Size = 0;
}


bool Reader::EndOfStream() const {return feof(reinterpret_cast<FILE*>(hndl))!=0;}

size_t Reader::ReadData(void* data, size_t bytes)
{
	if(bytes==0) return 0;
	INTRA_ASSERT(hndl!=null);
	INTRA_ASSERT(data!=null);
	size_t bytesRead = fread(data, 1, bytes, reinterpret_cast<FILE*>(hndl));
	//if(bytesRead<bytes) memset(reinterpret_cast<byte*>(data)+bytesRead, 0, bytes-bytesRead);
	return bytesRead;
}

void Reader::UnreadData(const void* src, size_t bytes)
{
	for(const byte* ptr = reinterpret_cast<const byte*>(src)+bytes; ptr>src;)
		ungetc(*--ptr, reinterpret_cast<FILE*>(hndl));
}


void Writer::WriteData(const void* data, size_t bytes)
{
	INTRA_ASSERT(hndl!=null);
	fwrite(data, 1, bytes, reinterpret_cast<FILE*>(hndl));
	//fflush(reinterpret_cast<FILE*>(hndl));
}



//Установить позицию чтения
void Reader::SetPos(ulong64 bytes)
{
	if(hndl==null) return;
#ifdef _MSC_VER
	fseeko64(reinterpret_cast<FILE*>(hndl), bytes, SEEK_SET);
#else
	fseek(reinterpret_cast<FILE*>(hndl), long(bytes), SEEK_SET);
#endif
}

ulong64 Reader::GetPos() const
{
	if(hndl==null) return 0;
#ifdef _MSC_VER
	return ulong64(ftello64(reinterpret_cast<FILE*>(hndl)));
#else
	return ulong64(ftell(reinterpret_cast<FILE*>(hndl)));
#endif
}

ulong64 Reader::GetSize() const
{
	if(hndl==null) return 0;
	return DiskFile::GetInfo(name).Size;
}

//Установить позицию записи
void Writer::SetPos(ulong64 bytes)
{
	if(hndl==null) return;
#ifdef _MSC_VER
	fseeko64(reinterpret_cast<FILE*>(hndl), bytes, SEEK_SET);
#else
	fseek(reinterpret_cast<FILE*>(hndl), long(bytes), SEEK_SET);
#endif
}

ulong64 Writer::GetPos() const
{
	if(hndl==null) return 0;
#ifdef _MSC_VER
	return ulong64(ftello64(reinterpret_cast<FILE*>(hndl)));
#else
	return ulong64(ftell(reinterpret_cast<FILE*>(hndl)));
#endif
}

ulong64 Writer::GetSize() const
{
	if(hndl==null) return 0;
	const auto oldpos = GetPos();
	fseek(reinterpret_cast<FILE*>(hndl), 0, SEEK_END);
	const auto size = GetPos();
	const_cast<Writer*>(this)->SetPos(oldpos);
	return size;
}

}
}}
