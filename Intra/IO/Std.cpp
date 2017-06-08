#include "IO/Std.h"
#include "IO/ConsoleInput.h"
#include "IO/ConsoleOutput.h"
#include "IO/FileReader.h"
#include "IO/FileWriter.h"
#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif

#include <io.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <Windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else
#include <unistd.h>
#endif

namespace Intra { namespace IO {

static bool StdInIsConsole()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	DWORD mode;
	return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) != 0;
#else
	return isatty(STDIN_FILENO);
#endif
}

static bool StdOutIsConsole()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	DWORD mode;
	return GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode) != 0;
#else
	return isatty(STDOUT_FILENO);
#endif
}

inline OsFile::NativeHandle StdInHandle()
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	return OsFile::NativeHandle(GetStdHandle(STD_INPUT_HANDLE));
#else
	return OsFile::NativeHandle(size_t(STDIN_FILENO));
#endif
}

inline OsFile::NativeHandle StdOutHandle()
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	return OsFile::NativeHandle(GetStdHandle(STD_OUTPUT_HANDLE));
#else
	return OsFile::NativeHandle(STDOUT_FILENO);
#endif
}

inline OsFile::NativeHandle StdErrHandle()
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	return OsFile::NativeHandle(GetStdHandle(STD_ERROR_HANDLE));
#else
	return OsFile::NativeHandle(STDERR_FILENO);
#endif
}

struct StdInOut::ConstructOnce {};
StdInOut::StdInOut(ConstructOnce)
{
	if(StdOutIsConsole()) FormattedWriter::operator=(ConsoleOutput());
	else
	{
		FileWriter writer(SharedMove(OsFile::FromNative(StdOutHandle(), false)));
		FormattedWriter::operator=(FormattedWriter(Cpp::Move(writer)));
	}

	if(StdInIsConsole()) InputStream::operator=(ConsoleInput());
	else
	{
		FileReader reader(SharedMove(OsFile::FromNative(StdInHandle(), false)));
		InputStream::operator=(Cpp::Move(reader));
	}
}

StdInOut Std(StdInOut::ConstructOnce{});

FormattedWriter StdErr(FileWriter(SharedMove(OsFile::FromNative(StdOutHandle(), false))));

}}

INTRA_WARNING_POP
