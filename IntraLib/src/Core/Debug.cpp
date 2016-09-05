#include "Core/Debug.h"
#include "IO/Stream.h"
#include "IO/LogSystem.h"
#include "GUI/MessageBox.h"

#include <stdlib.h>

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#if(INTRA_MINEXE>=3)

#else
#include <DbgHelp.h>
#ifdef _MSC_VER
#pragma comment(lib, "DbgHelp.lib")
#endif
#endif

#endif

namespace Intra {

void PrintDebugMessage(StringView message)
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	LPWSTR wmessage = new wchar_t[message.Length()+1];
	int wmessageLength = MultiByteToWideChar(CP_UTF8, 0, message.Data(), (int)message.Length(), wmessage, (int)message.Length());
	wmessage[wmessageLength] = L'\0';
	OutputDebugStringW(wmessage);
	delete[] wmessage;
#else
	IO::Console << message << IO::endl;
#endif
}

void PrintDebugMessage(StringView message, StringView file, int line)
{
	PrintDebugMessage(*String::Format()(file)("(")(line)("): ")(message));
}

bool IsDebuggerConnected()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	return IsDebuggerPresent()!=FALSE;
#else
	return false;
#endif
}


#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)


String GetStackWalk(size_t framesToSkip)
{
#if(INTRA_MINEXE>=3)
	(void)framesToSkip;
	return null;
#else
	SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_INCLUDE_32BIT_MODULES|SYMOPT_UNDNAME);
	if(!SymInitialize(GetCurrentProcess(), "http://msdl.microsoft.com/download/symbols", true))
		return null;

	void* addrs[50] = {0};
	ushort frames = CaptureStackBackTrace(DWORD(2+framesToSkip), 50, addrs, null);

	String result;
	result.Reserve(2048);

	for(ushort i=0; i<frames; i++)
	{
		ulong64 buffer[(sizeof(SYMBOL_INFO) + 1024 + sizeof(ulong64) - 1) / sizeof(ulong64)] = {0};
		SYMBOL_INFO* info = (SYMBOL_INFO*)buffer;
		info->SizeOfStruct = sizeof(SYMBOL_INFO);
		info->MaxNameLen = 1024;

		DWORD64 displacement = 0;
		if(SymFromAddr(GetCurrentProcess(), (ulong64)addrs[i], &displacement, info))
		{
			result += StringView(info->Name, info->NameLen);
			result += "\n";
		}
	}

	SymCleanup(GetCurrentProcess());

	return result;
#endif
}

#else
String GetStackWalk() {return null;}
#endif



void InternalError(const char* func, const char* file, int line, const char* info)
{
#if(INTRA_MINEXE>=3)
	(void)func, (void)file, (void)line, (void)info;
	abort();
#else
	//Проверим, что это не рекурсивная ошибка, возникшая при выводе другой ошибки
	static bool was = false;
	if(was) return;
	was = true;

	String msg;
	msg.Reserve(150 + core::strlen(func) + core::strlen(file) + core::strlen(info));
	msg += StringView(file);
	msg += "(";
	msg += ToString(line);
	msg +=  ")";
	msg += ": обнаружена ошибка при вызове функции\n";
	msg += StringView(func);
	msg += "\n";
	msg += StringView(info);
	msg += "\n\nStack trace:\n";
	msg += GetStackWalk(1);

#ifndef INTRA_NO_LOGGING
	if(IO::ErrorLog!=null)
	{
		IO::ErrorLog.PushFont({1,0,0}, 5);
		IO::ErrorLog << "Критическая ошибка!" << IO::endl;
		IO::ErrorLog.PopFont();
		IO::ErrorLog.PushFont({1,0,0}, 4);
		IO::ErrorLog << msg << IO::endl;
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

void InternalError(StringView func, StringView file, int line, StringView info)
{
	InternalError(String(func).CStr(), String(file).CStr(), line, String(info).CStr());
}

}

