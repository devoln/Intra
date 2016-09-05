#pragma once

#include "Meta/Preprocessor.h"
#include "CompilerSpecific/CompilerSpecific.h"

namespace Intra {

struct SourceInfo
{
	const char* file;
	unsigned line;
};

#define INTRA_SOURCE_INFO SourceInfo{__FILE__, (uint)__LINE__}

template<typename Char> struct GenericStringView;
typedef GenericStringView<char> StringView;
void PrintDebugMessage(StringView message);
void PrintDebugMessage(StringView message, StringView file, int line);
void InternalError(const char* func, const char* file, int line, const char* info);
void InternalError(StringView func, StringView file, int line, StringView info);



#ifdef _MSC_VER
#define INTRA_INTERNAL_ERROR(msg) (INTRA_DEBUGGER_BREAKPOINT, Intra::InternalError(__FUNCSIG__, __FILE__, __LINE__, msg))
#elif defined(__GNUC__)
#define INTRA_INTERNAL_ERROR(msg) (INTRA_DEBUGGER_BREAKPOINT, Intra::InternalError(__PRETTY_FUNCTION__, __FILE__, __LINE__, msg))
#else
#define INTRA_INTERNAL_ERROR(msg) (INTRA_DEBUGGER_BREAKPOINT, Intra::InternalError(__func__, __FILE__, __LINE__, msg))
#endif

#ifdef _DEBUG
#define INTRA_DEBUG
#endif

bool IsDebuggerConnected();

#ifdef _MSC_VER
#define INTRA_DEBUG_BREAK __debugbreak()
#elif defined(__GNUC__)
#define INTRA_DEBUG_BREAK __builtin_trap()
#else
#include <signal.h>
#define INTRA_DEBUG_BREAK raise(SIGTRAP)
#endif

#define INTRA_DEBUGGER_BREAKPOINT (Intra::IsDebuggerConnected()? (INTRA_DEBUG_BREAK, true): true)

namespace detail {

constexpr forceinline const char* past_last_slash(const char* str, const char* lastSlash)
{
    return *str=='\0'? lastSlash: *str=='/'? past_last_slash(str+1, str+1): past_last_slash(str+1, lastSlash);
}

constexpr forceinline const char* past_last_slash(const char* str) 
{ 
    return past_last_slash(str, str);
}

}

#define __SHORT_FILE__ Intra::detail::past_last_slash(__FILE__)

#ifdef INTRA_DEBUG
#define INTRA_DEBUG_WARNING_CHECK(expression, message) {if(!(expression)) {INTRA_DEBUGGER_BREAKPOINT; PrintDebugMessage(StringView("Предупреждение: ")+(message));}}
#define INTRA_ASSERT(expression) (expression) || (\
    (INTRA_INTERNAL_ERROR("Assertion (" # expression ") failed!"), true))
#define INTRA_NAN_CHECK(val) INTRA_ASSERT(val!=Math::NaN)
#define INTRA_DEBUG_CODE(...) {__VA_ARGS___};
#define INTRA_HEAP_CHECK INTRA_ASSERT(_heapchk()==_HEAPOK)

#define INTRA_ASSERT_WARNING(expression) ((expression) || (INTRA_DEBUGGER_BREAKPOINT && \
    (PrintDebugMessage("Warning (" __FILE__ ":" + ToString(__LINE__) + " in function " + \
        StringView(__FUNCTION__) + "): assertion (" # expression ") failed!\r\n" __FILE__ "\r\n"), false)))

#else
#define INTRA_DEBUG_WARNING_CHECK
#define INTRA_ASSERT
#define INTRA_NAN_CHECK(val) {if(val==Intra::Math::NaN) {(val)=0;}}
#define DEBUG_CODE(x)
#define HEAP_CHECK
#define INTRA_ASSERT_WARNING
#endif

}

