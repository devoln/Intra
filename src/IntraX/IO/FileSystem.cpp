#include "IntraX/IO/FileSystem.h"

#include "Intra/Range/Comparison.h"
#include "Intra/Range/Mutation/Replace.h"

#include "IntraX/Container/Sequential/String.h"

#include "IntraX/IO/FilePath.h"
#include "IntraX/IO/OsFile.h"
#include "IntraX/IO/FileReader.h"
#include "IntraX/IO/FileWriter.h"
#include "IntraX/IO/Std.h"

#include "IntraX/System/detail/Common.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
#include <sys/stat.h>

#ifdef __unix__

#include <unistd.h>
#include <sys/mman.h>
#include <cstdio>
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
static String osGetCurrentDirectory()
{
#ifdef _WIN32
	wchar_t wpath[MAX_PATH];
	unsigned wlength = GetCurrentDirectoryW(MAX_PATH, wpath);
	char path[MAX_PATH*3];
	int length = WideCharToMultiByte(CP_UTF8, 0u, wpath, int(wlength), path, sizeof(path), nullptr, nullptr);
	String result = StringView::FromPointerAndLength(path, length);
	if(!EndsWith(result, '\\')) result += '\\';
#else
	char path[2048];
	path[0] = '\0';
	String result = StringView(getcwd(path, sizeof(path)));
	if(!EndsWith(result, '/')) result += '/';
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
#ifdef _WIN32
	Replace(result, '/', '\\');
	nameIsFull = StartsWith(result, "\\") || result.View().Drop().StartsWith(":\\");
#else
	Replace(result, '\\', '/');
	nameIsFull = StartsWith(result, '/');
#endif
	if(!nameIsFull) result = mCurrentDirectory + result;
	return result;
}

bool OsFileSystem::FileExists(StringView fileName) const
{
	String fullFileName = GetFullFileName(fileName);
#ifdef _WIN32
	auto wfn = detail::Utf8ToWStringZ(fullFileName);
	return PathFileExistsW(wfn.Data()) != 0;
#else
	return access(fullFileName.CStr(), 0) != -1;
#endif
}

bool OsFileSystem::FileDelete(StringView fileName)
{
	String fullFileName = GetFullFileName(fileName);
#ifdef _WIN32
	auto wfn = detail::Utf8ToWStringZ(fullFileName);
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

#ifdef _WIN32
	const auto wOldFN = detail::Utf8ToWStringZ(oldFullFileName);
	const auto wNewFN = detail::Utf8ToWStringZ(newFullFileName);
	return MoveFileExW(wOldFN.Data(), wNewFN.Data(), MOVEFILE_COPY_ALLOWED) != 0;
#else
	return rename(oldFullFileName.CStr(), newFullFileName.CStr()) == 0;
#endif
}


FileInfo OsFileSystem::FileGetInfo(StringView fileName, ErrorReporter err) const
{
	FileInfo result;
	String fullFileName = GetFullFileName(fileName);
#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if(!GetFileAttributesExW(detail::Utf8ToWStringZ(fullFileName).Data(), GetFileExInfoStandard, &fad))
	{
		detail::ProcessLastError(err, "Cannot get attributes of file " + fileName + ": ", INTRA_SOURCE_INFO);
		return {0, 0};
	}
	result.Size = (uint64(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow;
	result.LastModified = (uint64(fad.ftLastWriteTime.dwHighDateTime) << 32) | fad.ftLastWriteTime.dwLowDateTime;
#else
	struct stat attrib;
	const bool success = stat(fullFileName.CStr(), &attrib) == 0;
	if(!success)
	{
		detail::ProcessLastError(status, "Cannot get attributes of file " + fileName + ": ", INTRA_SOURCE_INFO);
		return {0, 0};
	}
	result.LastModified = uint64(attrib.st_mtime);
	result.Size = uint64(attrib.st_size);
#endif
	return result;
}

uint64 OsFileSystem::FileGetTime(StringView fileName, ErrorReporter err) const
{return FileGetInfo(fileName, err).LastModified;}

uint64 OsFileSystem::FileGetSize(StringView fileName, ErrorReporter err) const
{return FileGetInfo(fileName, err).Size;}

FileReader OsFileSystem::FileOpen(StringView fileName, ErrorReporter err)
{return FileReader(Shared<OsFile>::New(GetFullFileName(fileName), OsFile::Mode::Read, err));}

FileWriter OsFileSystem::FileOpenWrite(StringView fileName, uint64 offset, ErrorReporter err)
{return FileWriter(Shared<OsFile>::New(GetFullFileName(fileName), OsFile::Mode::Write, err), offset);}

FileWriter OsFileSystem::FileOpenWrite(StringView fileName, ErrorReporter err)
{return FileOpenWrite(fileName, 0, err);}

FileWriter OsFileSystem::FileOpenOverwrite(StringView fileName, ErrorReporter err)
{return FileWriter::Overwrite(Shared<OsFile>::New(GetFullFileName(fileName), OsFile::Mode::Write, err));}

FileWriter OsFileSystem::FileOpenAppend(StringView fileName, ErrorReporter err)
{
	auto file = Shared<OsFile>::New(GetFullFileName(fileName), OsFile::Mode::Write, err);
	return FileWriter::Append(Move(file));
}

String OsFileSystem::FileToString(StringView fileName, ErrorReporter err)
{return OsFile::ReadAsString(GetFullFileName(fileName), err);}

INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
OsFileSystem OS;

} INTRA_END
