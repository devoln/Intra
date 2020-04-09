#ifdef __ANDROID__

// Wrapper that provides the ability to define main function on Android the same way as on any other platform.

struct android_app;

INTRA_BEGIN
android_app* gGlobalAndroidApp = null;
INTRA_END

extern "C" int main(int argc, const char* argv[]);
void android_main(struct android_app* state)
{
	Intra::gGlobalAndroidApp = state;
	const char* argv[] = {"program"};
	main(1, &argv[0]);
}

#elif defined(_WIN32)

// Wrapper that provides the ability to define main function on Windows the same way as console application or as main on any other platform.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif

#include <Windows.h>
#include <ShellAPI.h>

#ifdef _MSC_VER
#pragma comment(lib, "Shell32.lib")
#endif

#ifndef INTRA_CRTDECL
#ifdef _MSC_VER
#define INTRA_CRTDECL __cdecl
#else
#define INTRA_CRTDECL
#endif
#endif

extern "C" int INTRA_CRTDECL main(int argc, const char* argv[]);

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR cmdline, int)
{
	const char* cmdptr = cmdline;
	size_t len = 0;
	while(*cmdptr++) len++;

	const size_t argumentsStart = (len/2 + 1)*sizeof(size_t);

	static struct Buf
	{
		char* data;
		~Buf() {delete[] data;}
	} buf = {new char[argumentsStart + len + 2]};

	const char** const argv = reinterpret_cast<const char**>(buf.data);
	char* dstptr = buf.data + argumentsStart;
	argv[0] = dstptr;

	int argc = 0;
	bool inQm = false, inText = false, inSpace = true;
	cmdptr = cmdline;
	while(*cmdptr)
	{
		const char a = *cmdptr++;
		if(inQm)
		{
			if(a == '"') inQm = false;
			else *dstptr++ = a;
			continue;
		}

		switch(a)
		{
		case '\"':
			inQm = true;
			inText = true;
			if(inSpace) argv[argc++] = dstptr;
			inSpace = false;
			break;
		case ' ': case '\t': case '\n': case '\r':
			if(inText)* dstptr++ = '\0';
			inText = false;
			inSpace = true;
			break;
		default:
			inText = true;
			if(inSpace) argv[argc++] = dstptr;
			*dstptr++ = a;
			inSpace = false;
		}
	}
	*dstptr++ = '\0';

	return main(argc, argv);
}
#endif
