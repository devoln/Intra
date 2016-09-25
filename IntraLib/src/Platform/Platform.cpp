#include "Core/Core.h"
#include "Containers/Array.h"
#include "Containers/String.h"
#include "IO/File.h"

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)


android_app* g_GlobalAndroidApp=null;

extern "C" int main(int argc, const char* argv[]);
void android_main(struct android_app* state)
{
	g_GlobalAndroidApp = state;
	const char* argv[] = {"program"};
	main(1, &argv[0]);
}

namespace Intra {

Array<String> GetCommandLineArguments()
{
	return Array<String>({"program"});
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
	int argc=0;
	wchar** const argvW = reinterpret_cast<wchar**>(CommandLineToArgvW(cmd, &argc));
	static Array<String> args(static_cast<size_t>(argc));
	static Array<StringView> result(static_cast<size_t>(argc));
	if(!result.Empty()) return result;
	for(int i=0; i<argc; i++)
	{
		UTF16 utf16range(argvW[i], argvW[i]+wcslen(reinterpret_cast<const wchar_t*>(argvW[i])));
		String str = utf16range.ToUTF8();
		args.AddLast(str);
		result.AddLast(args.Last());
	}
	return result;
}

}

#ifndef INTRA_NO_WINDOWS_MAIN
extern "C" int INTRA_CRTDECL main(int argc, const char* argv[]);

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	using namespace Intra;
	auto args = GetCommandLineArguments();
	Array<const char*> argv(args.Length());
	for(::size_t i=0; i<args.Length(); i++) argv.AddLast(String(args[i]).CStr());
	if(argv==null) argv.AddLast("");
	return main(int(args.Count()), &argv[0]);
}
#endif

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

extern "C" int __cxa_thread_atexit(void(*func)(), void* obj, void* dsoSymbol)
{
	(void)func; (void)obj; (void)dsoSymbol;
	return 0;
}

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
	static const String cmdline = IO::DiskFile::ReadAsString(
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_FreeBSD)
"/proc/curproc/cmdline"
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Linux)
"/proc/self/cmdline"
#endif
	);
	static const Array<StringView> args = cmdline().Split("\0");
	return args;
}

}
#endif
