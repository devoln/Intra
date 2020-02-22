#pragma once

#include "Core/Core.h"

INTRA_BEGIN
struct SourceInfo
{
	const char* Function;
	const char* File;
	unsigned Line;
	constexpr forceinline explicit operator bool() const {return Function != null || File != null || Line != 0;}
};

struct DebugStringView
{
	const char* Str;
	index_t Len;

	template<index_t N> constexpr forceinline DebugStringView(const char(&str)[N]): Str(str), Len(N-1) {}
	constexpr forceinline DebugStringView(const char* str, index_t len): Str(str), Len(len) {}
	template<class STR> constexpr forceinline DebugStringView(STR str): Str(str.Data()), Len(str.Length()) {}

	constexpr forceinline index_t Length() const {return Len;}
	constexpr forceinline const char* Data() const {return Str;}
};


typedef void(*FatalErrorCallbackType)(DebugStringView msg, SourceInfo);

//! @returns reference to a function pointer to be called on any fatal error including assertion failures.
//! It becomes initialized automatically if System module is used.
//! Otherwise it should be assigned manually unless the access violations on errors are acceptable.
inline FatalErrorCallbackType& FatalErrorCallback()
{
	static FatalErrorCallbackType callback = null;
	return callback;
}

namespace D {
extern "C" {
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
#if(INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86)
INTRA_DLL_IMPORT int __stdcall IsDebuggerPresent();
#else
INTRA_DLL_IMPORT int IsDebuggerPresent();
#endif
#endif
}
}

inline bool IsDebuggerAttached()
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	return D::IsDebuggerPresent() != 0;
#else
	return false;
#endif
}
INTRA_END

#define INTRA_SOURCE_INFO ::Intra::SourceInfo{INTRA_CURRENT_FUNCTION, __FILE__, unsigned(__LINE__)}

#ifdef INTRA_SOURCE_LOCATION_SUPPORT
#define INTRA_DEFAULT_SOURCE_INFO ::Intra::SourceInfo{__builtin_FUNCTION(), __builtin_FILE(), unsigned(__builtin_LINE())}
#else
#define INTRA_DEFAULT_SOURCE_INFO ::Intra::SourceInfo{}
#endif

#define INTRA_DEBUGGER_BREAKPOINT ((!::Intra::IsConstantEvaluated() && ::Intra::IsDebuggerAttached())? (INTRA_DEBUG_BREAK, true): true)


#define INTRA_FATAL_ERROR(msg) (INTRA_DEBUGGER_BREAKPOINT, \
	::Intra::FatalErrorCallback()(msg, INTRA_SOURCE_INFO))

#define INTRA_ASSERT(expr) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR("Assertion " # expr " failed!"), true))

#define INTRA_ASSERT1(expr, arg0) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::DebugStringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ")+::Intra::StringOf(arg0)), true))

#define INTRA_ASSERT2(expr, arg0, arg1) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::DebugStringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ") + ::Intra::StringOf(arg0)+\
	"\n" # arg1 " = " + ::Intra::StringOf(arg1)), true))

#define INTRA_ASSERT3(expr, arg0, arg1, arg2) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::DebugStringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ")+ ::Intra::StringOf(arg0)+\
	"\n" # arg1 " = " + ::Intra::StringOf(arg1)+\
	"\n" # arg2 " = " + ::Intra::StringOf(arg2)), true))

#define INTRA_ASSERT_EQUALS(lhs, rhs) INTRA_LIKELY((lhs) == (rhs))? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::DebugStringView("Assertion " # lhs " == " # rhs " failed!\n")+\
		::Intra::StringOf((lhs)) + " != " + ::Intra::StringOf((rhs))), true))

#ifdef INTRA_DEBUG

#define INTRA_DEBUG_FATAL_ERROR(msg) INTRA_FATAL_ERROR(msg)

#define INTRA_DEBUG_ASSERT(expr) INTRA_ASSERT(expr)
#define INTRA_DEBUG_ASSERT_EQUALS(lhs, rhs) INTRA_ASSERT_EQUALS(lhs, rhs)
#define INTRA_DEBUG_ASSERT1(expr, arg0) INTRA_ASSERT1(expr, arg0)
#define INTRA_DEBUG_ASSERT2(expr, arg0, arg1) INTRA_ASSERT2(expr, arg0, arg1)
#define INTRA_DEBUG_ASSERT3(expr, arg0, arg1, arg2) INTRA_ASSERT3(expr, arg0, arg1, arg2)
#else
#define INTRA_DEBUG_FATAL_ERROR(msg)
#define INTRA_DEBUG_ASSERT(expr) (void)0
#define INTRA_DEBUG_ASSERT_EQUALS(lhs, rhs) (void)0
#define INTRA_DEBUG_ASSERT1(expr, arg0) (void)0
#define INTRA_DEBUG_ASSERT2(expr, arg0, arg1) (void)0
#define INTRA_DEBUG_ASSERT3(expr, arg0, arg1, arg2) (void)0
#endif

#define INTRA_PRECONDITION INTRA_DEBUG_ASSERT
#define INTRA_POSTCONDITION INTRA_DEBUG_ASSERT
