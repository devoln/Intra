#include "IO/Std.h"
#include "IO/ConsoleInput.h"
#include "IO/ConsoleOutput.h"
#include "IO/FileReader.h"
#include "IO/FileWriter.h"

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif

INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <io.h>
#include <Windows.h>
INTRA_WARNING_POP

#else
#include <unistd.h>
#endif

INTRA_BEGIN
static bool StdInIsConsole()
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	DWORD mode;
	return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) != 0;
#else
	return isatty(STDIN_FILENO);
#endif
}

static bool StdOutIsConsole()
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
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
	if(StdOutIsConsole()) *static_cast<FormattedWriter*>(this) = ConsoleOutput();
	else
	{
		FileWriter writer(SharedMove(OsFile::FromNative(StdOutHandle(), false)));
		*static_cast<FormattedWriter*>(this) = FormattedWriter(Move(writer));
	}

	if(StdInIsConsole()) *static_cast<InputStream*>(this) = ConsoleInput();
	else
	{
		FileReader reader(SharedMove(OsFile::FromNative(StdInHandle(), false)));
		*static_cast<InputStream*>(this) = Move(reader);
	}
}

INTRA_WARNING_DISABLE_GLOBAL_CONSTRUCTION
StdInOut Std(StdInOut::ConstructOnce{});
FormattedWriter StdErr(FileWriter(SharedMove(OsFile::FromNative(StdOutHandle(), false))));

INTRA_END
