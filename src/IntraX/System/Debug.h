#pragma once

#include <Intra/Range.h>
#include <Intra/Range/StringView.h>

namespace Intra { INTRA_BEGIN
/// Print stack trace to the provided buffer.
/// This function may be not implemented on some platforms and returns empty string.
/// @param[out] buf Buffer used to hold the result.
/// If result doesn't fit into buffer, it will be cropped.
/// After calling this function ``buf`` will be advanced by <result>.Length() elements to point at the free part of the buffer.
/// @param framesToSkip Number of last frames before calling this function that must be skipped.
/// @param maxFrames Max limit of stack frames to print.
/// @param untilMain Whether to stop stack tracing after encountering symbol containing 'main' substring.
/// @return StringView referring to the result written in ``buf``.
StringView GetStackTrace(Span<char>& buf, size_t framesToSkip, size_t maxFrames, bool untilMain = true);


/// Build a message, containing ``functionName``, ``fileName``, ``lineNumber``, ``type`` and ``msg``.
/// Resulting message will contain stacktrace, if it is supported in this configuration.
/// @param[out] buf Buffer used to hold the result.
/// If result doesn't fit into buffer, it will be cropped.
/// After calling this function ``buf`` will be advanced by <result>.Length() elements to point at the free part of the buffer.
/// @param stackFramesToSkip Number of last frames before calling this function that must be skipped.
/// @return StringView referring to the result written in ``buf``.
StringView BuildDiagnosticMessage(Span<char>& buf, StringView type, StringView msg, SourceInfo sourceInfo, size_t stackFramesToSkip = 0);

///@{
/// Show critical error message and terminate the program.
void FatalErrorMessageAbort(StringView msg, bool printStackTrace, SourceInfo);
void FatalErrorMessageAbort(StringView msg, SourceInfo);
///@}

#define INTRA_ASSERT1(expr, arg0) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::DebugStringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ") + ::IntraX::StringOf(arg0)), true))

#define INTRA_ASSERT2(expr, arg0, arg1) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::DebugStringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ") + ::IntraX::StringOf(arg0)+\
	"\n" # arg1 " = " + ::IntraX::StringOf(arg1)), true))

#define INTRA_ASSERT3(expr, arg0, arg1, arg2) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::DebugStringView("Assertion " # expr " failed!"\
	"\n" # arg0 " = ") + ::IntraX::StringOf(arg0)+\
	"\n" # arg1 " = " + ::IntraX::StringOf(arg1)+\
	"\n" # arg2 " = " + ::IntraX::StringOf(arg2)), true))

#define INTRA_ASSERT_EQUALS(lhs, rhs) INTRA_LIKELY((lhs) == (rhs))? (void)0: (void)(\
    (INTRA_FATAL_ERROR(::Intra::DebugStringView("Assertion " # lhs " == " # rhs " failed!\n")+\
		::IntraX::StringOf((lhs)) + " != " + ::IntraX::StringOf((rhs))), true))

#ifdef INTRA_DEBUG
#define INTRA_DEBUG_ASSERT_EQUALS(lhs, rhs) INTRA_ASSERT_EQUALS(lhs, rhs)
#define INTRA_DEBUG_ASSERT1(expr, arg0) INTRA_ASSERT1(expr, arg0)
#define INTRA_DEBUG_ASSERT2(expr, arg0, arg1) INTRA_ASSERT2(expr, arg0, arg1)
#define INTRA_DEBUG_ASSERT3(expr, arg0, arg1, arg2) INTRA_ASSERT3(expr, arg0, arg1, arg2)
#else
#define INTRA_DEBUG_ASSERT_EQUALS(lhs, rhs) (void)0
#define INTRA_DEBUG_ASSERT1(expr, arg0) (void)0
#define INTRA_DEBUG_ASSERT2(expr, arg0, arg1) (void)0
#define INTRA_DEBUG_ASSERT3(expr, arg0, arg1, arg2) (void)0
#endif

namespace z_D {
class ModuleUnitTest: public GloballyRegistered<ModuleUnitTest>
{
	ModuleUnitTest(const ModuleUnitTest&) = delete;
	ModuleUnitTest(ModuleUnitTest&&) = delete;
	ModuleUnitTest& operator=(const ModuleUnitTest&) = delete;
	ModuleUnitTest& operator=(ModuleUnitTest&&) = delete;
public:
	void(*const Func)();
	const SourceInfo Info;
	
	INTRA_FORCEINLINE constexpr ModuleUnitTest(void(*func)(), SourceInfo info):
		Func(func), Info(info) {}
};
}
} INTRA_END

#define INTRA_CORE_INTERNAL_MODULE_TEST(uniqueName,qualifiers) \
	static qualifiers void uniqueName(); \
	static ::Intra::z_D::ModuleUnitTest INTRA_CONCATENATE_TOKENS(uniqueName, _reg)(&uniqueName, ::Intra::SourceInfo{#uniqueName, __FILE__, unsigned(__LINE__)});\
	static qualifiers void uniqueName()

#ifndef INTRA_NO_UNITTESTS
#define INTRA_MODULE_UNITTEST INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION INTRA_CORE_INTERNAL_MODULE_TEST(INTRA_CONCATENATE_TOKENS(intra_unittest_, __COUNTER__),)
#define INTRA_CONSTEXPR_MODULE_UNITTEST(name) INTRA_CORE_INTERNAL_MODULE_TEST(INTRA_CONCATENATE_TOKENS(intra_unittest_, name), constexpr)
#elif defined(INTRA_MAYBE_UNUSED_SUPPORT) && INTRA_NO_UNITTESTS == 0
#define INTRA_MODULE_UNITTEST INTRA_MAYBE_UNUSED static inline void INTRA_CONCATENATE_TOKENS(intra_unittest_, __COUNTER__)
#else
//template avoids unused function warning and compilation
#define INTRA_MODULE_UNITTEST template<bool> INTRA_MAYBE_UNUSED static inline void INTRA_CONCATENATE_TOKENS(intra_unittest_, __COUNTER__)
#endif

#if INTRA_CONSTEXPR_TEST
#define INTRA_CONSTEXPR_TEST_RUN(name) static_assert((INTRA_CONCATENATE_TOKENS(intra_unittest_, name)(), true), "")
#endif

#ifndef INTRA_CONSTEXPR_TEST_RUN
#define INTRA_CONSTEXPR_TEST_RUN(name)
#endif
