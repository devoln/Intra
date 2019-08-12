#pragma once

#include "Core/Core.h"

INTRA_CORE_BEGIN
inline namespace Range {
template<typename Char> struct GenericStringView;
typedef GenericStringView<const char> StringView;
}

struct SourceInfo
{
	const char* Function;
	const char* File;
	unsigned Line;
	constexpr forceinline explicit operator bool() const {return Function != null || File != null || Line != 0;}
};


typedef void(*FatalErrorCallbackType)(StringView msg, SourceInfo);

//! @returns reference to a function pointer to be called on any fatal error including assertion failures.
//! It becomes initialized automatically if System module is used.
//! Otherwise it should be assigned manually unless the access violations on errors are acceptable.
inline FatalErrorCallbackType& FatalErrorCallback()
{
	static FatalErrorCallbackType callback = null;
	return callback;
}
INTRA_CORE_END

#define INTRA_SOURCE_INFO ::Intra::SourceInfo{INTRA_CURRENT_FUNCTION, __FILE__, unsigned(__LINE__)}

#ifdef INTRA_SOURCE_LOCATION_SUPPORT
#define INTRA_DEFAULT_SOURCE_INFO ::Intra::SourceInfo{__builtin_FUNCTION(), __builtin_FILE(), unsigned(__builtin_LINE())}
#else
#define INTRA_DEFAULT_SOURCE_INFO ::Intra::SourceInfo{}
#endif

#define INTRA_DEBUGGER_BREAKPOINT ((!::Intra::IsConstantEvaluated()/* && ::Intra::System::IsDebuggerAttached()*/)? (INTRA_DEBUG_BREAK, true): true)


#define INTRA_FATAL_ERROR(msg) (INTRA_DEBUGGER_BREAKPOINT, \
	::Intra::FatalErrorCallback()(msg, INTRA_SOURCE_INFO))

#define INTRA_ASSERT(expr) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR("Assertion " # expr " failed!"), true))

#define INTRA_ASSERT1(expr, arg0) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::StringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ")+::Intra::StringOf(arg0)), true))

#define INTRA_ASSERT2(expr, arg0, arg1) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::StringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ") + ::Intra::StringOf(arg0)+\
	"\n" # arg1 " = " + ::Intra::StringOf(arg1)), true))

#define INTRA_ASSERT3(expr, arg0, arg1, arg2) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::StringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ")+ ::Intra::StringOf(arg0)+\
	"\n" # arg1 " = " + ::Intra::StringOf(arg1)+\
	"\n" # arg2 " = " + ::Intra::StringOf(arg2)), true))

#define INTRA_ASSERT_EQUALS(lhs, rhs) INTRA_LIKELY((lhs) == (rhs))? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::StringView("Assertion " # lhs " == " # rhs " failed!\n")+\
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
