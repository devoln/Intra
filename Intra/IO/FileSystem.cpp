﻿#include "IO/FileSystem.h"

#include "Cpp/PlatformDetect.h"

#include "Range/Comparison/EndsWith.h"
#include "Range/Mutation/Replace.h"

#include "Container/Sequential/String.h"

#include "IO/FilePath.h"
#include "IO/OsFile.h"
#include "IO/FileReader.h"
#include "IO/FileWriter.h"
#include "IO/Std.h"

#include "System/detail/Common.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#include <sys/stat.h>


#if(defined(INTRA_PLATFORM_IS_UNIX) || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

#include <unistd.h>
#include <sys/mman.h>
#include <cstdio>
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

static String osGetCurrentDirectory()
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	wchar_t wpath[MAX_PATH];
	uint wlength = GetCurrentDirectoryW(MAX_PATH, wpath);
	char path[MAX_PATH*3];
	int length = WideCharToMultiByte(CP_UTF8, 0u, wpath, int(wlength), path, sizeof(path), null, null);
	String result = StringView(path, size_t(length));
	if(!Range::EndsWith(result, '\\')) result += '\\';
#else
	char path[2048];
	path[0] = '\0';
	String result = StringView(getcwd(path, sizeof(path)));
	if(!Range::EndsWith(result, '/')) result += '/';
#endif
	return result;
}

OsFileSystem::OsFileSystem():
	mCurrentDirectory(osGetCurrentDirectory()) {}

String OsFileSystem::GetFullFileName(StringView fileName) const
{
	//TODO: сделать обработку и удаление ../, ./ и повтор¤ющихс¤ слешей
	String result = fileName;
	bool nameIsFull = false;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	Range::Replace(result, '/', '\\');
	nameIsFull = Range::StartsWith(result, "\\") || result.Drop().StartsWith(":\\");
#else
	Range::Replace(result, '\\', '/');
	nameIsFull = Range::StartsWith(result, '/');
#endif
	if(!nameIsFull) result = mCurrentDirectory + result;
	return result;
}

bool OsFileSystem::FileExists(StringView fileName) const
{
	String fullFileName = GetFullFileName(fileName);
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	auto wfn = System::detail::Utf8ToWStringZ(fullFileName);
	return PathFileExistsW(wfn.Data()) != 0;
#else
	return access(fullFileName.CStr(), 0) != -1;
#endif
}

bool OsFileSystem::FileDelete(StringView fileName)
{
	String fullFileName = GetFullFileName(fileName);
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	auto wfn = System::detail::Utf8ToWStringZ(fullFileName);
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

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	const auto wOldFN = System::detail::Utf8ToWStringZ(oldFullFileName);
	const auto wNewFN = System::detail::Utf8ToWStringZ(newFullFileName);
	return MoveFileExW(wOldFN.Data(), wNewFN.Data(), MOVEFILE_COPY_ALLOWED) != 0;
#else
	return rename(oldFullFileName.CStr(), newFullFileName.CStr()) == 0;
#endif
}


FileInfo OsFileSystem::FileGetInfo(StringView fileName, ErrorStatus& status) const
{
	FileInfo result;
	String fullFileName = GetFullFileName(fileName);
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if(!GetFileAttributesExW(System::detail::Utf8ToWStringZ(fullFileName).Data(), GetFileExInfoStandard, &fad))
	{
		System::detail::ProcessLastError(status, "Cannot get attributes of file " + fileName + ": ", INTRA_SOURCE_INFO);
		return {0, 0};
	}
	result.Size = (ulong64(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow;
	result.LastModified = (ulong64(fad.ftLastWriteTime.dwHighDateTime) << 32) | fad.ftLastWriteTime.dwLowDateTime;
#else
	struct stat attrib;
	const bool success = stat(fullFileName.CStr(), &attrib) == 0;
	if(!success)
	{
		System::detail::ProcessLastError(status, "Cannot get attributes of file " + fileName + ": ", INTRA_SOURCE_INFO);
		return {0, 0};
	}
	result.LastModified = ulong64(attrib.st_mtime);
	result.Size = ulong64(attrib.st_size);
#endif
	return result;
}

ulong64 OsFileSystem::FileGetTime(StringView fileName, ErrorStatus& status) const
{return FileGetInfo(fileName, status).LastModified;}

ulong64 OsFileSystem::FileGetSize(StringView fileName, ErrorStatus& status) const
{return FileGetInfo(fileName, status).Size;}

FileReader OsFileSystem::FileOpen(StringView fileName, ErrorStatus& status)
{return FileReader(Shared<OsFile>::New(GetFullFileName(fileName), OsFile::Mode::Read, status));}

FileWriter OsFileSystem::FileOpenWrite(StringView fileName, ulong64 offset, ErrorStatus& status)
{return FileWriter(Shared<OsFile>::New(GetFullFileName(fileName), OsFile::Mode::Write, status), offset);}

FileWriter OsFileSystem::FileOpenWrite(StringView fileName, ErrorStatus& status)
{return FileOpenWrite(fileName, 0, status);}

FileWriter OsFileSystem::FileOpenOverwrite(StringView fileName, ErrorStatus& status)
{return FileWriter::Overwrite(Shared<OsFile>::New(GetFullFileName(fileName), OsFile::Mode::Write, status));}

FileWriter OsFileSystem::FileOpenAppend(StringView fileName, ErrorStatus& status)
{
	auto file = Shared<OsFile>::New(GetFullFileName(fileName), OsFile::Mode::Write, status);
	return FileWriter::Append(Cpp::Move(file));
}

String OsFileSystem::FileToString(StringView fileName, ErrorStatus& status)
{return OsFile::ReadAsString(GetFullFileName(fileName), status);}

OsFileSystem OS;

}}

INTRA_WARNING_POP
