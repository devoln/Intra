#pragma once

#define INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL 0
#define INTRA_COMPILER_INLINE_ASM_SYNTAX_ATAT 1

#if defined(__clang__) || defined(__GNUC__)

//#define INTRA_COMPILER_INLINE_ASM_SYNTAX INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL
#define INTRA_COMPILER_INLINE_ASM_SYNTAX INTRA_COMPILER_INLINE_ASM_SYNTAX_ATAT

#elif defined(_MSC_VER)

#define INTRA_COMPILER_INLINE_ASM_SYNTAX INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL

#endif

//! Используется для отключения кода препроцессором: #if INTRA_DISABLED ... #endif
#define INTRA_DISABLED 0


#ifdef __GNUC__

#if __GNUC__==4 && __GNUC_MINOR__>=3 || __GNUC__>=5
#define INTRA_VARIADIC_TEMPLATE_SUPPORT
#define INTRA_STATIC_ASSERT_SUPPORT
#endif

#if __GNUC__==4 && __GNUC_MINOR__>=4 || __GNUC__>=5
#define INTRA_INITIALIZER_LIST_SUPPORT
#define INTRA_TRAILING_RETURN_TYPE_SUPPORT
#define INTRA_STRONG_TYPED_ENUM_SUPPORT
#define INTRA_CHAR16_SUPPORT
#define INTRA_CHAR32_SUPPORT
#endif


#if !defined(INTRA_CHAR16_SUPPORT) && defined(__CHAR16_TYPE__)
#define INTRA_CHAR16_SUPPORT
#endif
#if !defined(INTRA_CHAR32_SUPPORT) && defined(__CHAR32_TYPE__)
#define INTRA_CHAR32_SUPPORT
#endif

#if __GNUC__==4 && __GNUC_MINOR__>=4 || __GNUC__>=5
#define INTRA_UNICODE_STRING_LITERAL_SUPPORT
#define INTRA_RAW_STRING_LITERAL_SUPPORT
#define INTRA_DEFAULTED_FUNCTION_SUPPORT
#define INTRA_DELETED_FUNCTION_SUPPORT
#endif


#if __GNUC__==4 && __GNUC_MINOR__>=6 || __GNUC__>=5
#define INTRA_FORWARD_DECLARED_ENUM_SUPPORT
#define INTRA_CONSTEXPR_SUPPORT
#define INTRA_UNRESTRICTED_UNION_SUPPORT
#define INTRA_RANGE_BASED_FOR_SUPPORT
#define INTRA_NOEXCEPT_SUPPORT
#endif



#if __GNUC__==4 && __GNUC_MINOR__>=7 || __GNUC__>=5
#define INTRA_USER_DEFINED_LITERAL_SUPPORT
#define INTRA_NON_STATIC_DATA_INITIALIZER_SUPPORT
#define INTRA_TEMPLATE_USING_SUPPORT
#define INTRA_OVERRIDE_SUPPORT
#define INTRA_FINAL_SUPPORT
#endif

#if __GNUC__==4 && __GNUC_MINOR__>=8 || __GNUC__>=5
#define INTRA_THREAD_LOCAL_SUPPORT
#endif
#define INTRA_PARTIAL_THREAD_LOCAL_SUPPORT

#if __GNUC__>=5 && __GNUC_MINOR__>=0
#define INTRA_VARIABLE_TEMPLATE_SUPPORT
#define INTRA_ADVANCED_CONSTEXPR_SUPPORT
#endif

#if __GNUC__>=6 && __GNUC_MINOR__>=0
#define INTRA_GENERALIZED_RANGE_BASED_FOR_SUPPORT
#endif


#define forceinline __inline__ __attribute__((always_inline))

#endif





#if defined(_MSC_VER) && !defined(__GNUC__)

#if _MSC_VER>=1900 //Visual Studio 2015
#define INTRA_CHAR16_SUPPORT
#define INTRA_CHAR32_SUPPORT
#define INTRA_USER_DEFINED_LITERAL_SUPPORT
#define INTRA_CONSTEXPR_SUPPORT
#define INTRA_NOEXCEPT_SUPPORT
#define INTRA_UNRESTRICTED_UNION_SUPPORT
#define INTRA_AUTO_RETURN_TYPE
#define INTRA_THREAD_LOCAL_SUPPORT
#define INTRA_ALIGNMENT_SUPPORT
#define INTRA_UNICODE_STRING_LITERAL_SUPPORT
#endif

#if _MSC_VER>=1800 //Visual Studio 2013
#define INTRA_NON_STATIC_DATA_INITIALIZER_SUPPORT
#define INTRA_INITIALIZER_LIST_SUPPORT
#define INTRA_RAW_STRING_LITERAL_SUPPORT
#define INTRA_DEFAULTED_FUNCTION_SUPPORT
#define INTRA_DELETED_FUNCTION_SUPPORT
#define INTRA_TEMPLATE_USING_SUPPORT
#define INTRA_VARIADIC_TEMPLATE_SUPPORT
#endif

#if _MSC_VER>=1700 //Visual Studio 2012
#define INTRA_FORWARD_DECLARED_ENUM_SUPPORT
#define INTRA_RANGE_BASED_FOR_SUPPORT
#endif

#if _MSC_VER>=1600 //Visual Studio 2010
#define INTRA_FINAL_SUPPORT
#define INTRA_STRONG_TYPED_ENUM_SUPPORT
#define INTRA_TRAILING_RETURN_TYPE_SUPPORT
#define INTRA_STATIC_ASSERT_SUPPORT
#define INTRA_PARTIAL_THREAD_LOCAL_SUPPORT //No destructors for thread_local objects
#endif

#if _MSC_VER>=1400
#define INTRA_OVERRIDE_SUPPORT
#endif

//#define forceinline inline
#define forceinline __forceinline

#endif

#ifdef _MSC_VER

#define INTRA_COMPILER_ASSUME(hint)   __assume(hint)
#define INTRA_COMPILER_PRAGMA(pragma) __pragma(pragma)

#endif

#ifdef __clang__

#if __has_feature(cxx_constexpr)
#define INTRA_CONSTEXPR_SUPPORT
#endif

#if __has_feature(cxx_noexcept)
#define INTRA_NOEXCEPT_SUPPORT
#endif

#if __has_feature(cxx_nonstatic_member_init)
#define INTRA_NON_STATIC_DATA_INITIALIZER_SUPPORT
#endif

#if __has_feature(cxx_override_control)
#define INTRA_OVERRIDE_SUPPORT
#define INTRA_FINAL_SUPPORT
#endif

#if __has_feature(cxx_range_for)
#define INTRA_RANGE_BASED_FOR_SUPPORT
#endif

#if __has_feature(cxx_raw_string_literals)
#define INTRA_RAW_STRING_LITERAL_SUPPORT
#endif

#if __has_feature(cxx_strong_enums)
#define INTRA_STRONG_TYPED_ENUM_SUPPORT
#endif

#if __has_feature(cxx_trailing_return)
#define INTRA_TRAILING_RETURN_TYPE_SUPPORT
#endif

#if __has_feature(cxx_unicode_literals)
#define INTRA_UNICODE_STRING_LITERAL_SUPPORT
#endif

#if __has_feature(cxx_unrestricted_unions)
#define INTRA_UNRESTRICTED_UNION_SUPPORT
#endif

#if __has_feature(cxx_variadic_templates)
#define INTRA_VARIADIC_TEMPLATE_SUPPORT
#endif

#if __has_feature(cxx_static_assert)
#define INTRA_STATIC_ASSERT_SUPPORT
#endif

#if __has_feature(cxx_alias_templates)
#define INTRA_TEMPLATE_USING_SUPPORT
#endif

#if __has_feature(cxx_user_literals)
#define INTRA_USER_DEFINED_LITERAL_SUPPORT
#endif

#if __has_feature(cxx_thread_local)
#define INTRA_THREAD_LOCAL_SUPPORT
#define INTRA_PARTIAL_THREAD_LOCAL_SUPPORT
#endif

#endif

#ifndef forceinline
#define forceinline inline
#endif

#define INTRA_COMPILER_BARRIER _ReadWriteBarrier()


//#undef INTRA_OVERRIDE_SUPPORT
//#undef INTRA_CONSTEXPR_SUPPORT
//#undef INTRA_NOEXCEPT_SUPPORT
//#undef INTRA_FINAL_SUPPORT

#ifndef INTRA_OVERRIDE_SUPPORT
#define override
#endif

#ifndef INTRA_FINAL_SUPPORT

#if _MSC_VER>=1400
#define final sealed
#else
#define final
#endif

#endif

#ifndef INTRA_CONSTEXPR_SUPPORT
#define constexpr
#endif

#ifndef INTRA_NOEXCEPT_SUPPORT
#define noexcept
#endif

#ifndef INTRA_THREAD_LOCAL_SUPPORT

#if defined(_MSC_VER) && _MSC_VER>=1600
#define thread_local __declspec(thread)
#endif

#ifdef __GNUC__
#define thread_local __thread
#endif

#define INTRA_OPTIONAL_THREAD_LOCAL

#else

#define INTRA_OPTIONAL_THREAD_LOCAL thread_local

#endif

#ifndef INTRA_IF_CONSTEXPR

#if defined(_MSC_VER) && !defined(__clang__)
#define INTRA_IF_CONSTEXPR(c) __if_exists( Meta::EnableIf< (c) > )
#endif

#endif

#ifndef INTRA_ALIGNMENT_SUPPORT

#if defined(_MSC_VER) && !defined(__clang__)
#define alignof __alignof
#define alignas(n) __declspec(align(n))
#endif

#endif

#ifndef INTRA_EXCEPTIONS_ENABLED

#ifdef __cpp_exceptions
#if(_cpp_exceptions>=199711)
#define INTRA_EXCEPTIONS_ENABLED
#endif
#endif

#endif

#ifndef INTRA_EXCEPTIONS_ENABLED

#ifdef _CPPUNWIND
#define INTRA_EXCEPTIONS_ENABLED
#endif

#endif

#ifndef INTRA_EXCEPTIONS_ENABLED

#ifdef __EXCEPTIONS
#define INTRA_EXCEPTIONS_ENABLED
#endif

#endif
