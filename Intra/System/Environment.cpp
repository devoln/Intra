#include "System/Environment.h"

#include "Cpp/Warnings.h"
#include "Cpp/PlatformDetect.h"
#include "Cpp/Fundamental.h"

#include "Utils/AsciiSet.h"
#include "Utils/FixedArray.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Android)

namespace Intra { namespace System {

static CSpan<StringView> getAndParseCommandLine()
{
	static const StringView result[] = {"program"};
	return result;
}

}

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

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

namespace Intra { namespace System {

static CSpan<StringView> getAndParseCommandLine()
{
	static FixedArray<char> buf;
	INTRA_DEBUG_ASSERT(buf.Empty() && "This function must be called only once!");

	const wchar_t* const wcmdline = GetCommandLineW();
	const wchar_t* wcmdptr = wcmdline;
	size_t len = 0;
	while(*wcmdptr++) len++;

	const size_t maxArgCount = (len + 1) / 2;
	buf.SetCount(maxArgCount*sizeof(StringView) + len*3);
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
			dstPtr += WideCharToMultiByte(CP_UTF8, 0,
				wargStart, int(wcmdptr-1-wargStart), dstPtr, int(len), null, null);
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
				if(wendPtr>wargStart) dstPtr += WideCharToMultiByte(CP_UTF8, 0,
					wargStart, int(wendPtr - wargStart), dstPtr, int(len), null, null);
				argOutput.Put(StringView(argStart, dstPtr));
				*dstPtr++ = '\0';
				argStart = dstPtr;
			}
			state = InSpace;
			break;
		default:
			if(state == InSpace) wargStart = wcmdptr - 1;
			state = InText;
		}
	}
	if(state == InText)
	{
		auto wendPtr = wcmdptr;
		if(wendPtr > wargStart && wendPtr[-1] == '"') --wendPtr;
		if(wendPtr > wargStart) dstPtr += WideCharToMultiByte(CP_UTF8, 0,
			wargStart, int(wendPtr - wargStart), dstPtr, int(len), null, null);
		argOutput.Put(StringView(argStart, dstPtr));
		*dstPtr++ = '\0';
		argStart = dstPtr;
	}
	return argBuf.DropLast(argOutput.Length());
}

TEnvironment::VarSet TEnvironment::Variables() const
{
	const wchar_t* const wenv = GetEnvironmentStringsW();

	size_t totalLen = 0;
	size_t num = 0;
	for(auto wenvPtr = wenv; *wenvPtr;)
	{
		totalLen += size_t(WideCharToMultiByte(CP_UTF8, 0, wenvPtr, -1, null, 0, null, null)) - 1;
		while(*wenvPtr++) {}
		wenvPtr++;
		num++;
	}

	FixedArray<char> buffer(num * sizeof(KeyValuePair<StringView, StringView>) + totalLen);

	Span<char> dstChars = SpanOf(buffer).Drop(num * sizeof(KeyValuePair<StringView, StringView>));
	auto dstPairs = SpanOfRaw<KeyValuePair<StringView, StringView>>(
		buffer.Data(), num * sizeof(KeyValuePair<StringView, StringView>));
	for(auto wenvPtr = wenv; *wenvPtr;)
	{
		const size_t len = size_t(WideCharToMultiByte(CP_UTF8, 0, wenvPtr, -1, dstChars.Begin, int(dstChars.Length()), null, null)) - 1;
		const StringView key = dstChars.FindBefore('=');
		const StringView value = dstChars(key.Length() + 1, len);
		dstChars.PopFirstExactly(len);
		dstPairs.Put({key, value});

		while(*wenvPtr++) {}
		wenvPtr++;
	}
	return {Cpp::Move(buffer), num};
}

String TEnvironment::Get(StringView var, bool* oExists) const
{
	FixedArray<wchar_t> buffer(32767 + var.Length() + 1);
	const size_t wvarLen = size_t(MultiByteToWideChar(CP_UTF8, 0, var.Data(), int(var.Length()), buffer.Data(), int(var.Length())));
	Span<wchar_t> wvar = Range::Take(buffer.Data(), wvarLen + 1);
	wvar.Last() = L'\0';
	const size_t wresultLen = GetEnvironmentVariableW(wvar.Data(), buffer.Data() + wvarLen + 1, 32767);
	if(wresultLen == 0)
	{
		if(oExists) *oExists = GetLastError() == ERROR_ENVVAR_NOT_FOUND;
		return null;
	}
	if(oExists) *oExists = true;

	size_t resultLenLimit = wresultLen;
	for(wchar_t c: wvar) if(c & ~0x7F) resultLenLimit += 2;

	String result;
	result.SetLengthUninitialized(resultLenLimit);
	const size_t resultLen = size_t(WideCharToMultiByte(CP_UTF8, 0,
		buffer.Data() + wvarLen + 1, int(wresultLen),
		result.Data(), int(result.Length()), null, null));
	result.SetLengthUninitialized(resultLen);
	return result;
}

}}

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

static CSpan<StringView> getAndParseCommandLine()
{
	static const StringView result[] = {"program"};
	return result;
}

#elif(defined(INTRA_PLATFORM_IS_UNIX))

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace Intra { namespace System {

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
		const size_t len = cmdlineBuf.Length();
		cmdlineBuf.SetCount(len + len/2 + 4096);
		bytesRead += size_t(read(fd, cmdlineBuf.Data() + len, cmdlineBuf.Length() - len));
	}
	while(bytesRead == cmdlineBuf.Length());
	cmdlineBuf.SetCount(bytesRead);
	if(bytesRead == 0) return null;

	size_t argc = 0;
	for(char c: cmdlineBuf) if(c == '\0') argc++;

	static FixedArray<StringView> argv(argc);
	argv[0] = StringView(cmdlineBuf.Data());
	for(size_t i=1; i<argc; i++)
		argv[i] = StringView(argv[i - 1].end() + 1);
	return argv.AsConstRange();
}

}}
#endif

#if(INTRA_PLATFORM_OS != INTRA_PLATFORM_OS_Windows)

#include <cstdlib>

extern char** environ;

namespace Intra { namespace System {

String TEnvironment::Get(StringView var, bool* oExists) const
{
	auto result = getenv(String(var).CStr());
	if(oExists) *oExists = result != null;
	return String(result);
}

TEnvironment::VarSet TEnvironment::Variables() const
{
	size_t num = 0;
	while(*environ[num]) num++;

	FixedArray<char> buffer(num * sizeof(KeyValuePair<StringView, StringView>));

	auto dstPairs = SpanOfRaw<KeyValuePair<StringView, StringView>>(
		buffer.Data(), num * sizeof(KeyValuePair<StringView, StringView>));
	for(size_t i=0; i<num; i++)
	{
		const StringView envStr = StringView(environ[i]);
		const StringView key = envStr.FindBefore('=');
		const StringView value = envStr.Drop(key.Length() + 1);
		dstPairs.Put({key, value});
	}
	return {Cpp::Move(buffer), num};
}

}}

#endif

namespace Intra { namespace System {

CSpan<const KeyValuePair<StringView, StringView>> TEnvironment::VarSet::AsRange() const
{
	return SpanOfRaw<KeyValuePair<StringView, StringView>>(
		mData.Data(), mCount*sizeof(KeyValuePair<StringView, StringView>));
}

TEnvironment::TEnvironment(): CommandLine(getAndParseCommandLine()) {}

const TEnvironment Environment;

}}

INTRA_WARNING_POP
