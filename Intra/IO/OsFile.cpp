#include "IO/OsFile.h"

#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"

#include "Container/Sequential/String.h"

#include "IO/FileSystem.h"
#include "IO/FileReader.h"
#include "IO/FileWriter.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(defined(INTRA_PLATFORM_IS_UNIX) || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Android || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

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

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

static GenericString<wchar_t> utf8ToWStringZ(StringView str)
{
	GenericString<wchar_t> wfn;
	wfn.SetLengthUninitialized(str.Length());
	const int wlen = MultiByteToWideChar(CP_UTF8, 0, str.Data(),
		int(str.Length()), wfn.Data(), int(wfn.Length()));
	wfn.SetLengthUninitialized(size_t(wlen + 1));
	wfn.Last() = 0;
	return wfn;
}

static void ProcessLastError(ErrorStatus& status, StringView message, const Utils::SourceInfo& srcInfo)
{
	const auto le = GetLastError();
	char* s = null;
	FormatMessageA(DWORD(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS),
		null, le,
		DWORD(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)),
		reinterpret_cast<char*>(&s), 0, null);
	status.Error(message + StringView(s) + "!", srcInfo);
	LocalFree(s);
}

#else

static forceinline StringView strerrorHelper(int, const char* buf)
{return StringView(buf);}

static forceinline StringView strerrorHelper(const char* str, const char*)
{return StringView(str);}

static void ProcessLastError(ErrorStatus& status, StringView message, const Utils::SourceInfo& srcInfo)
{
	char buf[64];
	status.Error(message + strerrorHelper(strerror_r(errno, buf, sizeof(buf)), buf) + "!", srcInfo);
}

#endif

OsFile::OsFile(StringView fileName, Mode mode, bool disableSystemBuffering, ErrorStatus& status):
	mMode(mode), mOwning(true)
{
	mFullPath = OS.GetFullFileName(fileName);
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	auto wFullFileName = utf8ToWStringZ(mFullPath);

	DWORD desiredAccess = GENERIC_READ;
	if(mode == Mode::Write) desiredAccess = GENERIC_WRITE;
	if(mode == Mode::ReadWrite) desiredAccess |= GENERIC_WRITE;

	const DWORD creationDisposition = DWORD(mode == Mode::Read? OPEN_EXISTING: OPEN_ALWAYS);
	DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
	if(disableSystemBuffering) flagsAndAttributes |= FILE_FLAG_NO_BUFFERING;

	const HANDLE hFile = CreateFileW(wFullFileName.Data(), desiredAccess,
		FILE_SHARE_READ, null, creationDisposition, flagsAndAttributes, null);

	if(hFile != INVALID_HANDLE_VALUE) mHandle = NativeHandle(hFile);
	else mHandle = null;
#else
	auto openFlags = (mMode == Mode::Read)? O_RDONLY: (mMode == Mode::Write? O_WRONLY: O_RDWR);
	if(mMode != Mode::Read) openFlags |= O_CREAT;
	if(disableSystemBuffering) openFlags |= O_DIRECT;
	const int fd = open(mFullPath.CStr(), openFlags, 0664);
	if(fd != -1) mHandle = NativeHandle(size_t(fd));
	else mHandle = null;
#endif
	if(mHandle == null)
	{
		mOwning = false;
		ProcessLastError(status, "Cannot open file " + fileName + ": ", INTRA_SOURCE_INFO);
	}
}

void OsFile::Close()
{
	if(mHandle == null) return;
	if(!mOwning) return;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	CloseHandle(HANDLE(mHandle));
#else
	close(int(size_t(mHandle)));
#endif
	mOwning = false;
}

size_t OsFile::ReadData(ulong64 fileOffset, void* dst, size_t bytes, ErrorStatus& status) const
{
	INTRA_DEBUG_ASSERT(mHandle != null);
	INTRA_DEBUG_ASSERT(mMode == Mode::Read || mMode == Mode::ReadWrite);
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	char* pdst = static_cast<char*>(dst);
	size_t totalBytesRead = 0;
	DWORD bytesRead;
	OVERLAPPED overlapped{};
	while(bytes > 0xFFFF0000)
	{
		overlapped.Offset = DWORD(fileOffset);
		overlapped.OffsetHigh = DWORD(fileOffset >> 32);
		if(!ReadFile(HANDLE(mHandle), pdst, 0xFFFF0000, &bytesRead, &overlapped) && GetLastError() != ERROR_HANDLE_EOF)
			ProcessLastError(status, String::Concat("Cannot read file ", FullPath(), " at offset ", fileOffset, ": "), INTRA_SOURCE_INFO);
		fileOffset += bytesRead;
		totalBytesRead += bytesRead;
		bytes -= bytesRead;
		pdst += bytesRead;
		if(bytesRead < 0xFFFF0000) return totalBytesRead;
	}
	overlapped.Offset = DWORD(fileOffset);
	overlapped.OffsetHigh = DWORD(fileOffset >> 32);
	if(!ReadFile(HANDLE(mHandle), pdst, DWORD(bytes), &bytesRead, &overlapped) && GetLastError() != ERROR_HANDLE_EOF)
		ProcessLastError(status, String::Concat("Cannot read file ", FullPath(), " at offset ", fileOffset, ": "), INTRA_SOURCE_INFO);
	totalBytesRead += bytesRead;
	return totalBytesRead;
#else
	auto bytesRead = pread(int(size_t(mHandle)), dst, bytes, off_t(fileOffset));
	if(bytesRead < 0)
	{
		bytesRead = 0;
		ProcessLastError(status, String::Concat("Cannot read file ", FullPath(), " at offset ", fileOffset, ": "), INTRA_SOURCE_INFO);
	}
	return size_t(bytesRead);
#endif
}

ulong64 OsFile::Size(ErrorStatus& status) const
{
	if(mHandle == null) return 0;
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	LARGE_INTEGER result;
	result.QuadPart = 0;
	if(GetFileSizeEx(HANDLE(mHandle), &result) == 0 && GetLastError() != 0)
		ProcessLastError(status, "Cannot get size of file " + FullPath() + ": ", INTRA_SOURCE_INFO);
	return ulong64(result.QuadPart);
#else
	const off_t result = lseek(int(size_t(mHandle)), 0, SEEK_END);
	if(result == off_t(-1))
	{
		ProcessLastError(status, "Cannot get size of file " + FullPath() + ": ", INTRA_SOURCE_INFO);
		return 0;
	}
	return size_t(result);
#endif
}

size_t OsFile::WriteData(ulong64 fileOffset, const void* src, size_t bytes, ErrorStatus& status) const
{
	INTRA_DEBUG_ASSERT(mHandle != null);
	INTRA_DEBUG_ASSERT(mMode == Mode::Write || mMode == Mode::ReadWrite);
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	size_t totalBytesWritten = 0;
	DWORD bytesWritten;
	OVERLAPPED overlapped{};
	const char* psrc = static_cast<const char*>(src);
	while(bytes > 0xFFFF0000)
	{
		overlapped.Offset = DWORD(fileOffset);
		overlapped.OffsetHigh = DWORD(fileOffset >> 32);
		if(!WriteFile(HANDLE(mHandle), psrc, 0xFFFF0000, &bytesWritten, &overlapped))
			ProcessLastError(status, String::Concat("Cannot write file ", FullPath(), " at offset ", fileOffset, ": "), INTRA_SOURCE_INFO);
		fileOffset += bytesWritten;
		totalBytesWritten += bytesWritten;
		bytes -= bytesWritten;
		psrc += bytesWritten;
		if(bytesWritten < 0xFFFF0000) return totalBytesWritten;
	}
	overlapped.Offset = DWORD(fileOffset);
	overlapped.OffsetHigh = DWORD(fileOffset >> 32);
	if(!WriteFile(HANDLE(mHandle), psrc, DWORD(bytes), &bytesWritten, &overlapped))
		ProcessLastError(status, String::Concat("Cannot write file ", FullPath(), " at offset ", fileOffset, ": "), INTRA_SOURCE_INFO);
#else
	auto bytesWritten = pwrite(int(size_t(mHandle)), src, bytes, off_t(fileOffset));
	if(bytesWritten < 0)
	{
		bytesWritten = 0;
		ProcessLastError(status, String::Concat("Cannot write file ", FullPath(), " at offset ", fileOffset, ": "), INTRA_SOURCE_INFO);
	}
#endif
	return size_t(bytesWritten);
}

void OsFile::SetSize(ulong64 size, ErrorStatus& status) const
{
	INTRA_DEBUG_ASSERT(mHandle != null);
	INTRA_DEBUG_ASSERT(mMode == Mode::Write || mMode == Mode::ReadWrite);
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	LARGE_INTEGER largeSize;
	largeSize.QuadPart = long64(size);
	SetFilePointerEx(HANDLE(mHandle), largeSize, null, FILE_BEGIN);
	if(!SetEndOfFile(HANDLE(mHandle)))
		ProcessLastError(status, String::Concat("Cannot set new size ", size, " of file", FullPath(), ": "), INTRA_SOURCE_INFO);
#else
	if(ftruncate(int(size_t(mHandle)), off_t(size)) != 0)
		ProcessLastError(status, String::Concat("Cannot set new size ", size, " of file", FullPath(), ": "), INTRA_SOURCE_INFO);
#endif
}



String OsFile::ReadAsString(StringView fileName, ErrorStatus& status)
{
	OsFile file(fileName, Mode::Read, status);
	if(file == null) return null;
	if(file.Size() != 0) return FileReader(SharedMove(file));

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	return null;
#else
	//В Unix-подобных системах файлом могут являться устройства и другие объекты, размер которых заранее неизвестен.
	//В этом случае мы попадём сюда. Будем читать файл блоками, пока он не закончится.
	int fd = int(size_t(file.mHandle));
	size_t pos = 0;
	String result;
	result.SetLengthUninitialized(4096);
	for(;;)
	{
		pos += size_t(read(fd, result.Data()+pos, result.Length() - pos));
		if(pos == result.Length())
		{
			result.SetLengthUninitialized(pos + pos/2);
			continue;
		}
		result.SetLengthUninitialized(pos);
		result.TrimExcessCapacity();
		return result;
	}
#endif
}

OsFile OsFile::FromNative(NativeHandle handle, bool owning)
{
	OsFile result;
	result.mHandle = handle;
	result.mMode = Mode::ReadWrite; //Некоторые проверки отключатся, на функциональность влиять не должно
	result.mOwning = owning;
	return result;
}


}}

INTRA_WARNING_POP
