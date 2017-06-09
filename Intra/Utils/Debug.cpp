#include "Debug.h"
#include "Cpp/Warnings.h"
#include "Cpp/Intrinsics.h"
#include "StringView.h"
#include "Span.h"
#include "FixedArray.h"

INTRA_DISABLE_REDUNDANT_WARNINGS
#include <cstdlib>

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <windows.h>

#ifndef INTRA_DBGHELP

#else

#ifdef _MSC_VER
#pragma warning(disable: 4091)
#pragma comment(lib, "DbgHelp.lib")
#endif
#include <DbgHelp.h>

#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else
#include <unistd.h>
#endif

namespace Intra { namespace Utils {

void PrintDebugMessage(StringView message)
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	FixedArray<wchar_t> wmessage(message.Length()+1);
	const size_t wmessageLength = size_t(MultiByteToWideChar(CP_UTF8, 0,
		message.Data(), int(message.Length()), wmessage.Data(), int(message.Length())));
	wmessage[wmessageLength] = L'\0';
	OutputDebugStringW(wmessage.Data());
#else
	auto written = write(STDERR_FILENO, message.Data(), message.Length());
	(void)written;
#endif
}

static void AppendUInt(Span<char>& dst, unsigned val)
{
	char lineStr[10];
	char* lineStrPtr = lineStr + 10;
	do *--lineStrPtr = char('0' + val % 10), val /= 10;
	while(val != 0);
	while(lineStrPtr != lineStr && !dst.Empty()) dst.Put(*lineStrPtr++);
}

void PrintDebugMessage(StringView message, StringView file, unsigned line)
{
	FixedArray<char> buffer(14 + file.Length() + message.Length());
	Span<char> dst = buffer;
	file.CopyToAdvance(dst);
	dst << '(';
	AppendUInt(dst, line);
	dst << "): " << message;
	PrintDebugMessage({buffer.Data(), dst.Data()});
}

bool IsDebuggerAttached()
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	return IsDebuggerPresent() != FALSE;
#else
	return false;
#endif
}

#if(INTRA_PLATFORM_OS != INTRA_PLATFORM_OS_Windows || defined(INTRA_DBGHELP))

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
	if(found.Data() > sym.Data() && !IsSeparatorChar(found.Data()[-1])) return false;
	if(found.Length() > 4 && !IsSeparatorChar(found[4])) return false;
	return true;
}

#endif

}}

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

namespace Intra { namespace Utils {

StringView GetStackTrace(Span<char>& dst, size_t framesToSkip, size_t maxFrames, bool untilMain)
{
#ifndef INTRA_DBGHELP
	(void)dst;
	(void)framesToSkip;
	(void)maxFrames;
	(void)untilMain;
	return null;
#else
	enum {MAX_STACK_FRAMES = 50};

	SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_INCLUDE_32BIT_MODULES|SYMOPT_UNDNAME);
	if(!SymInitialize(GetCurrentProcess(), "http://msdl.microsoft.com/download/symbols", true))
		return null;

	if(maxFrames > MAX_STACK_FRAMES) maxFrames = MAX_STACK_FRAMES;
	void* addrs[MAX_STACK_FRAMES] = {0};
	ushort frames = CaptureStackBackTrace(DWORD(2 + framesToSkip), maxFrames, addrs, null);

	const auto dstStart = dst;
	for(ushort i=0; i<frames; i++)
	{
		ulong64 buffer[(sizeof(SYMBOL_INFO) + 1024 + sizeof(ulong64) - 1) / sizeof(ulong64)] = {0};
		SYMBOL_INFO* info = reinterpret_cast<SYMBOL_INFO*>(buffer);
		info->SizeOfStruct = sizeof(SYMBOL_INFO);
		info->MaxNameLen = 1024;

		DWORD64 displacement = 0;
		if(SymFromAddr(GetCurrentProcess(), size_t(addrs[i]), &displacement, info))
		{
			auto sym = StringView(info->Name, info->NameLen);
			sym.CopyToAdvance(dst);
			dst << '\n';
			if(untilMain && ContainsMainFunction(sym)) break;
		}
	}

	SymCleanup(GetCurrentProcess());

	return {dstStart.Data(), dst.Data()};
#endif
}

}}

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_FreeBSD)
#include <execinfo.h>

namespace Intra { namespace Utils {

StringView GetStackTrace(Span<char>& dst, size_t framesToSkip, size_t maxFrames, bool untilMain)
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_FreeBSD)
	typedef size_t backtrace_size;
#else
	typedef int backtrace_size;
#endif

	enum {MAX_STACK_FRAMES = 50};
	if(framesToSkip > MAX_STACK_FRAMES) return null;
	if(maxFrames + framesToSkip > MAX_STACK_FRAMES) maxFrames = MAX_STACK_FRAMES - framesToSkip;
	void* pointerArr[MAX_STACK_FRAMES];
	size_t size = size_t(backtrace(pointerArr, backtrace_size(maxFrames + framesToSkip)));
	char** strings = backtrace_symbols(pointerArr, backtrace_size(size + framesToSkip));

	const auto dstStart = dst;
	for(size_t i = framesToSkip; i < size + framesToSkip; i++)
	{
		auto sym = StringView(strings[i]);
		sym.CopyToAdvance(dst);
		dst << '\n';
		if(untilMain && ContainsMainFunction(sym)) break;
	}

	free(strings);

	return {dstStart.Data(), dst.Data()};
}

}}
#else

namespace Intra { namespace Utils {

StringView GetStackTrace(Span<char>& dst, size_t framesToSkip, size_t maxFrames, bool untilMain)
{
	(void)framesToSkip; (void)maxFrames; (void)untilMain;
	return dst.TakeNone();
}

}}

#endif

namespace Intra { namespace Utils {

StringView BuildDiagnosticMessage(Span<char>& dst, StringView type,
	StringView func, StringView file, unsigned line, StringView info, size_t stackFramesToSkip)
{
	const StringView msg = dst;
	dst << file << '(';
	AppendUInt(dst, line);
	dst << ") " << type << ' ';
	if(func != null) dst << "in function\n" << func << ":\n";
	dst << info << '\n';
	if(stackFramesToSkip == ~size_t())
	{
		dst << "\nStack trace:\n";
		const StringView stackTrace = GetStackTrace(dst, 1+stackFramesToSkip, 50);
		if(stackTrace.Empty())
			dst << "<Not supported on this platform>\n";
	}
	return {msg.Data(), dst.Data()};
}

FixedArray<char> BuildAppendDiagnosticMessage(StringView initStr, StringView type,
	StringView func, StringView file, unsigned line, StringView info, size_t stackFramesToSkip)
{
	FixedArray<char> result(initStr.Length() + (1 << 16));
	Span<char> dst = result;
	dst << initStr << '\n';
	BuildDiagnosticMessage(dst, type, func, file, line, info, stackFramesToSkip);
	result.SetCount(result.Length() - dst.Length());
	return result;
}

void FatalErrorMessageAbort(const SourceInfo& srcInfo, StringView msg, bool printStackTrace)
{
#if(INTRA_MINEXE >= 3)
	(void)srcInfo;
	(void)msg;
	(void)printStackTrace;
	abort();
#else
	//Проверим, что это не рекурсивная ошибка, возникшая при выводе другой ошибки
	static bool was = false;
	if(was) return;
	was = true;

	char msgBuffer[16384];
	Span<char> msgBuf = SpanOfBuffer(msgBuffer);
	const StringView fullMsg = BuildDiagnosticMessage(msgBuf, "FATAL ERROR",
		srcInfo.Function, srcInfo.File, srcInfo.Line, msg, printStackTrace? 2: ~size_t());

	PrintDebugMessage(fullMsg);

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	FixedArray<wchar_t> wbuffer(msg.Length() + 1);
	LPWSTR wmessage = wbuffer.Data();
	int wmessageLength = MultiByteToWideChar(CP_UTF8, 0, fullMsg.Data(), int(fullMsg.Length()), wmessage, int(fullMsg.Length()));
	wmessage[wmessageLength] = L'\0';
	MessageBoxW(null, wmessage, L"Fatal error", MB_ICONERROR);
#endif

	exit(1);
#endif
}

void FatalErrorMessageAbort(const SourceInfo& srcInfo, StringView msg)
{
	FatalErrorMessageAbort(srcInfo, msg, true);
}

FatalErrorCallbackType gFatalErrorCallback = FatalErrorMessageAbort;

void CallFatalErrorCallback(const SourceInfo& srcInfo, StringView msg)
{gFatalErrorCallback(srcInfo, msg);}

void CallFatalErrorCallback(const SourceInfo& srcInfo, const char* msg)
{gFatalErrorCallback(srcInfo, msg);}

}}

