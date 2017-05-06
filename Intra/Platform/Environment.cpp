#include "Platform/Environment.h"
#include "Cpp/Warnings.h"
#include "Cpp/PlatformDetect.h"
#include "Cpp/Fundamental.h"
#include "IO/OsFile.h"
#include "Utils/AsciiSet.h"
#include "Utils/FixedArray.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)

namespace Intra {

CSpan<StringView> GetCommandLineArguments()
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

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace Intra {

static CSpan<StringView> getAndParseCommandLine()
{
	const wchar_t* const wcmdline = GetCommandLineW();
	const wchar_t* wcmdptr = wcmdline;
	size_t len = 0;
	while(*wcmdptr++) len++;

	const size_t maxArgCount = (len+1)/2;
	static FixedArray<char> buf(maxArgCount*sizeof(StringView) + len*3);
	const Span<StringView> argBuf = {reinterpret_cast<StringView*>(buf.Data()), maxArgCount};
	const Span<char> charBuf = SpanOf(buf).Drop(maxArgCount*sizeof(StringView));

	Span<StringView> argOutput = argBuf;

	char* dstPtr = charBuf.Data();
	const char* argStart = dstPtr;

	enum State {InQuotes, InText, InSpace};
	State state = InSpace;
	wcmdptr = wcmdline;
	const wchar_t* wargStart = wcmdptr;
	while(*wcmdptr)
	{
		const wchar_t a = *wcmdptr++;
		if(state == InQuotes)
		{
			if(a != '"') continue;
			dstPtr += WideCharToMultiByte(CP_UTF8, 0, wargStart, int(wcmdptr-1-wargStart), dstPtr, int(len), null, null);
			wargStart = wcmdptr;
			if(*wcmdptr == '"')
			{
				wargStart = ++wcmdptr;
				*dstPtr++ = '"';
				continue;
			}
			state = InText;
			continue;
		}

		switch(a)
		{
		case '\"':
			if(state == InSpace) wargStart = wcmdptr;
			state = InQuotes;
			break;
		case ' ': case '\t': case '\n': case '\r':
			if(state == InText)
			{
				auto wendPtr = wcmdptr-1;
				if(wendPtr>wargStart && wendPtr[-1]=='"') --wendPtr;
				if(wendPtr>wargStart) dstPtr += WideCharToMultiByte(CP_UTF8, 0, wargStart, int(wendPtr-wargStart), dstPtr, int(len), null, null);
				argOutput.Put(StringView(argStart, dstPtr));
				*dstPtr++ = '\0';
			}
			state = InSpace;
			break;
		default:
			if(state == InSpace) wargStart = wcmdptr-1;
			state = InText;
		}
	}
	if(state==InText)
	{
		auto wendPtr = wcmdptr-1;
		if(wendPtr>wargStart && wendPtr[-1]=='"') --wendPtr;
		if(wendPtr>wargStart) dstPtr += WideCharToMultiByte(CP_UTF8, 0, wargStart, int(wendPtr-wargStart), dstPtr, int(len), null, null);
		argOutput.Put(StringView(argStart, dstPtr));
		*dstPtr++ = '\0';
	}
	return argBuf.Take(argOutput.Length());
}

const CSpan<StringView> CommandLineArguments = getAndParseCommandLine();

}

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

namespace Intra {

static const StringView commandLineArgs[] = {{"program"}};
const CSpan<StringView> CommandLineArguments = commandLineArgs;

}

#elif(defined(INTRA_PLATFORM_IS_UNIX))
namespace Intra {

static CSpan<StringView> getAndParseCommandLine()
{
	int fd = open(
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_FreeBSD)
		"/proc/curproc/cmdline"
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux)
		"/proc/self/cmdline"
#endif
		, O_RDONLY);
	static FixedArray<char> cmdlineBuf;
	size_t bytesRead = 0;
	do
	{
		cmdlineBuf.SetCount(cmdlineBuf.Length()+cmdlineBuf.Length()/2+4096);
		bytesRead += size_t(read(fd, cmdlineBuf.Data()+len, cmdlineBuf.Length()-len));
	}
	while(bytesRead == cmdlineBuf.Length());
	cmdlineBuf.SetCount(bytesRead);
	if(bytesRead == 0) return null;

	size_t argc = 0;
	for(char c: cmdlineBuf) if(c == '\0') argc++;

	static FixedArray<StringView> argv(argc);
	argv[0] = StringView(cmdlineBuf.Data());
	for(size_t i=1; i<argc; i++) argv[i] = StringView(argv[i-1].end()+1);
	return argv.AsConstRange();
}

const CSpan<StringView> CommandLineArguments = getAndParseCommandLine();

}
#endif

INTRA_WARNING_POP
