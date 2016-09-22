#pragma once

#define INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL 0
#define INTRA_COMPILER_INLINE_ASM_SYNTAX_ATAT 1

#if defined(__clang__) || defined(__GNUC__)

//#define INTRA_COMPILER_INLINE_ASM_SYNTAX INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL
#define INTRA_COMPILER_INLINE_ASM_SYNTAX INTRA_COMPILER_INLINE_ASM_SYNTAX_ATAT

#elif defined(_MSC_VER)

#define INTRA_COMPILER_INLINE_ASM_SYNTAX INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL

#endif

#ifdef _MSC_VER
#define INTRA_CRTDECL __cdecl
#else
#define INTRA_CRTDECL
#endif


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

#ifdef _MSC_VER
//Clang 3.7 with Microsoft CodeGen (v140_clang_3_7) почему-то без этого не компилирует
#define _ALLOW_KEYWORD_MACROS
#endif

#endif





#if defined(_MSC_VER) && !defined(__GNUC__)
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS

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
#define INTRA_OVERRIDE_SUPPORT
#define INTRA_FINAL_SUPPORT
#define INTRA_STRONG_TYPED_ENUM_SUPPORT
#define INTRA_TRAILING_RETURN_TYPE_SUPPORT
#define INTRA_STATIC_ASSERT_SUPPORT
#define INTRA_PARTIAL_THREAD_LOCAL_SUPPORT //No destructors for thread_local objects
#endif

#define _ATL_MIN_CRT
#define _ATL_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define _SECURE_SCL 0
#define _SECURE_ATL 0

#define _HAS_EXCEPTIONS 0
//#define _STATIC_CPPLIB

//#define forceinline inline
#define forceinline __forceinline

#pragma warning(disable: 4714) //В дебаге не ругаться на то, что forceinline не сработал
#pragma warning(disable: 4063) //Не ругаться на недопустимые варианты в switch
//#pragma warning(disable: 4396) //Видимо баг, потому что ругается даже тогда, когда inline нет: "если дружественное объявление ссылается на специализацию функции-шаблона, встроенный спецификатор использовать невозможно"

//Убираем ненужное из -Wall
#if _MSC_VER>=1900
#pragma warning(disable: 4577) //Чтобы не ругался на noexcept при отключённых исключениях
#pragma warning(disable: 4868) //компилятор не может принудительно применить порядок вычисления "слева направо" для списка инициализаторов, заключенных в фигурные скобки
#endif

#pragma warning(disable: 4608)
#pragma warning(disable: 4640) //создание в конструкторе локального статического объекта может привести к ошибкам при работе с потоками
#pragma warning(disable: 4514) //подставляемая функция, не используемая в ссылках, была удалена
#pragma warning(disable: 4820) //не ругаться на выравнивание
#pragma warning(disable: 4574) //... определяется как "0": имелось в виду использование "#if ..."?
#pragma warning(disable: 4711) //не сообщать об автоматическом inline
#pragma warning(disable: 4710) //не сообщать об автоматическом не inline
#pragma warning(disable: 4061) //не ругаться на необработанные явно case enum'а


#define _ALLOW_KEYWORD_MACROS

#endif

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")

#ifdef INTRA_MINIMIZE_CRT
#define _NO_CRT_STDIO_INLINE
#endif

//#define _USE_32BIT_TIME_T
#define fseeko64(file, offset, origin) fseek(file, (long)(offset), (origin))
#define ftello64 _ftelli64 //_ftelli64 не работает на старой CRT

#define INTRA_COMPILER_ASSUME(hint)   __assume(hint)
#define INTRA_COMPILER_PRAGMA(pragma) __pragma(pragma)
#elif(defined(__clang__))
extern "C" char* gets(char* str); //Затыкаем ошибку в стандартной библиотеке glibc, из-за которой clang не компилирует
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



#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wreserved-user-defined-literal"
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wunused-value"
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
#define final
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
