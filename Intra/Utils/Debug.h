#pragma once

#include "Preprocessor/Preprocessor.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"
#include "Cpp/Intrinsics.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra {

namespace Range {
template<typename Char> struct GenericStringView;
typedef GenericStringView<const char> StringView;
template<typename T> struct Span;
}
using Range::StringView;
using Range::Span;

struct SourceInfo
{
	const char* Function;
	const char* File;
	unsigned Line;

	constexpr forceinline SourceInfo(null_t=null) noexcept: Function(null), File(null), Line(0) {}
	constexpr forceinline SourceInfo(const char* function, const char* file, unsigned line) noexcept:
		Function(function), File(file), Line(line) {}

	constexpr forceinline bool operator==(null_t) const noexcept
	{return Function == null && File == null && Line == 0;}
	
	constexpr forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}
};

#define INTRA_SOURCE_INFO ::Intra::SourceInfo{INTRA_CURRENT_FUNCTION, __FILE__, unsigned(__LINE__)}

//! Возвращает трассировку стека.
//! На некоторых платформах не реализована и возвращает пустую строку.
//! @param[out] buf Буфер, в который записывается результат.
//! Если результат не помещается в буфер, он будет обрезан.
//! В результате выполнения функции buf указывает на свободный остаток буфера.
//! @param framesToSkip Количество последних фреймов до вызова этой функции, которые нужно пропустить.
//! @param maxFrames Максимальное количество отображаемых последних фреймов стека.
//! @param untilMain При встрече первого символа, содержащего main, трассировка останавливается.
StringView GetStackTrace(Span<char>& buf, size_t framesToSkip, size_t maxFrames, bool untilMain=true);

//! Отправляет сообщение в отладочный вывод или печатает в stderr.
void PrintDebugMessage(StringView message);

//! Отправляет сообщение в отладочный вывод или печатает в stderr,
//! добавляя в начале имя файла и номер строки.
void PrintDebugMessage(StringView message, StringView file, unsigned line);


//! Формирует сообщение об ошибке, содержащее имя функции (func), файла (file), номер строки (line) и переданное описание ошибки (msg).
//! Также содержит stacktrace, если его построение поддерживается в текущей конфигурации.
//! @param[out] buf Буфер, в который записывается результат.
//! Если результат не помещается в буфер, он будет обрезан.
//! @param stackFramesToSkip Количество последних фреймов до вызова этой функции, которые нужно пропустить.
//! @return Ссылка на сообщение об ошибке, записанное в буфере buf, как StringView.
//! В результате выполнения функции buf указывает на свободный остаток буфера.
StringView BuildErrorMessage(Span<char>& buf, StringView func, StringView file,
	unsigned line, StringView msg, size_t stackFramesToSkip=0);

//! Вывести сообщение о критической ошибке на экран и аварийно завершить программу.
void FatalErrorMessageAbort(SourceInfo, StringView msg);

//! Функция, которая вызывается при FatalErrorMessageAbort, например когда срабатывает ассерт.
typedef void(*FatalErrorCallbackType)(SourceInfo, StringView msg);
extern FatalErrorCallbackType gFatalErrorCallback;

//Конструирование StringView вынесено в cpp, чтобы разорвать циклическую зависимость между реализациями ассерта и StringView.
void CallFatalErrorCallback(SourceInfo, StringView msg);
void CallFatalErrorCallback(SourceInfo, const char* msg);


#define INTRA_FATAL_ERROR(msg) (INTRA_DEBUGGER_BREAKPOINT, \
	::Intra::CallFatalErrorCallback(INTRA_SOURCE_INFO, msg))

#if defined(_DEBUG)

#ifndef INTRA_DEBUG
#define INTRA_DEBUG
#endif

#define INTRA_DEBUG_ALLOCATORS
#endif

bool IsDebuggerAttached();

#define INTRA_DEBUGGER_BREAKPOINT (::Intra::IsDebuggerAttached()? (INTRA_DEBUG_BREAK, true): true)


#define INTRA_ASSERT(expr) (expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR("Assertion " # expr " failed!"), true))

#define INTRA_ASSERT1(expr, arg0) (expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::StringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ")+::Intra::StringOf(arg0)), true))

#define INTRA_ASSERT2(expr, arg0, arg1) (expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::Range::StringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ")+::Intra::StringOf(arg0)+\
	"\n" # arg1 " = " + ::Intra::StringOf(arg1)), true))

#define INTRA_ASSERT3(expr, arg0, arg1, arg2) (expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::StringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ")+ ::Intra::StringOf(arg0)+\
	"\n" # arg1 " = " + ::Intra::StringOf(arg1)+\
	"\n" # arg2 " = " + ::Intra::StringOf(arg2)), true))

#define INTRA_ASSERT_EQUALS(lhs, rhs) (lhs)==(rhs)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::StringView("Assertion " # lhs " == " # rhs " failed!\n")+\
		::Intra::StringOf((lhs)) + " != " + ::Intra::StringOf((rhs))), true))


#ifdef INTRA_DEBUG

#define INTRA_DEBUG_FATAL_ERROR(msg) INTRA_FATAL_ERROR(msg)

#define INTRA_DEBUG_WARNING_CHECK(expression, message) {if(!(expression)) {INTRA_DEBUGGER_BREAKPOINT;\
	::Intra::PrintDebugMessage(StringView("Предупреждение: ")+(message));}}

#define INTRA_NAN_CHECK(val) INTRA_DEBUG_ASSERT(val != ::Intra::Math::NaN)
#define INTRA_DEBUG_CODE(...) {__VA_ARGS__};

#define INTRA_DEBUG_ASSERT(expr) INTRA_ASSERT(expr)
#define INTRA_DEBUG_ASSERT_EQUALS(lhs, rhs) INTRA_ASSERT_EQUALS(lhs, rhs)
#define INTRA_DEBUG_ASSERT1(expr, arg0) INTRA_ASSERT1(expr, arg0)
#define INTRA_DEBUG_ASSERT2(expr, arg0, arg1) INTRA_ASSERT2(expr, arg0, arg1)
#define INTRA_DEBUG_ASSERT3(expr, arg0, arg1, arg2) INTRA_ASSERT3(expr, arg0, arg1, arg2)

#ifdef _MSC_VER

#ifndef INTRA_AVOID_STD_HEADERS
INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <malloc.h>
INTRA_WARNING_POP
#define INTRA_HEAP_CHECK INTRA_DEBUG_ASSERT(_heapchk()==_HEAPOK)
#else
#define INTRA_HEAP_CHECK
#endif

#else
#define INTRA_HEAP_CHECK
#endif

#define INTRA_ASSERT_WARNING(expression) ((expression) || (INTRA_DEBUGGER_BREAKPOINT && \
    (::Intra::PrintDebugMessage("Warning (" __FILE__ ":" + ::Intra::ToString(__LINE__) + " in function " + \
        ::Intra::StringView(__FUNCTION__) + "): assertion (" # expression ") failed!\r\n" __FILE__ "\r\n"), false)))

#else
#define INTRA_DEBUG_FATAL_ERROR(msg)
#define INTRA_DEBUG_WARNING_CHECK
#define INTRA_DEBUG_ASSERT(expr) (void)0
#define INTRA_DEBUG_ASSERT_EQUALS(lhs, rhs) (void)0
#define INTRA_DEBUG_ASSERT1(expr, arg0) (void)0
#define INTRA_DEBUG_ASSERT2(expr, arg0, arg1) (void)0
#define INTRA_DEBUG_ASSERT3(expr, arg0, arg1, arg2) (void)0
#define INTRA_NAN_CHECK(val) {if(val==Intra::Math::NaN) {(val)=0;}}
#define INTRA_DEBUG_CODE(...) (void)0
#define INTRA_HEAP_CHECK
#define INTRA_ASSERT_WARNING
#endif

}

INTRA_WARNING_POP
