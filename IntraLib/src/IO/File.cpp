#include "IO/File.h"
#include "Algorithms/Range.h"
#include "Containers/String.h"

#include <stddef.h>

#include <stdio.h>
#include <sys/stat.h>


#if(INTRA_PLATFORM_IS_POSIX || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
#include <unistd.h>
#include <sys/mman.h>
#elif INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
#include <io.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#undef GetCurrentDirectory
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
	(void)(addr, flags);

	HANDLE fileHandle = (HANDLE)_get_osfhandle(fd);
	DWORD flProtect=0, dwDesiredAccess=0;
	if(prot==PROT_READ) flProtect=PAGE_READONLY, dwDesiredAccess=FILE_MAP_READ;
	else if(prot==PROT_WRITE) flProtect=PAGE_READWRITE, dwDesiredAccess=FILE_MAP_WRITE;
	else return (void*)-1;

	HANDLE hnd = CreateFileMappingW(fileHandle, nullptr, flProtect, 0, (DWORD)length, nullptr);
	void* map = nullptr;
    if(hnd!=nullptr)
    {
        map = MapViewOfFile(hnd, dwDesiredAccess, 0, offset, length);
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
		result.LastModified=attrib.st_mtime;
		result.Size=attrib.st_size;
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
		int length = WideCharToMultiByte(CP_UTF8, 0, wpath, (int)wlength, path, (int)core::numof(path), null, null);
		const String result = StringView(path, length);
#else
		char path[MAX_PATH];
		path[0] = 0;
		getcwd(path, MAX_PATH);
		const String result = String(path, strlen(path));
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
		return file.ReadNChars((size_t)file.GetSize());
	}


	ulong64 CommonFileImpl::GetFileTime() const
	{
		return DiskFile::GetFileTime(name);
	}

	void CommonFileImpl::open(StringView file, bool readAccess, bool writeAccess, bool append, Error* oError)
	{
		close();
		if(file==null) return;
		name = file;
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
		hndl = fopen(Name.CStr(), modes[append][writeAccess][readAccess]);
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
		fclose((FILE*)hndl);
		hndl=null;
	}

	int CommonFileImpl::GetFileDescriptor() const
	{
		return fileno((FILE*)hndl);
	}


	void Reader::map(ulong64 firstByte, size_t bytes) const
	{
		auto size = GetSize();
		if(bytes==Meta::NumericLimits<size_t>::Max() || firstByte+bytes>size)
			bytes = size_t(size-firstByte);
#if(INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Emscripten)
		mapping.data = (byte*)mmap(null, bytes, PROT_READ, MAP_SHARED, GetFileDescriptor(), (long)firstByte);
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


	bool Reader::EndOfStream() const {return feof((FILE*)hndl)!=0;}

	size_t Reader::ReadData(void* data, size_t bytes)
	{
		INTRA_ASSERT(hndl!=null);
		INTRA_ASSERT(data!=null);
		size_t bytesRead = fread(data, 1, bytes, (FILE*)hndl);
		//if(bytesRead<bytes) memset((byte*)data+bytesRead, 0, bytes-bytesRead);
		return bytesRead;
	}

	void Reader::UnreadData(const void* src, size_t bytes)
	{
		for(byte* ptr = (byte*)src+bytes; ptr>src;)
			ungetc(*--ptr, (FILE*)hndl);
	}


	void Writer::WriteData(const void* data, size_t bytes)
	{
		INTRA_ASSERT(hndl!=null);
		fwrite(data, 1, bytes, (FILE*)hndl);
		//fflush((FILE*)hndl);
	}



	//Установить позицию чтения
	void Reader::SetPos(ulong64 bytes)
	{
		if(hndl==null) return;
#ifdef _MSC_VER
		fseeko64((FILE*)hndl, bytes, SEEK_SET);
#else
		fseek((FILE*)hndl, (long)bytes, SEEK_SET);
#endif
	}

	ulong64 Reader::GetPos() const
	{
		if(hndl==null) return 0;
#ifdef _MSC_VER
		return ftello64((FILE*)hndl);
#else
		return ftell((FILE*)hndl);
#endif
	}

	ulong64 Reader::GetSize() const
	{
		if(hndl==null) return 0;
		const ulong64 oldpos = GetPos();
		fseek((FILE*)hndl, 0, SEEK_END);
		const ulong64 size = GetPos();
		const_cast<Reader*>(this)->SetPos(oldpos);
		return size;
	}

	//Установить позицию записи
	void Writer::SetPos(ulong64 bytes)
	{
		if(hndl==null) return;
#ifdef _MSC_VER
		fseeko64((FILE*)hndl, bytes, SEEK_SET);
#else
		fseek((FILE*)hndl, (long)bytes, SEEK_SET);
#endif
	}

	ulong64 Writer::GetPos() const
	{
		if(hndl==null) return 0;
#ifdef _MSC_VER
		return ftello64((FILE*)hndl);
#else
		return ftell((FILE*)hndl);
#endif
	}

	ulong64 Writer::GetSize() const
	{
		if(hndl==null) return 0;
		const auto oldpos = GetPos();
		fseek((FILE*)hndl, 0, SEEK_END);
		const auto size = GetPos();
		const_cast<Writer*>(this)->SetPos(oldpos);
		return size;
	}

	}
}}
