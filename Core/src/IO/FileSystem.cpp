#include "IO/FileSystem.h"
#include "Platform/PlatformInfo.h"
#include "Container/Sequential/String.h"
#include "Algo/String/Path.h"
#include "Algo/Comparison/EndsWith.h"
#include "Algo/Mutation/Replace.h"
#include "IO/OsFile.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include <sys/stat.h>


#if(defined(INTRA_PLATFORM_IS_UNIX) || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

#include <unistd.h>
#include <sys/mman.h>
#include <cstdio>

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

static String osGetCurrentDirectory()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	wchar_t wpath[MAX_PATH];
	uint wlength = GetCurrentDirectoryW(MAX_PATH, wpath);
	char path[MAX_PATH*3];
	int length = WideCharToMultiByte(CP_UTF8, 0u, wpath, int(wlength), path, int(Meta::NumOf(path)), null, null);
	String result = StringView(path, size_t(length));
	if(!Algo::EndsWith(result, '\\')) result += '\\';
#else
	char path[2048];
	path[0] = '\0';
	String result = StringView(getcwd(path, sizeof(path)));
	if(!Algo::EndsWith(result, '/')) result += '/';
#endif
	return result;
}

OsFileSystem::OsFileSystem():
	mCurrentDirectory(osGetCurrentDirectory()) {}

String OsFileSystem::GetFullFileName(StringView fileName) const
{
	//TODO: сделать обработку и удаление ../, ./ и повтор€ющихс€ слешей
	String result = fileName;
	bool nameIsFull = false;
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	Algo::Replace(result, '/', '\\');
	nameIsFull = Algo::StartsWith(Range::Drop(result), ":\\");
#else
	Algo::Replace(result, '\\', '/');
	nameIsFull = Algo::StartsWith(result, '/');
#endif
	if(!nameIsFull) result = mCurrentDirectory+result;
	return result;
}

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

bool OsFileSystem::FileExists(StringView fileName) const
{
	String fullFileName = GetFullFileName(fileName);
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	auto wfn = utf8ToWStringZ(fullFileName);
	return PathFileExistsW(wfn.Data()) != 0;
#else
	return access(fullFileName.CStr(), 0) != -1;
#endif
}

bool OsFileSystem::FileDelete(StringView fileName)
{
	String fullFileName = GetFullFileName(fileName);
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	auto wfn = utf8ToWStringZ(fullFileName);
	return DeleteFileW(wfn.Data()) != 0;
#else
	return remove(fullFileName.CStr()) == 0;
#endif
}

bool OsFileSystem::FileMove(StringView oldFileName, StringView newFileName, bool overwriteExisting)
{
	String oldFullFileName = GetFullFileName(oldFileName);
	if(!FileExists(oldFullFileName)) return false;

	String newFullFileName = GetFullFileName(newFileName);
	if(FileExists(newFullFileName))
	{
		if(overwriteExisting) FileDelete(newFullFileName);
		else return false;
	}

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	auto wOldFN = utf8ToWStringZ(oldFullFileName);
	auto wNewFN = utf8ToWStringZ(newFullFileName);
	return MoveFileExW(wOldFN.Data(), wNewFN.Data(), MOVEFILE_COPY_ALLOWED) != 0;
#else
	return rename(oldFullFileName.CStr(), newFullFileName.CStr()) == 0;
#endif
}


FileInfo OsFileSystem::FileGetInfo(StringView fileName) const
{
	FileInfo result;
	String fullFileName = GetFullFileName(fileName);
#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if(!GetFileAttributesExW(utf8ToWStringZ(fullFileName).Data(), GetFileExInfoStandard, &fad)) return {0, 0};
	result.Size = (ulong64(fad.nFileSizeHigh) << 32)|fad.nFileSizeLow;
	result.LastModified = (ulong64(fad.ftLastWriteTime.dwHighDateTime) << 32)|fad.ftLastWriteTime.dwLowDateTime;
#else
	struct stat attrib;
	bool exist = stat(fullFileName.CStr(), &attrib) == 0;
	if(!exist) return {0, 0};
	result.LastModified = ulong64(attrib.st_mtime);
	result.Size = ulong64(attrib.st_size);
#endif
	return result;
}

ulong64 OsFileSystem::FileGetTime(StringView fileName) const
{return FileGetInfo(fileName).LastModified;}

ulong64 OsFileSystem::FileGetSize(StringView fileName) const
{return FileGetInfo(fileName).Size;}

OsFile OsFileSystem::FileOpen(StringView fileName)
{return OsFile(GetFullFileName(fileName), OsFile::Mode::Read);}

OsFile OsFileSystem::FileOpenWrite(StringView fileName)
{return OsFile(GetFullFileName(fileName), OsFile::Mode::Write);}

OsFile OsFileSystem::FileOpenReadWrite(StringView fileName)
{return OsFile(GetFullFileName(fileName), OsFile::Mode::ReadWrite);}

String OsFileSystem::FileToString(StringView fileName)
{return OsFile::ReadAsString(GetFullFileName(fileName));}

OsFileSystem OS;

}}

INTRA_WARNING_POP
