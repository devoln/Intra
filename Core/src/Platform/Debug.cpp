#include "Platform/Debug.h"
#include "Platform/CppWarnings.h"
#include "IO/LogSystem.h"
#include "Platform/MessageBox.h"
#include "Algo/String/CStr.h"
#include "Range/Stream/Operators.h"
#include "Range/Decorators/Split.h"
#include "Range/Decorators/Map.h"
#include "Algo/Search/Trim.h"
#include "Utils/AsciiSet.h"
#include "IO/Std.h"

INTRA_DISABLE_REDUNDANT_WARNINGS
#include <cstdlib>

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
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

#endif

namespace Intra {

void PrintDebugMessage(StringView message)
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	LPWSTR wmessage = new wchar_t[message.Length()+1];
	int wmessageLength = MultiByteToWideChar(CP_UTF8, 0, message.Data(),
		int(message.Length()), wmessage, int(message.Length()));
	wmessage[wmessageLength] = L'\0';
	OutputDebugStringW(wmessage);
	delete[] wmessage;
#else
	IO::StdErr.PrintLine(message);
#endif
}

void PrintDebugMessage(StringView message, StringView file, int line)
{
	PrintDebugMessage(*String::Format()(file)("(")(line)("): ")(message));
}

bool IsDebuggerAttached()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	return IsDebuggerPresent()!=FALSE;
#else
	return false;
#endif
}

#if(INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Windows || defined(INTRA_DBGHELP))

static bool ContainsMainFunction(StringView sym)
{
	auto tokens = Range::Split(sym, AsciiSet(",.: \t\r\n"));
	auto trimmedTokens = Range::Map(tokens, [](StringView str) {return Algo::Trim(str, '_');});
	return Algo::Contains(trimmedTokens, "main");
}

#endif

}

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

namespace Intra {

String GetStackTrace(size_t framesToSkip, size_t maxFrames, bool untilMain)
{
#ifndef INTRA_DBGHELP
	(void)framesToSkip;
	(void)maxFrames;
	(void)untilMain;
	return null;
#else
	SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_INCLUDE_32BIT_MODULES|SYMOPT_UNDNAME);
	if(!SymInitialize(GetCurrentProcess(), "http://msdl.microsoft.com/download/symbols", true))
		return null;

	if(maxFrames>50) maxFrames = 50;
	void* addrs[50] = {0};
	ushort frames = CaptureStackBackTrace(DWORD(2+framesToSkip), maxFrames, addrs, null);

	String result;
	result.Reserve(2048);

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
			result += sym;
			result += "\n";
			if(untilMain && ContainsMainFunction(sym)) break;
		}
	}

	SymCleanup(GetCurrentProcess());

	return result;
#endif
}

}

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Linux)
#include <execinfo.h>

namespace Intra {

String GetStackTrace(size_t framesToSkip, size_t maxFrames, bool untilMain)
{
	if(framesToSkip>50) return null;
	if(maxFrames+framesToSkip>50) maxFrames = 50-framesToSkip;
	void* pointerArr[50];
	size_t size = size_t(backtrace(pointerArr, int(maxFrames+framesToSkip)));
	char** strings = backtrace_symbols(pointerArr, int(size+framesToSkip));

	String result;
	for(size_t i=framesToSkip; i<size+framesToSkip; i++)
	{
		auto sym = StringView(strings[i]);
		result += sym;
		result += '\n';
		if(untilMain && ContainsMainFunction(sym)) break;
	}

	free(strings);

	return result;
}

}
#else

namespace Intra {

String GetStackTrace(size_t framesToSkip, size_t maxFrames, bool untilMain)
{(void)framesToSkip; (void)maxFrames; (void)untilMain; return null;}

}

#endif

namespace Intra {

String BuildErrorMessage(StringView func, StringView file, int line, StringView info, size_t stackFramesToSkip)
{
	String msg;
	msg.Reserve(100 + file.Length() + func.Length() + info.Length());
	Range::LastAppender(msg) << file << "(" << line << "): "
		"internal error detected in function\n" << func << "\n" << info;
	String stackTrace = GetStackTrace(1+stackFramesToSkip, 50);
	if(stackTrace!=null)
	{
		msg += "\n\nStack trace:\n";
		msg += stackTrace;
	}
	return msg;
}

void InternalErrorMessageAbort(StringView func, StringView file, int line, StringView info)
{
	#if(INTRA_MINEXE>=3)
	(void)func, (void)file, (void)line, (void)info;
	abort();
#else
	//Проверим, что это не рекурсивная ошибка, возникшая при выводе другой ошибки
	static bool was = false;
	if(was) return;
	was = true;

	String msg = BuildErrorMessage(func, file, line, info, 2);

#ifndef INTRA_NO_LOGGING
	if(IO::ErrorLog!=null)
	{
		IO::ErrorLog.PushFont({1,0,0}, 5);
		IO::ErrorLog.PrintLine("Критическая ошибка!");
		IO::ErrorLog.PopFont();
		IO::ErrorLog.PushFont({1,0,0}, 4);
		IO::ErrorLog.PrintLine(msg);
		IO::ErrorLog.PopFont();
	}
#endif

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	PrintDebugMessage(msg);
#endif
	ShowMessageBox(msg, "Критическая ошибка!", MessageIcon::Error);
	exit(1);
#endif
}

void CallInternalErrorCallback(const char* func, const char* file, int line, const char* info)
{gInternalErrorCallback(StringView(func), StringView(file), line, StringView(info));}

void CallInternalErrorCallback(const char* func, const char* file, int line, StringView info)
{gInternalErrorCallback(StringView(func), StringView(file), line, info);}

InternalErrorCallbackType gInternalErrorCallback = InternalErrorMessageAbort;

}

