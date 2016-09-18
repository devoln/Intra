#include "IO/File.h"
#include "Algorithms/Range.h"
#include "Containers/String.h"

#include <stddef.h>

#include <stdio.h>
#include <sys/stat.h>


#if(defined(INTRA_PLATFORM_IS_POSIX) || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

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
#include <Windows.h>
#undef GetCurrentDirectory

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else

#endif


#ifdef _stat
#undef _stat
#endif

#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
#ifndef _ACRTIMP
#define _ACRTIMP
#endif
extern "C"
_ACRTIMP int INTRA_CRTDECL _stat(
	_In_z_ char const*     _FileName,
	_Out_  struct _stat32* _Stat
	);
#endif

#ifdef _MSC_VER
#define access _access
#define fileno _fileno


#endif

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 0

void* mmap(void* addr, size_t length, int prot, int flags, int fd, long offset)
{
	(void)addr; (void)flags;

	HANDLE fileHandle = (HANDLE)_get_osfhandle(fd);
	DWORD flProtect=0, dwDesiredAccess=0;
	if(prot==PROT_READ) flProtect=PAGE_READONLY, dwDesiredAccess=FILE_MAP_READ;
	else if(prot==PROT_WRITE) flProtect=PAGE_READWRITE, dwDesiredAccess=FILE_MAP_WRITE;
	else return (void*)-1;

	HANDLE hnd = CreateFileMappingW(fileHandle, nullptr, flProtect, 0, DWORD(length), nullptr);
	void* map = nullptr;
    if(hnd!=nullptr)
    {
        map = MapViewOfFile(hnd, dwDesiredAccess, 0, DWORD(offset), length);
        CloseHandle(hnd);
    }

    return map!=nullptr? map: (void*)-1;
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
	namespace DiskFile
	{
	bool Exists(StringView fileName) {String fn=fileName; return access(fn.CStr(), 0)!=-1;}
	bool Delete(StringView fileName) {String fn=fileName; return remove(fn.CStr()) == 0;}
	bool MoveOrRename(StringView oldFileName, StringView newFileName)
	{
		if(Exists(newFileName) || !Exists(oldFileName)) return false;
		return rename(String(oldFileName).CStr(), String(newFileName).CStr() ) == 0;
	}


	Info GetInfo(StringView fileName)
	{
		Info result;
		String fn=fileName;
#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
		struct _stat32 attrib;
		result.Exist = _stat(fn.CStr(), &attrib)==0;
#else
		struct stat attrib;
		result.Exist = stat(fn.CStr(), &attrib)==0;
#endif
		result.LastModified = result.Exist? ulong64(attrib.st_mtime): 0ull;
		result.Size = result.Exist? ulong64(attrib.st_size): 0ull;
		return result;
	}

	ulong64 GetFileTime(StringView fileName)
	{
		return GetInfo(fileName).LastModified;
	}

	String NormalizeSlashes(StringView path)
	{
		return path.Trim(Range::IsSpace<char>).ReplaceAll('\\', '/');
	}

	String AddTrailingSlash(StringView path)
	{
		return (path.Last()=='/' || path.Last()=='\\')? String(path): path+'/';
	}

	StringView RemoveTrailingSlash(StringView path)
	{
		return path.TrimRight(IsPathSeparator);
	}

	String GetCurrentDirectory()
	{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
		wchar_t wpath[MAX_PATH];
		uint wlength = GetCurrentDirectoryW(MAX_PATH, (LPWSTR)wpath);
		char path[MAX_PATH*3];
		int length = WideCharToMultiByte(CP_UTF8, 0u, wpath, int(wlength), path, int(core::numof(path)), null, null);
		const String result = StringView(path, size_t(length));
#else
		char path[2048];
		path[0] = '\0';
		String result = StringView(getcwd(path, sizeof(path)));
#endif
		return AddTrailingSlash(result);
	}


	void SplitPath(StringView fullPath, StringView* oDirectoryPath, StringView* oNameOnly, StringView* oExtension, StringView* oName)
	{
		size_t extLength=0;
		StringView pathWithoutExt = fullPath.Retro().Find('.', &extLength).Drop().Retro();
		if(oExtension)
		{
			if(extLength<fullPath.Length()) *oExtension = fullPath.Tail(extLength);
			else *oExtension = null;
		}
		size_t nameLength = 0;
		StringView directoryWithSlash = pathWithoutExt.Retro().Find(IsPathSeparator, &nameLength).Retro();
		if(oDirectoryPath) *oDirectoryPath = directoryWithSlash;
		if(oNameOnly) *oNameOnly = pathWithoutExt.Tail(nameLength);
		if(oName) *oName = fullPath.Tail(nameLength+1+extLength);
	}

	StringView ExtractDirectoryPath(StringView fullPath)
	{
		StringView result;
		SplitPath(fullPath, &result, null, null, null);
		return result;
	}

	StringView ExtractNameWithoutExtension(StringView fullPath)
	{
		StringView result;
		SplitPath(fullPath, null, &result, null, null);
		return result;
	}

	StringView ExtractName(StringView fullPath)
	{
		StringView result;
		SplitPath(fullPath, null, null, null, &result);
		return result;
	}

	StringView ExtractExtension(StringView fullPath)
	{
		StringView result;
		SplitPath(fullPath, null, null, &result, null);
		return result;
	}

	String ReplaceExtension(StringView fullPath, StringView newExtension)
	{
		StringView path, file;
		SplitPath(fullPath, &path, &file, null, null);
		return path+file+newExtension;
	}

	StringView GetParentPath(StringView path)
	{
		return path.TrimRight(IsPathSeparator).Retro().Find(IsPathSeparator).Retro();
	}

	bool IsAbsolutePath(StringView path)
	{
		if(path.Empty()) return false;
		if(IsPathSeparator(path.First())) return true;

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
		if(path.Length()>1 && path[0]>='A' && path[0]<='Z' && path[1]==':')
			return true;
#endif

		return false;
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
	{
		return DiskFile::GetFileTime(name);
	}

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
		int wnameLength = MultiByteToWideChar(CP_UTF8, 0, name.Data(), (int)name.Length(), utf16Name, int(core::numof(utf16Name)-1));
		utf16Name[wnameLength] = L'\0';
		wchar_t utf16Mode[4] = {0};
		StringView(modes[append][writeAccess][readAccess]).CopyTo(ArrayRange<wchar_t>(utf16Mode));
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
	{
		return fileno(reinterpret_cast<FILE*>(hndl));
	}


	void Reader::map(ulong64 firstByte, size_t bytes) const
	{
		auto size = GetSize();
		if(bytes==Meta::NumericLimits<size_t>::Max() || firstByte+bytes>size)
			bytes = size_t(size-firstByte);
#if(INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Emscripten)
		mapping.data = reinterpret_cast<byte*>(mmap(null, bytes, PROT_READ, MAP_SHARED, GetFileDescriptor(), long(firstByte)));
#else
		mapping.data = Memory::Allocate(bytes);
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
		Memory::Free(mapping.data);
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
