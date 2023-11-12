#include "Debug.h"

#include <cstdlib>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER

#if defined(WINAPI_FAMILY_PARTITION) && defined(WINAPI_PARTITION_DESKTOP)
#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) && _WIN32_WINNT >= 0x0602
#define WINSTORE_APP
#endif
#endif

#ifndef WINSTORE_APP
#pragma comment(lib, "user32.lib")
#endif

INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <Windows.h>

#ifndef INTRA_DBGHELP

#else

#ifdef _MSC_VER
#pragma comment(lib, "DbgHelp.lib")
#endif
#include <DbgHelp.h>

#endif
INTRA_WARNING_POP

#else
#include <unistd.h>
#endif
#endif

namespace Intra { INTRA_BEGIN
#ifdef _WIN32
void PrintDebugMessage(StringView message)
{
	wchar_t wmessageBuf[8182]; // avoid dynamic memory allocation (Core module restriction)
	Span<wchar_t> wmessage = Take(wmessageBuf, message.Length()+1);
	const index_t wmessageLength = index_t(MultiByteToWideChar(CP_UTF8, 0,
		message.Data(), int(message.Length()), wmessage.Data(), int(wmessage.Length()-1)));
	wmessage[wmessageLength] = L'\0';
	OutputDebugStringW(wmessage.Data());
}
#else
void PrintDebugMessage(StringView message)
{
	auto written = write(STDERR_FILENO, message.Data(), message.Length());
	(void)written;
}
#endif


static void AppendUInt(Span<char>& dst, unsigned val)
{
	char lineStr[10];
	char* lineStrPtr = lineStr + 10;
	do *--lineStrPtr = char('0' + val % 10), val /= 10;
	while(val != 0);
	while(lineStrPtr != lineStr + 10 && !dst.Full()) dst.Put(*lineStrPtr++);
}

void PrintDebugMessage(StringView message, StringView file, unsigned line)
{
	char messageBuf[8192]; // avoid dynamic memory allocation (Core module restriction)
	const Span<char> buffer = Take(messageBuf, 14 + file.Length() + message.Length());
	Span<char> dst = buffer;
	file.WriteTo(dst);
	dst << '(';
	AppendUInt(dst, line);
	dst << "): " << message;
	PrintDebugMessage(StringView::FromPointerRange(buffer.Data(), dst.Data()));
}

#if((!defined(_WIN32) || defined(INTRA_DBGHELP)) && defined(__EMSCRIPTEN__))

static bool IsSeparatorChar(char c)
{
	const char* sepChars = ",.: \t\r\n()";
	while(*sepChars) if(c == *sepChars++) return true;
	return false;
}

static bool ContainsMainFunction(StringView sym)
{
	const StringView found = sym.Find("main");
	if(found.Empty()) return false;
	if(found.Data() > sym.Data() && !IsSeparatorChar(found.Data()[-1]) && found.Data()[-1] != '_') return false;
	if(found.Length() > 4 && !IsSeparatorChar(found[4])) return false;
	return true;
}

#endif
} INTRA_END

#ifdef _WIN32

namespace Intra { INTRA_BEGIN
StringView GetStackTrace(Span<char>& dst, size_t framesToSkip, size_t maxFrames, bool untilMain)
{
#ifndef INTRA_DBGHELP
	(void)dst;
	(void)framesToSkip;
	(void)maxFrames;
	(void)untilMain;
	return nullptr;
#else
#define INTRA_STACKTRACE_SUPPORTED

	enum {MAX_STACK_FRAMES = 50};

	SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_INCLUDE_32BIT_MODULES|SYMOPT_UNDNAME);
	if(!SymInitialize(GetCurrentProcess(), "http://msdl.microsoft.com/download/symbols", true))
		return nullptr;

	if(maxFrames > MAX_STACK_FRAMES) maxFrames = MAX_STACK_FRAMES;
	void* addrs[MAX_STACK_FRAMES] = {};
	uint16 frames = CaptureStackBackTrace(DWORD(1 + framesToSkip), maxFrames, addrs, nullptr);

	const auto dstStart = dst;
	for(uint16 i = 0; i < frames; i++)
	{
		uint64 buffer[(sizeof(SYMBOL_INFO) + 1024 + sizeof(uint64) - 1) / sizeof(uint64)] = {0};
		SYMBOL_INFO* info = reinterpret_cast<SYMBOL_INFO*>(buffer);
		info->SizeOfStruct = sizeof(SYMBOL_INFO);
		info->MaxNameLen = 1024;

		DWORD64 displacement = 0;
		DWORD options = SymGetOptions();
		DWORD newOptions = options & ~DWORD(SYMOPT_UNDNAME);
		newOptions = newOptions | SYMOPT_PUBLICS_ONLY;
		SymSetOptions(newOptions);
		if(SymFromAddr(GetCurrentProcess(), size_t(addrs[i]), &displacement, info))
		{
			auto len = UnDecorateSymbolName(info->Name, dst.Data(), dst.Length(),
				UNDNAME_NO_ACCESS_SPECIFIERS|UNDNAME_NO_MEMBER_TYPE|UNDNAME_NO_MS_KEYWORDS|
				UNDNAME_NO_THROW_SIGNATURES|UNDNAME_NO_ALLOCATION_MODEL|UNDNAME_NO_ALLOCATION_LANGUAGE|
				UNDNAME_NO_CV_THISTYPE|UNDNAME_NO_LEADING_UNDERSCORES|UNDNAME_NO_THISTYPE);
			auto sym = dst.Take(len);
			dst.PopFirstCount(len);
			dst << '\n';
			if(untilMain && ContainsMainFunction(sym)) break;
		}
	}

	SymCleanup(GetCurrentProcess());

	return StringView::FromPointerRange(dstStart.Data(), dst.Data());
#endif
}
} INTRA_END

#elif defined(__linux__) || defined(__FreeBSD__)
#include <execinfo.h>

#if defined(__GNUC__)// && !defined(__clang__)
#include <cxxabi.h>
static Intra::StringView TryDemangle(Intra::Span<char>& dst, Intra::StringView mangledName)
{
	auto start = dst;
	int status;
	char buf[1024];
	Intra::SpanOfBuffer(buf) << mangledName << '\0';
	auto demangledName = abi::__cxa_demangle(buf, 0, 0, &status);
	if(status == 0)
	{
		dst << Intra::StringView(demangledName);
		free(demangledName);
	}
	else dst << Intra::StringView(mangledName);
	return {start.Data(), dst.Data()};
}
#else
static Intra::StringView TryDemangle(Intra::Span<char>& dst, Intra::StringView mangledName)
{
	auto start = dst;
	dst << Intra::StringView(mangledName);
	return StringView::FromPointerRange(start.Data(), dst.Data());
}
#endif

namespace Intra { INTRA_BEGIN
StringView GetStackTrace(Span<char>& dst, size_t framesToSkip, size_t maxFrames, bool untilMain)
{
#define INTRA_STACKTRACE_SUPPORTED
	framesToSkip++;

	enum {MAX_STACK_FRAMES = 50};
	if(framesToSkip > MAX_STACK_FRAMES) return nullptr;
	if(maxFrames + framesToSkip > MAX_STACK_FRAMES) maxFrames = MAX_STACK_FRAMES - framesToSkip;
	void* pointerArr[MAX_STACK_FRAMES];
	size_t size = size_t(backtrace(pointerArr, Overflow::Saturate(maxFrames + framesToSkip)));
	char** strings = backtrace_symbols(pointerArr, Overflow::Saturate(size + framesToSkip));

	const auto dstStart = dst;
	for(size_t i = framesToSkip; i < size + framesToSkip; i++)
	{
		StringView fullStr(strings[i]);
		StringView moduleName = fullStr.FindBefore('(');
		StringView mangledName = fullStr.Drop(moduleName.Length() + 1).FindBefore(')').FindBefore('+');
		dst << '[' << moduleName << "] ";
		auto sym = TryDemangle(dst, mangledName);
		dst << '\n';
		if(untilMain && ContainsMainFunction(sym)) break;
	}

	free(strings);

	return StringView::FromPointerRange(dstStart.Data(), dst.Data());
}
} INTRA_END
#else

namespace Intra { INTRA_BEGIN
StringView GetStackTrace(Span<char>& dst, size_t framesToSkip, size_t maxFrames, bool untilMain)
{
	(void)framesToSkip; (void)maxFrames; (void)untilMain;
	return dst.TakeNone();
}
} INTRA_END

#endif

namespace Intra { INTRA_BEGIN
namespace z_D {
template<typename... Ts> Span<char> SprintfHelper(Span<char>& dst, const char* format, Ts... args)
{
	const unsigned fullLengthWithoutNullOrMax = unsigned(snprintf(dst.Begin, dst.size(), format, args...)); // some implementations return -1 (MaxValue<unsigned>)
	auto res = dst|Take(fullLengthWithoutNullOrMax);
	dst.Begin += res.Length();
	if(dst.Empty() && !res.Empty() && res.Last() == '\0') res.PopLast();
	return res;
}
}
	
StringView BuildDiagnosticMessage(Span<char>& dst, StringView type, StringView msg, SourceInfo sourceInfo, size_t stackFramesToSkip)
{
	const StringView msg = dst;
	dst << file;
	if(line != 0)
	{
		dst << '(';
		AppendUInt(dst, line);
		dst << ") ";
	}
	dst << type << ' ';
	if(func) dst << "in function\n" << func << ":\n";
	dst << info << '\n';
#ifdef INTRA_STACKTRACE_SUPPORTED
	if(stackFramesToSkip != ~size_t())
	{
		dst << "\nStack trace:\n";
		const StringView stackTrace = GetStackTrace(dst, stackFramesToSkip + 1, 50);
		if(stackTrace.Empty())
			dst << "<Not supported on this platform>\n";
	}
#else
	(void)stackFramesToSkip;
#endif
	return StringView::FromPointerRange(msg.Data(), dst.Data());
}

void FatalErrorMessageAbort(StringView msg, bool printStackTrace, SourceInfo srcInfo)
{
	if constexpr(DisableTroubleshooting)
	{
		abort();
		return;
	}

	static bool alreadyInAbort = false;
	if(alreadyInAbort) return; // prevent recursive call
	alreadyInAbort = true;

	char msgBuffer[8192]; // avoid dynamic memory allocation
	Span<char> msgBuf = SpanOfBuffer(msgBuffer);
	const StringView fullMsg = BuildDiagnosticMessage(msgBuf, "FATAL ERROR",
		StringView(srcInfo.Function), StringView(srcInfo.File), srcInfo.Line, msg, printStackTrace? 2: MaxValueOf<size_t>);

	PrintDebugMessage(fullMsg);

#if(defined(_WIN32) && !defined(WINSTORE_APP))
	wchar_t wbuffer[8192]; // avoid dynamic memory allocation
	const index_t wstrMaxLength = Min(fullMsg.Length(), LengthOf(wbuffer) - 1);
	const index_t wmessageLength = z_D::MultiByteToWideChar(65001, 0, fullMsg.Data(), int(fullMsg.Length()), wbuffer, int(wstrMaxLength));
	wbuffer[wmessageLength] = L'\0';
	MessageBoxW(nullptr, wbuffer, L"Fatal error", MB_ICONERROR);
#endif

	exit(1);
}

static void FatalErrorMessageAbort(DebugStringView msg, SourceInfo srcInfo)
{
	FatalErrorMessageAbort(msg, true, srcInfo);
}

#if INTRA_MINEXE < 3
namespace z_D {
INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
static char initAbortCallback =	(FatalErrorCallback() = FatalErrorMessageAbort, 0);
}
#endif
} INTRA_END
