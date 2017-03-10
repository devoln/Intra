#include "Platform/Environment.h"
#include "Platform/CppWarnings.h"
#include "Platform/PlatformInfo.h"
#include "Container/Sequential/Array.h"
#include "Container/Sequential/String.h"
#include "IO/OsFile.h"
#include "Range/Special/Unicode.h"
#include "Range/Decorators/Split.h"
#include "Utils/AsciiSet.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)

namespace Intra {

ArrayRange<const StringView> GetCommandLineArguments()
{
	StringView result[] = {"program"};
	return result;
}

}

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <Windows.h>
#include <ShellAPI.h>

#ifdef _MSC_VER
#pragma comment(lib, "Shell32.lib")
#pragma warning(pop)
#endif

namespace Intra {

ArrayRange<const StringView> GetCommandLineArguments()
{
	auto cmd = GetCommandLineW();
	int argc = 0;
	wchar** const argvW = reinterpret_cast<wchar**>(CommandLineToArgvW(cmd, &argc));
	static Array<String> args(static_cast<size_t>(argc));
	static Array<StringView> result(static_cast<size_t>(argc));
	if(!result.Empty()) return result;
	for(int i=0; i<argc; i++)
	{
		UTF16 utf16range(argvW[i], argvW[i]+wcslen(reinterpret_cast<const wchar_t*>(argvW[i])));
		args.AddLast(utf16range.ToUTF8());
		result.AddLast(args.Last());
	}
	return result;
}

}

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

namespace Intra {

ArrayRange<const StringView> GetCommandLineArguments()
{
	static const StringView args[] = {{"program"}};
	return args;
}

}

#elif(defined(INTRA_PLATFORM_IS_UNIX))
namespace Intra {

ArrayRange<const StringView> GetCommandLineArguments()
{
	static const String cmdline = IO::OsFile::ReadAsString(
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_FreeBSD)
"/proc/curproc/cmdline"
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Linux)
"/proc/self/cmdline"
#endif
	);
	static const Array<StringView> args = Range::Split(cmdline, AsciiSet("\0"));
	return args;
}

}
#endif

INTRA_WARNING_POP
