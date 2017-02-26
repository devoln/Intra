#pragma once

#include "Meta/Preprocessor.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/ForwardDecls.h"
#include "Container/ForwardDecls.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra {

struct SourceInfo
{
	const char* file;
	unsigned line;
};

#define INTRA_SOURCE_INFO ::Intra::SourceInfo{__FILE__, uint(__LINE__)}

//! Возвращает трассировку стека.
//! На некоторых платформах не реализована и возвращает пустую строку.
//! @param framesToSkip Количество последних фреймов до вызова этой функции, которые нужно пропустить.
//! @param maxFrames Максимальное количество отображаемых фреймов стека.
//! @param untilMain При встрече первого символа, содержащего main, трассировка останавливается
String GetStackTrace(size_t framesToSkip, size_t maxFrames, bool untilMain=true);

void PrintDebugMessage(StringView message);
void PrintDebugMessage(StringView message, StringView file, int line);
String BuildErrorMessage(StringView func, StringView file, int line, StringView info, size_t stackFramesToSkip=0);
void InternalErrorMessageAbort(StringView func, StringView file, int line, StringView info);

typedef void(*InternalErrorCallbackType)(StringView func, StringView file, int line, StringView info);
extern InternalErrorCallbackType gInternalErrorCallback;
void CallInternalErrorCallback(const char* func, const char* file, int line, const char* info);
void CallInternalErrorCallback(const char* func, const char* file, int line, StringView info);



#ifdef _MSC_VER
#define INTRA_CURRENT_FUNCTION __FUNCSIG__
#elif defined(__GNUC__)
#define INTRA_CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
#define INTRA_CURRENT_FUNCTION __func__
#endif

#define INTRA_INTERNAL_ERROR(msg) (INTRA_DEBUGGER_BREAKPOINT, \
	::Intra::CallInternalErrorCallback(INTRA_CURRENT_FUNCTION, __FILE__, __LINE__, msg))

#if defined(_DEBUG)

#ifndef INTRA_DEBUG
#define INTRA_DEBUG
#endif

#define INTRA_DEBUG_ALLOCATORS
#endif

bool IsDebuggerAttached();


namespace D {

constexpr forceinline const char* pastLastSlash(const char* str, const char* lastSlash)
{
	return *str=='\0'? lastSlash: *str=='/'?
		pastLastSlash(str+1, str+1):
		pastLastSlash(str+1, lastSlash);
}

constexpr forceinline const char* pastLastSlash(const char* str) 
{return pastLastSlash(str, str);}

}}

#define INTRA_SHORT_FILE ::Intra::D::pastLastSlash(__FILE__)


#ifdef _MSC_VER
#define INTRA_DEBUG_BREAK __debugbreak()
#elif defined(__GNUC__)
#define INTRA_DEBUG_BREAK __builtin_trap()
#else
#include <signal.h>
#define INTRA_DEBUG_BREAK raise(SIGTRAP)
#endif

#define INTRA_DEBUGGER_BREAKPOINT (::Intra::IsDebuggerAttached()? (INTRA_DEBUG_BREAK, true): true)


#define INTRA_ASSERT(expr) (expr)? (void)0: (void)(\
    (INTRA_INTERNAL_ERROR("Assertion " # expr " failed!"), true))

#define INTRA_ASSERT1(expr, arg0) (expr)? (void)0: (void)(\
    (INTRA_INTERNAL_ERROR(::Intra::StringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ")+::Intra::ToString(arg0)), true))

#define INTRA_ASSERT2(expr, arg0, arg1) (expr)? (void)0: (void)(\
    (INTRA_INTERNAL_ERROR(::Intra::Range::StringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ")+::Intra::ToString(arg0)+\
	"\n" # arg1 " = " + ::Intra::ToString(arg1)), true))

#define INTRA_ASSERT3(expr, arg0, arg1, arg2) (expr)? (void)0: (void)(\
    (INTRA_INTERNAL_ERROR(::Intra::Range::StringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ")+ ::Intra::ToString(arg0)+\
	"\n" # arg1 " = " + ::Intra::ToString(arg1)+\
	"\n" # arg2 " = " + ::Intra::ToString(arg2)), true))

#define INTRA_ASSERT_EQUALS(lhs, rhs) (lhs)==(rhs)? (void)0: (void)(\
    (INTRA_INTERNAL_ERROR(::Intra::Range::StringView("Assertion " # lhs " == " # rhs " failed!\n")+\
		::Intra::ToString((lhs)) + " != " + ::Intra::ToString((rhs))), true))


#ifdef INTRA_DEBUG

#define INTRA_DEBUG_INTERNAL_ERROR(msg) INTRA_INTERNAL_ERROR(msg)

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
#define INTRA_DEBUG_INTERNAL_ERROR(msg)
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

INTRA_WARNING_POP
