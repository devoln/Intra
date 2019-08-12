#pragma once

#include "Core/Assert.h"
#include "Core/Range/StringView.h"

INTRA_BEGIN
inline namespace System {

//! Print stack trace to the provided buffer.
//! This function may be not implemented on some platforms and returns empty string.
//! @param[out] buf Buffer used to hold the result.
//! If result doesn't fit into buffer, it will be cropped.
//! After calling this function ``buf`` will be advanced by <result>.Length() elements to point at the free part of the buffer.
//! @param framesToSkip Number of last frames before calling this function that must be skipped.
//! @param maxFrames Max limit of stack frames to print.
//! @param untilMain Whether to stop stack tracing after encountering symbol containing 'main' substring.
//! @return StringView referring to the result written in ``buf``.
StringView GetStackTrace(Span<char>& buf, size_t framesToSkip, size_t maxFrames, bool untilMain = true);

//! Prints ``message`` as debug output (on Windows) or prints to stderr (other OS).
void PrintDebugMessage(StringView message);

//! Prints ``message`` as debug output (on Windows) or prints to stderr (other OS).
//! Adds ``file`` name and ``line`` number as a prefix.
void PrintDebugMessage(StringView message, StringView file, unsigned line);


//! Build a message, containing ``functionName``, ``fileName``, ``lineNumber``, ``type`` and ``msg``.
//! Resulting message will contain stacktrace, if it is supported in this configuration.
//! @param[out] buf Buffer used to hold the result.
//! If result doesn't fit into buffer, it will be cropped.
//! After calling this function ``buf`` will be advanced by <result>.Length() elements to point at the free part of the buffer.
//! @param framesToSkip Number of last frames before calling this function that must be skipped.
//! @return StringView referring to the result written in ``buf``.
StringView BuildDiagnosticMessage(Span<char>& buf, StringView type, StringView functionName, StringView fileName,
	unsigned lineNumber, StringView msg, size_t stackFramesToSkip = 0);

///@{
//! Show critical error message and terminate the program.
void FatalErrorMessageAbort(StringView msg, bool printStackTrace, SourceInfo);
void FatalErrorMessageAbort(StringView msg, SourceInfo);
///@}



#if defined(_DEBUG)
#define INTRA_DEBUG_ALLOCATORS
#endif

bool IsDebuggerAttached();

namespace D {
struct ModuleUnitTest
{
	void(*const Func)();
	const SourceInfo Info;
	
	ModuleUnitTest(void(*func)(), SourceInfo info):
		Func(func), Info(info), mNext(lastInited) {lastInited = this;}

	INTRA_NODISCARD forceinline ModuleUnitTest* NextListNode() const {return mNext;}
	
	INTRA_NODISCARD forceinline static ModuleUnitTest* First() {return lastInited;}

private:
	ModuleUnitTest* mNext;
	static ModuleUnitTest* lastInited;

	ModuleUnitTest(const ModuleUnitTest&) = delete;
	ModuleUnitTest(ModuleUnitTest&&) = delete;
	ModuleUnitTest& operator=(const ModuleUnitTest&) = delete;
	ModuleUnitTest& operator=(ModuleUnitTest&&) = delete;
};
}
}
INTRA_END




#ifdef _MSC_VER

namespace Intra {
namespace C {
extern "C" int __cdecl _heapchk();
}}
#define INTRA_HEAP_CHECK INTRA_ASSERT(Intra::C::_heapchk() == _HEAPOK)

#else
#define INTRA_HEAP_CHECK
#endif

#define INTRA_CORE_INTERNAL_MODULE_TEST(uniqueName,qualifiers) \
	static qualifiers void uniqueName; \
	static ::Intra::D::ModuleUnitTest INTRA_CONCATENATE_TOKENS(uniqueName, _reg)(&uniqueName, INTRA_SOURCE_INFO);\
	static qualifiers void uniqueName()

#ifndef INTRA_NO_UNITTESTS
#define INTRA_MODULE_UNITTEST INTRA_CORE_INTERNAL_MODULE_TEST(INTRA_CONCATENATE_TOKENS(intra_unittest__, INTRA_UNIQUE_NUMBER),)
#define INTRA_CONSTEXPR_MODULE_UNITTEST(name) INTRA_CORE_INTERNAL_MODULE_TEST(INTRA_CONCATENATE_TOKENS(intra_unittest__, name), constexpr)
#elif defined(INTRA_MAYBE_UNUSED_SUPPORT) && INTRA_NO_UNITTESTS == 0
#define INTRA_MODULE_UNITTEST INTRA_MAYBE_UNUSED static inline void INTRA_CONCATENATE_TOKENS(intra_unittest__, INTRA_UNIQUE_NUMBER)
#else
//template avoids unused function warning and compilation
#define INTRA_MODULE_UNITTEST template<bool> INTRA_MAYBE_UNUSED static inline void INTRA_CONCATENATE_TOKENS(intra_unittest__, INTRA_UNIQUE_NUMBER)
#endif

#if defined(INTRA_CONSTEXPR_TEST) && INTRA_CONSTEXPR_TEST >= 201304
#define INTRA_CONSTEXPR_TEST_RUN(name) enum{INTRA_CONCATENATE_TOKENS(intra_unittest_run__, INTRA_UNIQUE_NUMBER) = INTRA_CORE_INTERNAL_MODULE_TEST(INTRA_CONCATENATE_TOKENS(intra_unittest__, name))()};

#if INTRA_CONSTEXPR_TEST >= 201603
#define INTRA_CONSTEXPR3_TEST_RUN(name) INTRA_CONSTEXPR_TEST_RUN(name)
#endif

#endif

#ifndef INTRA_CONSTEXPR_TEST_RUN
INTRA_CONSTEXPR_TEST_RUN(name)
#endif

#ifndef INTRA_CONSTEXPR3_TEST_RUN
#define INTRA_CONSTEXPR3_TEST_RUN(name)
#endif
