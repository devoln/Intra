#include "IO/OsFile.h"

#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"

#include "Container/Sequential/String.h"

#include "IO/FileSystem.h"
#include "IO/FileReader.h"
#include "IO/FileWriter.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(defined(INTRA_PLATFORM_IS_UNIX) || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

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

OsFile::OsFile(StringView fileName, Mode mode, bool disableSystemBuffering, ErrorStatus& status):
	mMode(mode), mOwning(true)
{
	mFullPath = OS.GetFullFileName(fileName);
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	auto wFullFileName = utf8ToWStringZ(mFullPath);

	DWORD desiredAccess = GENERIC_READ;
	if(mode == Mode::Write) desiredAccess = GENERIC_WRITE;
	if(mode == Mode::ReadWrite) desiredAccess |= GENERIC_WRITE;

	DWORD creationDisposition = DWORD(mode == Mode::Read? OPEN_EXISTING: OPEN_ALWAYS);
	DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
	if(disableSystemBuffering) flagsAndAttributes |= FILE_FLAG_NO_BUFFERING;

	HANDLE hFile = CreateFileW(wFullFileName.Data(), desiredAccess,
		FILE_SHARE_READ, null, creationDisposition, flagsAndAttributes, null);

	if(hFile != INVALID_HANDLE_VALUE) mHandle = reinterpret_cast<NativeHandle*>(hFile);
	else
	{
		mHandle = null;
		mOwning = false;

		char* s = null;
		FormatMessageA(DWORD(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS),
			null, GetLastError(),
			DWORD(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)),
			(char*)&s, 0, null);
		status.Error("Cannot open file " + fileName + ": " + StringView(s) + "!", INTRA_SOURCE_INFO);
		LocalFree(s);
	}
#else
	auto openFlags = (mMode == Mode::Read)? O_RDONLY: (mMode == Mode::Write? O_WRONLY: O_RDWR);
	if(mMode != Mode::Read) openFlags |= O_CREAT;
	if(disableSystemBuffering) openFlags |= O_DIRECT;
	int fd = open(mFullPath.CStr(), openFlags, 0664);
	if(fd != -1) mHandle = reinterpret_cast<NativeHandle*>(size_t(fd));
	else
	{
		mHandle = null;
		mOwning = false;
		status.Error("Cannot open file " + fileName + "! errno = " + StringView(strerror(errno)));
	}
#endif
}

void OsFile::Close()
{
	if(mHandle == null) return;
	if(!mOwning) return;
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	CloseHandle(reinterpret_cast<HANDLE>(mHandle));
#else
	close(int(size_t(mHandle)));
#endif
	mOwning = false;
}

size_t OsFile::ReadData(ulong64 fileOffset, void* dst, size_t bytes) const
{
	INTRA_DEBUG_ASSERT(mHandle != null);
	INTRA_DEBUG_ASSERT(mMode == Mode::Read || mMode == Mode::ReadWrite);
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	DWORD bytesRead;
	OVERLAPPED overlapped{};
	overlapped.Offset = DWORD(fileOffset);
	overlapped.OffsetHigh = DWORD(fileOffset >> 32);
	(void)ReadFile(reinterpret_cast<HANDLE>(mHandle), dst, bytes, &bytesRead, &overlapped);
#else
	auto bytesRead = pread64(int(reinterpret_cast<size_t>(mHandle)), dst, bytes, off64_t(fileOffset));
	if(bytesRead < 0) bytesRead = 0;
#endif
	return size_t(bytesRead);
}

ulong64 OsFile::Size() const
{
	if(mHandle == null) return 0;
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	LARGE_INTEGER result;
	result.QuadPart = 0;
	GetFileSizeEx(reinterpret_cast<HANDLE>(mHandle), &result);
	return ulong64(result.QuadPart);
#else
	off64_t result = lseek64(int(reinterpret_cast<size_t>(mHandle)), 0, SEEK_END);
	if(result == off64_t(-1)) return 0;
	return size_t(result);
#endif
}

size_t OsFile::WriteData(ulong64 fileOffset, const void* src, size_t bytes) const
{
	INTRA_DEBUG_ASSERT(mHandle != null);
	INTRA_DEBUG_ASSERT(mMode == Mode::Write || mMode == Mode::ReadWrite);
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	DWORD bytesWritten;
	OVERLAPPED overlapped;
	overlapped.hEvent = null;
	overlapped.Internal = overlapped.InternalHigh = 0;
	overlapped.Offset = DWORD(fileOffset);
	overlapped.OffsetHigh = DWORD(fileOffset >> 32);
	WriteFile(reinterpret_cast<HANDLE>(mHandle), src, bytes, &bytesWritten, &overlapped);
#else
	auto bytesWritten = pwrite64(int(reinterpret_cast<size_t>(mHandle)), src, bytes, off64_t(fileOffset));
	if(bytesWritten < 0) bytesWritten = 0;
#endif
	return size_t(bytesWritten);
}

void OsFile::SetSize(ulong64 size) const
{
	INTRA_DEBUG_ASSERT(mHandle != null);
	INTRA_DEBUG_ASSERT(mMode == Mode::Write || mMode == Mode::ReadWrite);
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	LARGE_INTEGER largeSize;
	largeSize.QuadPart = long64(size);
	SetFilePointerEx(HANDLE(mHandle), largeSize, null, FILE_BEGIN);
	SetEndOfFile(HANDLE(mHandle));
#else
	const bool success = ftruncate64(int(size_t(mHandle)), off64_t(size)) == 0;
	(void)success;
#endif
}



String OsFile::ReadAsString(StringView fileName, ErrorStatus& error)
{
	OsFile file(fileName, Mode::Read, error);
	if(file == null) return null;
	if(file.Size() != 0) return FileReader(SharedMove(file));

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
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
		pos += size_t(read(fd, result.Data()+pos, result.Length()-pos));
		if(pos == result.Length())
		{
			result.SetLengthUninitialized(pos+pos/2);
			continue;
		}
		result.SetLengthUninitialized(pos);
		result.TrimExcessCapacity();
		return result;
	}
#endif
}

OsFile OsFile::FromNative(NativeHandle* handle, bool owning)
{
	OsFile result;
	result.mHandle = handle;
	result.mMode = Mode::ReadWrite; //Некоторые проверки отключатся, на функциональность влиять не должно
	result.mOwning = owning;
	return result;
}


}}

INTRA_WARNING_POP
