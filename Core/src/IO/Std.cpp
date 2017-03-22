#include "IO/Std.h"
#include "IO/ConsoleInput.h"
#include "IO/ConsoleOutput.h"
#include "Platform/PlatformInfo.h"

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

static OsFile::NativeHandle* StdInHandle()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	return reinterpret_cast<OsFile::NativeHandle*>(GetStdHandle(STD_INPUT_HANDLE));
#else
	return reinterpret_cast<OsFile::NativeHandle*>(size_t(STDIN_FILENO));
#endif
}

static OsFile::NativeHandle* StdOutHandle()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	return reinterpret_cast<OsFile::NativeHandle*>(GetStdHandle(STD_OUTPUT_HANDLE));
#else
	return reinterpret_cast<OsFile::NativeHandle*>(size_t(STDOUT_FILENO));
#endif
}

StdInOut::StdInOut(ConstructOnce)
{
	if(StdOutIsConsole()) FormattedWriter::operator=(ConsoleOutput());
	else
	{
		mOutputFile = OsFile::FromNative(StdOutHandle(), false);
		FormattedWriter::operator=(FormattedWriter(mOutputFile.Writer()));
	}

	if(StdInIsConsole()) InputStream::operator=(ConsoleIn);
	else
	{
		mInputFile = OsFile::FromNative(StdInHandle(), false);
		InputStream::operator=(mInputFile.Reader());
	}
}

struct StdInOut::ConstructOnce {};
StdInOut Std(StdInOut::ConstructOnce{});

}}
