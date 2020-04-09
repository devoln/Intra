#pragma once

#include "Intra/Core.h"

INTRA_BEGIN
struct SourceInfo
{
	const char* Function;
	const char* File;
	unsigned Line;
	constexpr explicit operator bool() const {return Function != null || File != null || Line != 0;}
	constexpr SourceInfo(
#ifdef INTRA_SOURCE_LOCATION_SUPPORT
		const char* function = __builtin_FUNCTION(),
		const char* file = __builtin_FILE(),
		unsigned line = __builtin_LINE()):
#else
		const char* function = null,
		const char* file = null,
		unsigned line = 0):
#endif
        Function(function), File(file), Line(line) {}
};

struct DebugStringView
{
	const char* Str;
	size_t Len;

	template<size_t N> constexpr DebugStringView(const char(&str)[N]): Str(str), Len(N-1) {}
	constexpr DebugStringView(const char* str, size_t len): Str(str), Len(len) {}
	template<class STR> constexpr DebugStringView(STR str): Str(str.Data()), Len(str.Length()) {}

	[[nodiscard]] constexpr index_t Length() const {return index_t(Len);}
	[[nodiscard]] constexpr const char* Data() const {return Str;}
};


using FatalErrorCallbackType = void(*)(DebugStringView msg, SourceInfo);

//! @returns reference to a function pointer to be called on any fatal error including assertion failures.
//! It becomes initialized automatically if System module is used.
//! Otherwise it should be assigned manually unless the access violations on errors are acceptable.
inline FatalErrorCallbackType& FatalErrorCallback()
{
	static FatalErrorCallbackType callback = null;
	return callback;
}

namespace z_D {
extern "C" {
#ifdef _WIN32
#ifdef __i386__
INTRA_DLL_IMPORT int __stdcall IsDebuggerPresent();
#else
INTRA_DLL_IMPORT int IsDebuggerPresent();
#endif
#endif
}
}

inline bool IsDebuggerAttached()
{
#ifdef _WIN32
	return z_D::IsDebuggerPresent() != 0;
#else
	return false;
#endif
}
INTRA_END

#define INTRA_SOURCE_INFO ::Intra::SourceInfo(INTRA_CURRENT_FUNCTION, __FILE__, unsigned(__LINE__))

#define INTRA_DEBUGGER_BREAKPOINT ((!::Intra::IsConstantEvaluated() && ::Intra::IsDebuggerAttached())? (__builtin_trap(), true): true)


#define INTRA_FATAL_ERROR(msg) (INTRA_DEBUGGER_BREAKPOINT, \
	::Intra::FatalErrorCallback()(msg, INTRA_SOURCE_INFO))

#define INTRA_ASSERT(expr) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR("Assertion " # expr " failed!"), true))

#ifdef INTRA_DEBUG

#define INTRA_DEBUG_FATAL_ERROR(msg) INTRA_FATAL_ERROR(msg)

#define INTRA_DEBUG_ASSERT(expr) INTRA_ASSERT(expr)
#else
#define INTRA_DEBUG_FATAL_ERROR(msg)
#define INTRA_DEBUG_ASSERT(expr) (void)0
#endif

#define INTRA_PRECONDITION INTRA_DEBUG_ASSERT
#define INTRA_POSTCONDITION INTRA_DEBUG_ASSERT
