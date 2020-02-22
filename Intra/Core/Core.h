#pragma once

/**
  This header contains:
  1. C++20 feature testing macros definition for older MSVC, and also feature testing macros that are not present in C++20 (named as INTRA_*_SUPPORT).
  2. emulation of some features or defining them to be no-op,
  3. compiler independent wrappers around compiler extensions and intrinsics,
  4. platform detection
  5. compiler independent warning management macros
  6. implementation of some C++ features that usually require including standard library headers, but without including them:
     placement new (with slightly different syntax), initializer lists.
*/

#if __cplusplus < 201305 && (!defined(_MSC_VER) || _MSC_VER < 1910)
#error Intra library requires C++14 or above! Please, use GCC 5.3 (-std=c++14), Clang 3.5 (-std=c++14), MSVC 2017 or higher
//TODO: and ICC 17+.
#endif

#define INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL 0
#define INTRA_COMPILER_INLINE_ASM_SYNTAX_ATAT 1

#if defined(__cpp_variable_templates) || defined(__cpp_exceptions) || defined(__cpp_rtti) ||\
	defined(_MSC_VER) && _MSC_VER >= 1915 || defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 5
//Starting from GCC 5+, Clang 3.4+, MSVC 19.15+ (2017 15.8) feature test macros are already defined by the compiler
#define INTRA_EMULATE_FEATURE_TEST_MACROS 0
#else
//For older compilers we define them manually
#define INTRA_EMULATE_FEATURE_TEST_MACROS 1
#endif

#if defined(__clang__) || defined(__GNUC__)

//#define INTRA_COMPILER_INLINE_ASM_SYNTAX INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL
#define INTRA_COMPILER_INLINE_ASM_SYNTAX INTRA_COMPILER_INLINE_ASM_SYNTAX_ATAT

#elif defined(_MSC_VER)

#define INTRA_COMPILER_INLINE_ASM_SYNTAX INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL

#endif

//! Readable way to unconditionally disable code blocks with preprocessor: #if INTRA_DISABLED ... #endif
#define INTRA_DISABLED 0

#if defined(__GNUC__) && !defined(__clang__)

// GCC supports constexpr math functions as a non-standard extension: both <cmath> and __builtin_*
#define INTRA_MATH_CONSTEXPR_SUPPORT
#define INTRA_CONSTEXPR_BUILTIN_MEMCMP_SUPPORT
#define INTRA_CONSTEXPR_BUILTIN_STRLEN_SUPPORT
#define INTRA_ASSUME_ALIGNED(ptr, alignmentBytes) static_cast<decltype(+ptr)>(__builtin_assume_aligned(ptr, alignmentBytes))
#define INTRA_SOURCE_LOCATION_SUPPORT

#if __GNUC__ >= 9
#define INTRA_IS_CONSTANT_EVALUATED_SUPPORT
#endif

#define INTRA_BUILTIN_CONSTANT_P_SUPPORT

#if __GNUC__ >= 7
#define INTRA_NODISCARD [[nodiscard]]
#define INTRA_MAYBE_UNUSED [[maybe_unused]]
#else
#define INTRA_NODISCARD [[gnu::warn_unused_result]]
#define INTRA_MAYBE_UNUSED [[gnu::unused]]
#endif

#ifndef INTRA_OPTIMIZE_FUNCTION
#define INTRA_OPTIMIZE_FUNCTION(...) __VA_ARGS__ __attribute__((flatten, optimize("O3")))
#define INTRA_OPTIMIZE_FUNCTION_END
#endif

#ifndef forceinline
#define forceinline __inline__ __attribute__((always_inline))
#endif

#elif defined(__clang__)

#define INTRA_CONSTEXPR_BUILTIN_STRLEN_SUPPORT

#if __has_feature(cxx_constexpr_string_builtins) //Supported since Clang 4.0
#define INTRA_CONSTEXPR_BUILTIN_MEMCMP_SUPPORT
#endif

#if __has_builtin(__builtin_is_constant_evaluated)
#define INTRA_IS_CONSTANT_EVALUATED_SUPPORT
#endif

#if __has_builtin(__builtin_constant_p)
#define INTRA_BUILTIN_CONSTANT_P_SUPPORT
#endif

#if __has_builtin(__builtin_FUNCTION) && __has_builtin(__builtin_FILE) && __has_builtin(__builtin_LINE)
#define INTRA_SOURCE_LOCATION_SUPPORT
#endif

#if __has_builtin(__builtin_assume_aligned)
#define INTRA_ASSUME_ALIGNED(ptr, alignmentBytes) static_cast<decltype(+ptr)>(__builtin_assume_aligned(ptr, alignmentBytes))
#endif

#ifndef INTRA_OPT_UNROLL_LOOP
#define INTRA_OPT_UNROLL_LOOP _Pragma(clang loop unroll(enable))
#endif

#elif defined(_MSC_VER) && !defined(__GNUC__)

//No need to define feature test macros for features introduced after VS 2017 15.8
#if INTRA_EMULATE_FEATURE_TEST_MACROS

#if _MSC_VER >= 1914 && _MSVC_LANG >= 201703 //Visual Studio 2017 15.7
#define __cpp_inheriting_constructors 201511
#define __cpp_variadic_using 201611
#define __cpp_deduction_guides 201703
#define INTRA_CONSTEXPR_BUILTIN_MEMCMP_SUPPORT
#define INTRA_CONSTEXPR_BUILTIN_STRLEN_SUPPORT
#else
#define __cpp_inheriting_constructors 200802
#endif

#if _MSC_VER >= 1911 && _MSVC_LANG >= 201703 //Visual Studio 2017 15.3
#define INTRA_NODISCARD [[nodiscard]]
#define INTRA_MAYBE_UNUSED [[maybe_unused]]
#endif

#if _MSVC_LANG >= 201703
#define __cpp_range_based_for 201603
#define __cpp_static_assert 201411
#else
#define __cpp_range_based_for 200907
#define __cpp_static_assert 200410
#endif

#define __cpp_constexpr 201304
#define __cpp_variable_templates 201304
#define __cpp_unicode_characters 200704
#define __cpp_user_defined_literals 200809
#define __cpp_return_type_deduction 201304
#define __cpp_unicode_literals 200710
#define __cpp_attributes 200809
#define __cpp_nsdmi 200809
#define __cpp_initializer_lists 200806
#define __cpp_raw_strings 200710
#define __cpp_alias_templates 200704
#define __cpp_variadic_templates 200704
#define __cpp_explicit_conversion 200710
#endif

#ifndef forceinline
#define forceinline __forceinline
#endif

#ifndef INTRA_OPTIMIZE_FUNCTION
#define INTRA_OPTIMIZE_FUNCTION(...) __pragma(optimize("gt", on)) __VA_ARGS__ //gty
#define INTRA_OPTIMIZE_FUNCTION_END __pragma(optimize("", on))
#endif

#else
#warning Unrecognized compiler
#endif

#ifndef INTRA_EXPORT
// Will be defined to "export" for compilers supporting C++ modules
#define INTRA_EXPORT
#endif

#if (defined(__GNUC__) || defined(__clang__)) && !defined(_MSC_VER)
#define INTRA_LIKELY(expr)  __builtin_expect(!!(expr), 1)
#else
#define INTRA_LIKELY(expr)  (!!(expr))
#endif

#ifdef _MSC_VER
#define INTRA_COMPILER_ASSUME(hint) __assume(hint)
#else
#define INTRA_COMPILER_ASSUME(hint) INTRA_LIKELY(hint)
#endif

#ifndef INTRA_OPTIMIZE_FUNCTION
#define INTRA_OPTIMIZE_FUNCTION(...) __VA_ARGS__
#define INTRA_OPTIMIZE_FUNCTION_END
#endif

#ifndef INTRA_OPT_UNROLL_LOOP
#define INTRA_OPT_UNROLL_LOOP
#endif

#ifndef forceinline
#define forceinline inline
#endif

// The folowing definitions are used by this library to define functions that can be constexpr depending on standard math functions' constexprness
#ifdef INTRA_MATH_CONSTEXPR_SUPPORT
#define INTRA_MATH_CONSTEXPR constexpr
#else
#define INTRA_MATH_CONSTEXPR
#endif

//constexpr only if constexpr lambdas are supported
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201603
#define INTRA_CONSTEXPR3 constexpr
#else
#define INTRA_CONSTEXPR3
#endif

#ifndef __cpp_exceptions
#if defined(_CPPUNWIND) || defined(__EXCEPTIONS)
#define __cpp_exceptions 199711
#endif
#endif

#ifndef __cpp_concepts
//partially emulate concepts
#define concept constexpr bool
#elif defined(__GNUC__) && !defined(__clang__) && __GNUC__ <= 8
//older GCC concepts do not support modern syntax without bool, emulate modern syntax
#define concept concept bool
#endif

#ifndef INTRA_NODISCARD
#define INTRA_NODISCARD
#endif

#ifndef INTRA_MAYBE_UNUSED
#define INTRA_MAYBE_UNUSED
#else
#define INTRA_MAYBE_UNUSED_SUPPORT
#endif

#ifdef _MSC_VER
#define INTRA_CURRENT_FUNCTION __FUNCSIG__
#elif defined(__GNUC__)
#define INTRA_CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
#define INTRA_CURRENT_FUNCTION __func__
#endif

#ifdef _MSC_VER
#define INTRA_EMPTY_BASES __declspec(empty_bases)
#define INTRA_NOVTABLE  __declspec(novtable)
#define INTRA_CRTDECL __cdecl //Used to import functions from CRT without including its headers
#if(defined(INTRA_SIMD_LEVEL) && INTRA_SIMD_LEVEL != 0)
#define INTRA_VECTORCALL __vectorcall
#else
#define INTRA_VECTORCALL
#endif
#else
#define INTRA_EMPTY_BASES
#define INTRA_NOVTABLE
#define INTRA_VECTORCALL
#define INTRA_CRTDECL
#endif

#ifdef _MSC_VER
#define INTRA_NO_VECTORIZE_LOOP __pragma(loop(no_vector))
#define INTRA_NO_VECTORIZE_FUNC
#elif defined(__GNUC__)
#define INTRA_NO_VECTORIZE_LOOP
#define INTRA_NO_VECTORIZE_FUNC __attribute__((optimize("no-tree-vectorize")))
#else
#define INTRA_NO_VECTORIZE_LOOP
#define INTRA_NO_VECTORIZE_FUNC
#endif


#ifdef __COUNTER__
#define INTRA_UNIQUE_NUMBER __COUNTER__
#else
#define INTRA_UNIQUE_NUMBER __LINE__
#endif

#ifndef INTRA_ASSUME_ALIGNED
#define INTRA_ASSUME_ALIGNED(ptr, alignmentBytes) ptr
#endif


#ifndef INTRA_CONSTEXPR_TEST
/*!
  @brief If constexpr tests are enabled it is defined to compiler supported constexpr version value.

  If constexpr is supported tests are enabled by default. Possible values:
  1) 201304 for C++14
  2) 201603 for C++17
  To disable constexpr tests use #define INTRA_CONSTEXPR_TEST 0
*/
#define INTRA_CONSTEXPR_TEST __cpp_constexpr
#endif

#define INTRA_CHECK_TABLE_SIZE(table, expectedSize) static_assert(\
	sizeof(table)/sizeof(table[0]) == size_t(expectedSize), "Table is outdated!")

#if defined(_WIN32) || defined(__CYGWIN__)
#define INTRA_DLL_IMPORT __declspec(dllimport)
#define INTRA_DLL_EXPORT __declspec(dllexport)
#define INTRA_DLL_LOCAL
#elif __GNUC__ >= 4
#define INTRA_DLL_IMPORT __attribute__ ((visibility ("default")))
#define INTRA_DLL_EXPORT __attribute__ ((visibility ("default")))
#define INTRA_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define INTRA_DLL_IMPORT
#define INTRA_DLL_EXPORT
#define INTRA_DLL_LOCAL
#endif

#ifdef INTRA_BUILD_DLL
#define INTRA_DLL_API INTRA_DLL_EXPORT
#elif defined(INTRA_USE_DLL)
#define INTRA_DLL_API INTRA_DLL_IMPORT
#else
#define INTRA_DLL_API
#endif

#ifdef _MSC_VER

#if(!defined(__GNUC__) && !defined(__clang__))
#pragma execution_character_set("utf-8")
#endif

#endif


///@{
//! Processor architectures
#define INTRA_PLATFORM_X86 0
#define INTRA_PLATFORM_X86_64 1
#define INTRA_PLATFORM_ARM 2
#define INTRA_PLATFORM_PowerPC 3
#define INTRA_PLATFORM_MIPS 4
#define INTRA_PLATFORM_IA64 5
#define INTRA_PLATFORM_ARM64 6
#define INTRA_PLATFORM_Emscripten 7
///@}

///@{
//! Byte orders
#define INTRA_PLATFORM_ENDIANESS_LittleEndian 0
#define INTRA_PLATFORM_ENDIANESS_BigEndian 1
#define INTRA_PLATFORM_ENDIANESS_MixedEndian 2
///@}

///@{
//! OS types
#define INTRA_PLATFORM_OS_Windows 0
#define INTRA_PLATFORM_OS_Linux 1
#define INTRA_PLATFORM_OS_Android 2
#define INTRA_PLATFORM_OS_FreeBSD 3
#define INTRA_PLATFORM_OS_iOS 4
#define INTRA_PLATFORM_OS_MacOS 5
#define INTRA_PLATFORM_OS_Emscripten 6
#define INTRA_PLATFORM_OS_WebAssembly 7
///@}

// Try to automatically detect the target OS
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)

#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_Windows
#endif

#endif

#ifdef __EMSCRIPTEN__
#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_Emscripten
#endif

#ifndef INTRA_PLATFORM_ARCH
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_Emscripten
#endif
#endif

#ifdef __ANDROID__
#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_Android
#endif
#endif

#ifdef __linux__
#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_Linux
#endif
#endif

#ifdef __FreeBSD__
#ifndef INTRA_PLATFORM_OS
#define INTRA_PLATFORM_OS INTRA_PLATFORM_OS_FreeBSD
#endif
#endif

#ifndef INTRA_PLATFORM_ARCH

#if defined(WIN64) || defined(_WIN64)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_X86_64
#elif(defined(WIN32) || defined(_WIN32))
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_X86
#endif

#endif

// Try to automatically detect the target processor architecture
#ifndef INTRA_PLATFORM_ARCH
#if defined(__i386__) || defined(__i686__) || defined(_M_IX86)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_X86
#endif
#if defined(__amd64__) || defined(_M_AMD64) || defined(_M_X64)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_X86_64
#endif

#if(defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64))
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_ARM64
#elif defined(__arm__) || defined(__thumb__) || defined(_M_ARM) || defined(_M_ARMT)

#ifdef __LP64__
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_ARM64
#else
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_ARM
#endif

#endif

#if defined(__powerpc__) || defined(_M_PPC) || defined(__powerpc)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_PowerPC
#endif

#if defined(__mips__) || defined(__MIPS__) || defined(__mips)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_MIPS
#endif

#if defined(__ia64__) || defined(_M_IA64) || defined(__IA64__)
#define INTRA_PLATFORM_ARCH INTRA_PLATFORM_IA64
#endif

#endif

#if(INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86_64 ||\
	INTRA_PLATFORM_ARCH == INTRA_PLATFORM_IA64 ||\
	INTRA_PLATFORM_ARCH == INTRA_PLATFORM_ARM64)
#define INTRA_PLATFORM_IS_64
#endif

// Try to automatically detect the target architecture's byte order
#if !defined(INTRA_PLATFORM_ENDIANESS) && defined(__BYTE_ORDER__)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define INTRA_PLATFORM_ENDIANESS INTRA_PLATFORM_ENDIANESS_LittleEndian
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define INTRA_PLATFORM_ENDIANESS INTRA_PLATFORM_ENDIANESS_BigEndian
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
#define INTRA_PLATFORM_ENDIANESS INTRA_PLATFORM_ENDIANESS_MixedEndian
#endif

#endif

#ifndef INTRA_PLATFORM_ENDIANESS

#if(INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86_64)
#define INTRA_PLATFORM_ENDIANESS INTRA_PLATFORM_ENDIANESS_LittleEndian
#endif

#endif

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_FreeBSD ||\
	defined(unix) || defined(__unix) || defined(__unix__))
#define INTRA_PLATFORM_IS_UNIX
#endif


#ifndef INTRA_DISABLE_PLATFORM_DETECT_ERRORS // If something was not set and we couldn't detect it, report an error
#ifndef INTRA_PLATFORM_ARCH
#error Cannot determine target processor architecture!
#endif

#ifndef INTRA_PLATFORM_OS
#error Cannot determine target OS!
#endif

#ifndef INTRA_PLATFORM_ENDIANESS
#error Cannot determine byte order for this platform!
#endif
#endif

#define INTRA_PREPROCESSOR_QUOTE(x) #x
#define INTRA_DETAIL_CONCATENATE_TOKENS(x, y) x ## y
#define INTRA_CONCATENATE_TOKENS(x, y) INTRA_DETAIL_CONCATENATE_TOKENS(x, y)

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
#if _MSC_VER >= 1920 //Visual Studio 2019 has no Windows XP toolkit version
#define INTRA_DROP_XP_SUPPORT
#endif
#endif

#ifdef __clang__
#define INTRAZ_D_COMPILER_DIAGNOSTIC_NAME clang
#elif defined(__GNUC__)
#define INTRAZ_D_COMPILER_DIAGNOSTIC_NAME GCC
#endif

#if defined(__clang__) || defined(__GNUC__)
#define INTRA_WARNING_PUSH _Pragma(INTRA_PREPROCESSOR_QUOTE(INTRA_COMPILER_DIAGNOSTIC_NAME) " diagnostic push")
#define INTRA_WARNING_POP _Pragma(INTRA_PREPROCESSOR_QUOTE(INTRA_COMPILER_DIAGNOSTIC_NAME) " diagnostic pop")
#ifdef INTRA_WARNINGS_AS_ERRORS
#define INTRAZ_D_DIAGNOSTIC_MODE error
#else
#define INTRAZ_D_DIAGNOSTIC_MODE enable
#endif
#define INTRAZ_D_WARNING_HELPER(mode, x) INTRA_PREPROCESSOR_QUOTE(INTRAZ_D_COMPILER_DIAGNOSTIC_NAME diagnostic mode x)
#define INTRAZ_D_IGNORE_WARNING_HELPER(y) INTRA_DETAIL_WARNING_HELPER(ignored, #y)
#define INTRA_IGNORE_WARNING(x) _Pragma(INTRA_DETAIL_IGNORE_WARNING_HELPER(-W ## x))
#define INTRAZ_D_ENABLE_WARNING_HELPER(mode, y) INTRA_DETAIL_WARNING_HELPER(mode, #y)
#define INTRA_ENABLE_WARNING(x) _Pragma(INTRAZ_D_ENABLE_WARNING_HELPER, (-W ## x))
#define INTRA_ENABLE_WARNING_NO_ERROR(x) _Pragma(INTRAZ_D_ENABLE_WARNING_HELPER(enable, -W ## x))

#define INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_GCC_CLANG INTRA_IGNORE_WARNING(ctor-dtor-privacy)

#define INTRAZ_D_DISABLE_ALL_WARNINGS_GCC_CLANG \
	INTRA_IGNORE_WARNING(all) \
	INTRA_IGNORE_WARNING(extra) \
	INTRA_IGNORE_WARNING(old-style-cast) \
	INTRA_IGNORE_WARNING(conversion) \
	INTRA_IGNORE_WARNING(sign-conversion) \
	INTRA_IGNORE_WARNING(init-self) \
	INTRA_IGNORE_WARNING(unreachable-code) \
	INTRA_IGNORE_WARNING(pointer-arith) \
	INTRA_IGNORE_WARNING(pedantic)

#define INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC_CLANG \
	INTRA_ENABLE_WARNING(all) \
	INTRA_ENABLE_WARNING(extra) \
	INTRA_ENABLE_WARNING(old-style-cast) \
	INTRA_ENABLE_WARNING(conversion) \
	INTRA_ENABLE_WARNING(sign-conversion) \
	INTRA_ENABLE_WARNING(init-self) \
	INTRA_ENABLE_WARNING(unreachable-code) \
	INTRA_ENABLE_WARNING(pointer-arith) \
	INTRA_ENABLE_WARNING(pedantic) \
	INTRA_ENABLE_WARNING(non-virtual-dtor) \
	INTRA_ENABLE_WARNING(effc++) \
	INTRA_ENABLE_WARNING(shadow) \
	INTRA_ENABLE_WARNING_NO_ERROR(unused-variable)
//-Wshadow=local?

#define INTRA_WARNING_DISABLE_LOSING_CONVERSION INTRA_IGNORE_WARNING(conversion)
#define INTRA_WARNING_DISABLE_SIGN_CONVERSION INTRA_IGNORE_WARNING(sign-conversion)
#define INTRA_WARNING_DISABLE_UNREACHABLE_CODE INTRA_IGNORE_WARNING(unreachable-code)
#define INTRA_WARNING_DISABLE_NO_VIRTUAL_DESTRUCTOR INTRA_IGNORE_WARNING(non-virtual-dtor)
#else
#define INTRA_IGNORE_WARNING(x)
#define INTRA_ENABLE_WARNING_NO_ERROR(x)
#define INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_GCC_CLANG
#define INTRAZ_D_DISABLE_ALL_WARNINGS_GCC_CLANG
#define INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC_CLANG
#ifdef _MSC_VER
#define INTRA_WARNING_PUSH __pragma(warning(push))
#define INTRA_WARNING_POP __pragma(warning(pop))
#else
#define INTRA_WARNING_PUSH
#define INTRA_WARNING_POP
#endif
#endif

#define INTRA_WARNING_DISABLE_UNUSED_FUNCTION INTRA_IGNORE_WARNING(unused-function)

#ifdef __clang__
#define INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_CLANG \
	INTRA_IGNORE_WARNING(c++98-compat) \
	INTRA_IGNORE_WARNING(c++98-compat-pedantic) \
	INTRA_IGNORE_WARNING(documentation-unknown-command) \
	INTRA_IGNORE_WARNING(comma) \
	INTRA_IGNORE_WARNING(duplicate-enum) \
	INTRA_IGNORE_WARNING(implicit-fallthrough) \
	INTRA_IGNORE_WARNING(float-equal) \
	INTRA_IGNORE_WARNING(reserved-id-macro)
#define INTRA_WARNING_DISABLE_GLOBAL_CONSTRUCTION INTRA_IGNORE_WARNING(exit-time-destructors) INTRA_IGNORE_WARNING(global-constructors)
#define INTRA_WARNING_DISABLE_UNDEFINED_REINTERPRET_CAST INTRA_IGNORE_WARNING(undefined-reinterpret-cast)
#define INTRA_WARNING_DISABLE_LANGUAGE_EXTENSION INTRA_IGNORE_WARNING(language-extension-token)
#define INTRAZ_D_DISABLE_ALL_WARNINGS_CLANG \
	INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_CLANG \
	INTRA_WARNING_DISABLE_GLOBAL_CONSTRUCTION \
	INTRA_WARNING_DISABLE_UNDEFINED_REINTERPRET_CAST \
	INTRA_WARNING_DISABLE_LANGUAGE_EXTENSION
#define INTRAZ_D_ENABLE_USEFUL_WARNINGS_CLANG //there is no currently known useful clang-only warnings
#else
#define INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_CLANG
#define INTRA_WARNING_DISABLE_GLOBAL_CONSTRUCTION
#define INTRA_WARNING_DISABLE_UNDEFINED_REINTERPRET_CAST
#define INTRA_WARNING_DISABLE_LANGUAGE_EXTENSION
#define INTRAZ_D_DISABLE_ALL_WARNINGS_CLANG
#define INTRAZ_D_ENABLE_USEFUL_WARNINGS_CLANG
#endif

#if defined(__GNUC__) && !defined(__clang__)
#if __GNUC__ >= 7
#define INTRAZ_D_DISABLE_ALL_WARNINGS_GCC7 INTRA_IGNORE_WARNING(duplicated-branches) INTRA_IGNORE_WARNING(restrict)
#define INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC7 INTRA_ENABLE_WARNING(duplicated-branches) INTRA_ENABLE_WARNING(restrict)
#else
#define INTRAZ_D_DISABLE_ALL_WARNINGS_GCC7
#define INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC7
#endif
#if __GNUC__ >= 6
#define INTRAZ_D_DISABLE_ALL_WARNINGS_GCC6 INTRA_IGNORE_WARNING(duplicated-cond) INTRA_IGNORE_WARNING(null-dereference)
#define INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC6 INTRA_ENABLE_WARNING(duplicated-cond) INTRA_ENABLE_WARNING(null-dereference)
#else
#define INTRAZ_D_DISABLE_ALL_WARNINGS_GCC6
#define INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC6
#endif
#define INTRAZ_D_DISABLE_ALL_WARNINGS_GCC \
	INTRAZ_D_DISABLE_ALL_WARNINGS_GCC6 \
	INTRAZ_D_DISABLE_ALL_WARNINGS_GCC7 \
	INTRA_IGNORE_WARNING(logical-op) \
	INTRA_IGNORE_WARNING(useless-cast)

#define INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC \
	INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC6 \
	INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC7 \
	INTRA_ENABLE_WARNING(logical-op) \
	INTRA_ENABLE_WARNING(useless-cast)

#define INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_GCC //no redundant gcc-specific warnings
#else
#define INTRAZ_D_DISABLE_ALL_WARNINGS_GCC
#define INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC
#define INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_GCC
#endif

#ifdef _MSC_VER
//TODO: find a way to get rid of this
#pragma warning(disable: 4514 4710 4714) //these warnings cannot be removed locally
//4514 - unused inline function
//4710 - function not inlined
//4714 - cannot inline forceinline function
#define INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_MSVC __pragma(warning(disable: 4514 4710 4714  4820 4711 4577 4868 5045))
//4820 - bytes padding added after construct
//4711 - function selected for inline expansion
//4577 - noexcept when compiling with exceptions disabled
//4868 - compiler may not enforce left-to-right evaluation order in braced initializer list
//5045 - compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

#if !defined(__clang__)
#define INTRA_DISABLE_LNK4221 namespace {char INTRA_CONCATENATE_TOKENS($DisableLNK4221__, __LINE__);}
#define INTRA_WARNING_DISABLE_SIGN_CONVERSION __pragma(warning(disable: 4365))
#define INTRA_WARNING_DISABLE_LOSING_CONVERSION __pragma(warning(disable: 4244))
#define INTRA_WARNING_DISABLE_UNREACHABLE_CODE __pragma(warning(disable: 4702))
#define INTRA_WARNING_DISABLE_NO_VIRTUAL_DESTRUCTOR __pragma(warning(disable: 4265))
#endif
#define INTRA_WARNING_DISABLE_UNHANDLED_ENUM_CASES __pragma(warning(disable: 4061))
#define INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED __pragma(warning(disable: 4625 4626 4512))
#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED __pragma(warning(disable: 5026 5027))
#define INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED __pragma(warning(disable: 4510 4610 4623))
#define INTRA_WARNING_DISABLE_ASSIGN_IMPLICITLY_DELETED __pragma(warning(disable: 4626 5027))
#define INTRA_WARNING_DISABLE_CONSTANT_CONDITION __pragma(warning(disable: 4127))
#define INTRA_WARNING_DISABLE_CATCH_ALL __pragma(warning(disable: 4571))
#if _MSC_VER >= 1916
#define INTRA_WARNING_DISABLE_STD_DECLARATION __pragma(warning(disable: 4643))
#endif

#define INTRA_PUSH_DISABLE_ALL_WARNINGS __pragma(warning(push, 0)) \
	__pragma(warning(disable: 4548 4987 4774 4702 4355 4738 4571)) \
	INTRA_WARNING_DISABLE_LOSING_CONVERSION \
	INTRA_WARNING_DISABLE_SIGN_CONVERSION \
	INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED \
	INTRAZ_D_DISABLE_ALL_WARNINGS_GCC_CLANG \
	INTRAZ_D_DISABLE_ALL_WARNINGS_GCC \
	INTRAZ_D_DISABLE_ALL_WARNINGS_CLANG

#define INTRAZ_D_ENABLE_USEFUL_WARNINGS_MSVC __pragma(warning(default: 4365 4244 4702 4265 4061 4625 4626 4512 5026 5027 4510 4610 4623 4626 5027 4127 4571  4548 4987 4774 4702 4355 4738))
#define INTRA_PUSH_ENABLE_USEFUL_WARNINGS __pragma(warning(push, 4)) INTRA_ENABLE_USEFUL_WARNINGS
#else
#define INTRA_PUSH_DISABLE_ALL_WARNINGS \
	INTRA_WARNING_PUSH \
	INTRAZ_D_DISABLE_ALL_WARNINGS_GCC_CLANG \
	INTRAZ_D_DISABLE_ALL_WARNINGS_GCC \
	INTRAZ_D_DISABLE_ALL_WARNINGS_CLANG

#define INTRA_PUSH_ENABLE_USEFUL_WARNINGS INTRA_WARNING_PUSH INTRA_ENABLE_USEFUL_WARNINGS

#define INTRA_WARNING_DISABLE_UNHANDLED_ENUM_CASES
#define INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_ASSIGN_IMPLICITLY_DELETED
#define INTRA_WARNING_DISABLE_CONSTANT_CONDITION
#endif

#define INTRA_DISABLE_REDUNDANT_WARNINGS \
	INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_GCC_CLANG \
	INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_GCC \
	INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_CLANG \
	INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_MSVC

#define INTRA_ENABLE_USEFUL_WARNINGS \
	INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC_CLANG \
	INTRAZ_D_ENABLE_USEFUL_WARNINGS_CLANG \
	INTRAZ_D_ENABLE_USEFUL_WARNINGS_GCC \
	INTRAZ_D_ENABLE_USEFUL_WARNINGS_MSVC

#ifndef INTRA_WARNING_DISABLE_CATCH_ALL
#define INTRA_WARNING_DISABLE_CATCH_ALL
#endif

#ifndef INTRA_WARNING_DISABLE_STD_DECLARATION
#define INTRA_WARNING_DISABLE_STD_DECLARATION
#endif

#ifndef INTRA_DISABLE_LNK4221
#define INTRA_DISABLE_LNK4221
#endif

#ifndef INTRA_WARNING_DISABLE_GLOBAL_CONSTRUCTION
#define INTRA_WARNING_DISABLE_GLOBAL_CONSTRUCTION
#endif

#ifndef INTRA_WARNING_DISABLE_UNDEFINED_REINTERPRET_CAST
#define INTRA_WARNING_DISABLE_UNDEFINED_REINTERPRET_CAST
#endif

#ifndef INTRA_WARNING_DISABLE_LANGUAGE_EXTENSION
#define INTRA_WARNING_DISABLE_LANGUAGE_EXTENSION
#endif

#define INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS \
	INTRA_WARNING_PUSH INTRA_DISABLE_REDUNDANT_WARNINGS

#define INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED \
	INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED \
	INTRA_WARNING_DISABLE_MOVE_IMPLICITLY_DELETED

#define INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED \
	INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED \
	INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED

#define INTRA_BEGIN namespace Intra { INTRA_PUSH_ENABLE_USEFUL_WARNINGS INTRA_DISABLE_REDUNDANT_WARNINGS
#define INTRA_END INTRA_WARNING_POP }

INTRA_BEGIN

#ifndef INTRA_NO_BASIC_TYPE_SIZE_CHECK
// This library relies on certain sizes of the base types.
// Platforms where these types differ are not supported.
// You can disable this check to try to run this library at your own risk.
static_assert(
	sizeof(char) == 1 &&
	sizeof(short) == 2 &&
	(sizeof(int) == 2 || //TODO: there might be some Intra code assuming sizeof(int) == 4. Check and fix it.
		sizeof(int) == 4) &&
	sizeof(long long) == 8 &&
	sizeof(float) == 4 &&
	sizeof(double) == 8,
	"Some of fundamental types have unexpected sizes!");
#endif

typedef unsigned char byte;
typedef signed char sbyte;
typedef unsigned short ushort;
typedef unsigned uint;
typedef long long int64;
typedef unsigned long long uint64;

#if defined(__SIZEOF_INT__) && __SIZEOF_INT__ == 2
#if defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 4
typedef long int32;
typedef unsigned long uint32;
#else
#error Neither int nor long is 32 bit?
#endif
#else
typedef int int32;
typedef unsigned uint32;
#endif

//! Signed integral type with size dependent on pointer size.
typedef decltype(reinterpret_cast<char*>(1)-reinterpret_cast<char*>(0)) intptr;

//! Unsigned integral type with size dependent on pointer size.
typedef decltype(sizeof(int)) uintptr;

//! Integral type with size dependent on pointer size that is used for indices and sizes.
typedef intptr index_t;

//null is used for null pointers. In Intra it has additional meaning: empty object, equivalent to default constructed one.
typedef decltype(nullptr) null_t;
constexpr const null_t null = nullptr;

#ifdef __SIZEOF_INT128__
#define INTRA_INT128_SUPPORT
using int128 = __int128;
using uint128 = unsigned __int128;
#endif

template<typename T, T value> struct TypeFromValue
{static constexpr T _ = value;};
template<bool value> using TIndex = TypeFromValue<index_t, value>;
template<bool value> using TBool = TypeFromValue<bool, value>;
typedef TBool<false> FalseType;
typedef TBool<true> TrueType;

namespace D {
template<typename T> struct TRemoveReference_ {typedef T _;};
template<typename T> struct TRemoveReference_<T&> {typedef T _;};
template<typename T> struct TRemoveReference_<T&&> {typedef T _;};
}
template<typename T> using TRemoveReference = typename D::TRemoveReference_<T>::_;

template<typename T> constexpr forceinline TRemoveReference<T>&& Move(T&& t) noexcept
{return static_cast<TRemoveReference<T>&&>(t);}

template<typename T> struct CLValueReferenceT: FalseType {};
template<typename T> struct CLValueReferenceT<T&>: TrueType {};

template<typename T> constexpr forceinline T&& Forward(TRemoveReference<T>& t) noexcept
{return static_cast<T&&>(t);}

template<typename T> constexpr forceinline T&& Forward(TRemoveReference<T>&& t) noexcept
{
	static_assert(!CLValueReferenceT<T>::_, "Bad Forward call!");
	return static_cast<T&&>(t);
}

template<typename T> constexpr forceinline void Swap(T&& a, T&& b)
{
	if(&a == &b) return;
	auto temp = Move(a);
	a = Move(b);
	b = Move(temp);
}

template<typename T, typename U=T> constexpr forceinline T Exchange(T& dst, U&& newValue)
{
	T oldValue = Move(dst);
	dst = Forward<U>(newValue);
	return oldValue;
}

constexpr const struct {template<typename T> const auto& operator=(const T&) const {return *this;}} _;

#define INTRA_DEFAULT_DECL

/** Use Owner<T*> to explicitly show that the pointer owns its data.

  Only one pointer can own an object. It can be assigned to other Owner but the previous Owner must be reset.
  @see gsl::owner and its description in C++ Core Guidelines for more information.
*/
template<typename T> using Owner = T;

/** Use NotNull<T*> to explicitly show that the pointer must not be null.

  When INTRA_DEBUG_ABI is defined:
  An attempt to pass null (nullptr) or NULL directly will result in a compile time error.
  An attempt to pass null pointer in runtime will call debugger.

  Otherwise it is just an alias for underlying pointer.
*/
#ifndef INTRA_DEBUG_ABI
template<typename T> using NotNull = T;
#else
template<typename T> struct NotNull
{
	NotNull(int) = delete;
	NotNull(null_t) = delete;
	constexpr forceinline NotNull(T ptr): mPtr((((ptr != null) && (INTRA_DEBUG_BREAK, 0)), ptr)) {}
	constexpr forceinline operator T() const {return mPtr;}
private:
	T mPtr;
};
#endif

INTRA_END

typedef Intra::intptr ptrdiff_t;
typedef Intra::uintptr size_t;

INTRA_BEGIN
#ifdef _MSC_VER
__pragma(warning(disable: 4100 4577))
#endif

constexpr forceinline bool IsConstantEvaluated() noexcept
{
#ifdef INTRA_IS_CONSTANT_EVALUATED_SUPPORT
	return __builtin_is_constant_evaluated();
#elif defined(INTRA_AGRESSIVE_CONSTEXPR)
	//If detection is not supported but constexpr is supported then assume constant evaluation by default. This may decrease runtime performance
	return true;
#else
	//If detection is not supported but constexpr is supported then assume runtime evaluation by default. Constexpr function call may fail to compile
	return false;
#endif
}

template<typename Arg0, typename... Args> constexpr forceinline bool IsConstantEvaluated(Arg0&& arg0, Args&&... args) noexcept
{
#ifdef INTRA_IS_CONSTANT_EVALUATED_SUPPORT
	return __builtin_is_constant_evaluated();
#elif defined(INTRA_BUILTIN_CONSTANT_P_SUPPORT)
	return __builtin_constant_p(arg0) && IsConstantEvaluated(args...);
#elif defined(INTRA_AGRESSIVE_CONSTEXPR)
	//If detection is not supported then assume constant evaluation by default. This may decrease runtime performance
	return true;
#else
	//If detection is not supported then assume runtime evaluation by default. Constexpr function call may fail to compile
	return false;
#endif
}

#ifdef _MSC_VER
#define INTRA_DEBUG_BREAK __debugbreak()
#elif defined(__GNUC__)
#define INTRA_DEBUG_BREAK __builtin_trap()
#else
#include <signal.h>
#define INTRA_DEBUG_BREAK raise(SIGTRAP)
#endif

#ifdef _DEBUG

#ifndef INTRA_DEBUG
#define INTRA_DEBUG
#endif

#ifndef INTRA_DEBUG_ABI
#define INTRA_DEBUG_ABI
#endif

#define INTRA_DEBUG_ALLOCATORS

#endif

static constexpr const struct TConstruct {} Construct{};
template<typename T> struct TConstructT {};
template<typename T> static constexpr const TConstructT<T> ConstructT{};

INTRA_END

forceinline void* operator new(size_t, Intra::TConstruct, void* dst) {return dst;}
forceinline void operator delete(void*, Intra::TConstruct, void*) noexcept {}

#ifndef INTRA_INCLUDE_INITIALIZER_LIST_HEADER
INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//To avoid including heavy header file define initializer list here
//(according to profiling in MSVC it saves 18 ms per each compilation unit not including any non-Intra header)
#if !defined(_INITIALIZER_LIST_) && !defined(_LIBCPP_INITIALIZER_LIST) && !defined(_INITIALIZER_LIST)
#define _INITIALIZER_LIST_ //include guard for MSVC standard library
#define _LIBCPP_INITIALIZER_LIST //include guard for libc++
#define _INITIALIZER_LIST //include guard for libstdc++
//If you have a redefinition error for some compiler/lib you probably need to add more checks and more #defines
//or just compile without this optimizationn using -D INTRA_INCLUDE_INITIALIZER_LIST_HEADER

namespace std {

template<class T> class initializer_list
{
public:
	using value_type = T;
	using reference = const T&;
	using const_reference = const T&;
	using size_type = size_t;

	using iterator = const T*;
	using const_iterator = const T*;

	constexpr forceinline initializer_list() noexcept = default;
	constexpr forceinline initializer_list(const T* begin, const T* end) noexcept: mBegin(begin), mLength(size_t(end-begin)) {}

	INTRA_NODISCARD constexpr forceinline const T* begin() const noexcept {return mBegin;}
	INTRA_NODISCARD constexpr forceinline const T* end() const noexcept {return mBegin + mLength;}
	INTRA_NODISCARD constexpr forceinline size_t size() const noexcept {return mLength;}

private: //this version works for all tested compilers, however MSVC version with two pointers _First and _Last doesn't work for GCC
	const T* mBegin{};
	size_t mLength{};
};

template<class T> constexpr INTRA_NODISCARD forceinline const T* begin(initializer_list<T> list) noexcept {return list.begin();}
template<class T> constexpr INTRA_NODISCARD forceinline const T* end(initializer_list<T> list) noexcept {return list.end();}

}

#endif
INTRA_WARNING_POP
#else
INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <initializer_list>
INTRA_WARNING_POP
#endif

INTRA_BEGIN
template<typename T> using InitializerList = std::initializer_list<T>;
INTRA_END

#ifndef INTRA_MINEXE
//! Define INTRA_MINEXE to:
//0) default value
//1) to disable some not very important manual optimizations (slight slowdown)
//2) to disable some important manual optimizations (moderate slowdown)
//3) to allow pessimizations to reduce exe size by all means (not recommended - this can even increase algorithmic complexity)
#define INTRA_MINEXE 0
#endif
