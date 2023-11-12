#include "IntraX/System/Environment.h"

#include "Intra/Container/AsciiSet.h"
#include "IntraX/Utils/FixedArray.h"

#ifdef __ANDROID__

namespace Intra { INTRA_BEGIN
static Span<const StringView> getAndParseCommandLine()
{
	static const StringView result[] = {"program"};
	return result;
}
} INTRA_END

#elif defined(_WIN32)

INTRA_PUSH_DISABLE_ALL_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
INTRA_WARNING_POP

namespace Intra { INTRA_BEGIN
extern "C" int __wgetmainargs(int* argc, wchar_t*** argv, wchar_t*** env, int doWildCard, void* startInfo);
static Span<const StringView> getAndParseCommandLine()
{
	INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
	static char buf[65536];
#if 0
	const wchar_t* const wcmdline = GetCommandLineW();
	const index_t len = wcslen(wcmdline);

	const auto maxArgCount = index_t(size_t(len + 1) / 2);
	const Span<StringView> argBuf = SpanOfPtr(reinterpret_cast<StringView*>(buf), maxArgCount);
	const Span<char> charBuf = SpanOf(buf).Drop(maxArgCount*index_t(sizeof(StringView)));

	Span<StringView> argOutput = argBuf;

	char* dstPtr = charBuf.Data();
	const char* argStart = dstPtr;

	enum State {InQuotes, InText, InSpace};
	State state = InSpace;
	const wchar_t* wcmdptr = wcmdline;
	const wchar_t* wargStart = wcmdptr;
	while(*wcmdptr)
	{
		const wchar_t a = *wcmdptr++;
		if(state == InQuotes)
		{
			if(a != '"') continue;
			dstPtr += WideCharToMultiByte(CP_UTF8, 0,
				wargStart, int(wcmdptr - 1 - wargStart), dstPtr, int(charBuf.end() - dstPtr), nullptr, nullptr);
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
					wargStart, int(wendPtr - wargStart), dstPtr, int(len), nullptr, nullptr);
				argOutput.Put(StringView::FromPointerRange(argStart, dstPtr));
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
			wargStart, int(wendPtr - wargStart), dstPtr, int(len), nullptr, nullptr);
		argOutput.Put(StringView::FromPointerRange(argStart, dstPtr));
		*dstPtr++ = '\0';
		argStart = dstPtr;
	}
	return argBuf.DropLast(argOutput.Length());
#else
	int argc = 0;
	wchar_t** wargv = nullptr;
	wchar_t** wenv = nullptr;
	__wgetmainargs(&argc, &wargv, &wenv, false, nullptr);
	const Span<char> charBuf = SpanOf(buf).Drop(argc * index_t(sizeof(StringView)));
	const Span<StringView> res = SpanOfPtr(reinterpret_cast<StringView*>(buf), argc);
	for(int i = 0; i < __argc; i++)
	{
		const auto len = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, charBuf.Begin, int(Length(charBuf)), nullptr, nullptr) - 1;
		res[i] = StringView(charBuf.Begin, len);
		charBuf.Begin += len;
	}
	return res;
#endif
}

TEnvironment::VarSet TEnvironment::Variables() const
{
	const wchar_t* const wenv = GetEnvironmentStringsW();

	index_t totalLen = 0;
	index_t num = 0;
	for(auto wenvPtr = wenv; *wenvPtr;)
	{
		totalLen += size_t(WideCharToMultiByte(CP_UTF8, 0, wenvPtr, -1, nullptr, 0, nullptr, nullptr)) - 1;
		wenvPtr+= wcslen(wenvPtr) + 1;
		num++;
	}

	FixedArray<char> buffer(num * index_t(sizeof(Tuple<StringView, StringView>)) + totalLen);

	Span<char> dstChars = SpanOf(buffer).Drop(num * index_t(sizeof(Tuple<StringView, StringView>)));
	auto dstPairs = SpanOfRawElements<Tuple<StringView, StringView>>(buffer.Data(), num);
	for(auto wenvPtr = wenv; *wenvPtr;)
	{
		const auto len = WideCharToMultiByte(CP_UTF8, 0, wenvPtr, -1, dstChars.Begin, int(dstChars.Length()), nullptr, nullptr) - 1;
		const StringView key = dstChars.FindBefore('=');
		const StringView value = dstChars.Drop(key.Length() + 1).Take(len - (key.Length() + 1));
		dstChars.PopFirstExactly(len);
		dstPairs.Put({key, value});

		while(*wenvPtr++) {}
		wenvPtr++;
	}
	FreeEnvironmentStringsW(wenv);
	return {Move(buffer), num};
}

Optional<String> TEnvironment::operator[](StringView var) const
{
	FixedArray<wchar_t> buffer(32767 + var.Length() + 1);
	const auto wvarLen = MultiByteToWideChar(CP_UTF8, 0, var.Data(), int(var.Length()), buffer.Data(), int(var.Length()));
	Span<wchar_t> wvar = SpanOfPtr(buffer.Data(), wvarLen + 1);
	wvar.Last() = L'\0';
	const auto wresultLen = index_t(GetEnvironmentVariableW(wvar.Data(), buffer.Data() + wvarLen + 1, 32767));
	if(wresultLen == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND) return nullptr;

	index_t resultLenLimit = wresultLen;
	for(wchar_t c: wvar) if(c & ~0x7F) resultLenLimit += 2;

	String result;
	result.SetLengthUninitialized(resultLenLimit);
	const index_t resultLen = WideCharToMultiByte(CP_UTF8, 0,
		buffer.Data() + wvarLen + 1, int(wresultLen),
		result.Data(), int(result.Length()), nullptr, nullptr);
	result.SetLengthUninitialized(resultLen);
	return result;
}
} INTRA_END

#elif defined(__EMSCRIPTEN__)

namespace Intra { INTRA_BEGIN
static Span<const StringView> getAndParseCommandLine()
{
	static const StringView result[] = {"program"};
	return result;
}
} INTRA_END

#elif defined(__unix__)

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __linux__
#define _GNU_SOURCE
#include <features.h>
#ifndef __USE_GNU
#define INTRA_IS_MUSL_BUILD
#endif
#undef _GNU_SOURCE
#endif

namespace Intra { INTRA_BEGIN

static FixedArray<StringView> gArgs;
static __attribute__((constructor)) void initCmdlineArgs(
#ifndef INTRA_IS_MUSL_BUILD
	int argc, char** argv
#endif
)
{
#ifdef INTRA_IS_MUSL_BUILD
	int fd = open(
		TargetOS == OperatingSystem::Linux? "/proc/self/cmdline":
		"/proc/curproc/cmdline",
		O_RDONLY);
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
	if(bytesRead == 0) return nullptr;

	size_t argc = 0;
	for(char c: cmdlineBuf) if(c == '\0') argc++;
	gArgs.SetCount(argc);
	argv[0] = StringView(cmdlineBuf.Data());
	for(size_t i = 1; i < argc; i++)
		argv[i] = StringView(argv[i - 1].end() + 1);
#else
	gArgs.SetCount(argc);
	for(size_t i = 0; i < argc; i++)
		gArgs[i] = StringView(argv[i]);
#endif
}

static Span<const StringView> getAndParseCommandLine() {return gArgs;}
} INTRA_END
#endif

#ifndef _WIN32

#include <cstdlib>

extern char** environ;

namespace Intra { INTRA_BEGIN
Optional<String> TEnvironment::operator[](StringView var) const
{
	auto result = getenv(String(var).CStr());
	if(result) return String(result);
	return {};
}

TEnvironment::VarSet TEnvironment::Variables() const
{
	size_t num = 0;
	while(*environ[num]) num++;

	FixedArray<char> buffer(num * sizeof(KeyValuePair<StringView, StringView>));

	auto dstPairs = SpanOfRaw<KeyValuePair<StringView, StringView>>(
		buffer.Data(), num * sizeof(KeyValuePair<StringView, StringView>));
	for(size_t i = 0; i < num; i++)
	{
		const StringView envStr = StringView(environ[i]);
		const StringView key = envStr.FindBefore('=');
		const StringView value = envStr.Drop(key.Length() + 1);
		dstPairs.Put({key, value});
	}
	return {Move(buffer), num};
}
} INTRA_END
#endif

namespace Intra { INTRA_BEGIN
Span<const const Tuple<StringView, StringView>> TEnvironment::VarSet::AsRange() const
{
	return SpanOfRaw<Tuple<StringView, StringView>>(
		mData.Data(), size_t(mCount)*sizeof(Tuple<StringView, StringView>));
}

TEnvironment::TEnvironment(): CommandLine(getAndParseCommandLine()) {}

INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
const TEnvironment Environment;
} INTRA_END
