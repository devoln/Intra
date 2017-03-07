#include "IO/File.h"
#include "Algo/Mutation/Copy.h"
#include "Algo/String/Path.h"
#include "Range/Generators/ArrayRange.h"
#include "Container/Sequential/String.h"
#include "Memory/Allocator.hh"
#include "Platform/Compatibility.h"
#include "Platform/CppWarnings.h"
#include "IO/FileSystem.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

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



#undef EOF

namespace Intra { namespace IO {

namespace DiskFile {

String ReadAsString(StringView fileName, bool* fileOpened)
{
	Reader file(fileName);
	if(fileOpened!=null) *fileOpened = (file!=null);
	if(file==null) return null;
	size_t size = size_t(OS.FileGetInfo(fileName).Size);
	if(size==0)
	{
		String result;
		while(!file.EndOfStream())
			result += file.Read<char>();
		return result;
	}
	return file.ReadNChars(size);
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





bool Reader::EndOfStream() const {return feof(reinterpret_cast<FILE*>(hndl))!=0;}

size_t Reader::ReadData(void* data, size_t bytes)
{
	if(bytes==0) return 0;
	INTRA_DEBUG_ASSERT(hndl!=null);
	INTRA_DEBUG_ASSERT(data!=null);
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
	INTRA_DEBUG_ASSERT(hndl!=null);
	fwrite(data, 1, bytes, reinterpret_cast<FILE*>(hndl));
	//fflush(reinterpret_cast<FILE*>(hndl));
}

void Writer::Flush()
{
	if(hndl==null) return;
	fflush(reinterpret_cast<FILE*>(hndl));
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
	return OS.FileGetSize(name);
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
