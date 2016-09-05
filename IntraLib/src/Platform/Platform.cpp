#include "Core/Core.h"
#include "Containers/Array.h"
#include "Containers/String.h"

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
#include <windows.h>
#include <ShellAPI.h>

#ifdef _MSC_VER
#pragma comment(lib, "Shell32.lib")
#endif

namespace Intra {

Array<String> GetCommandLineArguments()
{
	auto cmd = GetCommandLineW();
	int argc=0;
	const wchar** argvW = (const wchar**)CommandLineToArgvW(cmd, &argc);
	Array<String> args((uint)argc);
	for(int i=0; i<argc; i++)
	{
		UTF16 utf16range(argvW[i], argvW[i]+wcslen((const wchar_t*)argvW[i]));
		String str = utf16range.ToUTF8();
		args.AddLast(str);
	}
	return args;
}

}

#ifndef INTRA_NO_WINDOWS_MAIN
extern "C" int __cdecl main(int argc, const char* argv[]);
int WINAPI WinMain(__in HINSTANCE, __in_opt HINSTANCE, __in LPSTR, __in int)
{
	using namespace Intra;
	Array<String> args = GetCommandLineArguments();
	Array<const char*> argv(args.Count());
	for(::size_t i=0; i<args.Count(); i++) argv.AddLast(args[i].CStr());
	if(argv==null) argv.AddLast("");
	return main((int)args.Count(), &argv[0]);
}
#endif

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

extern "C" int __cxa_thread_atexit(void(*func)(), void* obj, void* dso_symbol)
{
	return 0;
}

namespace Intra {

Array<String> GetCommandLineArguments()
{
	return Array<String>({"program"});
}

}

#endif
