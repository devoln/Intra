#pragma once

/**
  This header contains:
  1. compiler independent wrappers around compiler extensions and intrinsics;
  2. architecture detection;
  3. compiler independent warning management macros;
  4. implementation of a placement new (with slightly different syntax) without including any standard headers.
*/

#if (__cplusplus < 201703 || !defined(__cpp_concepts)) && (!defined(_MSVC_LANG) || _MSVC_LANG < 201704 || _MSC_VER < 1928)
static_assert(false, "Unsupported compiler configuration. Supported configurations: "
	"GCC 10 (-std=c++20), "
	"Clang 10 (-std=c++20), "
	"MSVC 2019.8 (/std:c++latest), "
	"and above");
#endif

#if !defined(__GNUC__) && !defined(_MSC_VER)
static_assert(false, "Unrecognized compiler!");
#endif

#if defined(__GNUC__) || defined(__clang__)
#define INTRA_MAY_ALIAS __attribute__((__may_alias__))
#define INTRA_FORCEINLINE inline __attribute__((always_inline))
#define INTRA_ARTIFICIAL [[gnu::artificial]]
#define INTRA_OPTIMIZE_FUNCTION_END
#define INTRA_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define INTRA_ASSUME_ALIGNED(ptr, alignmentBytes) static_cast<decltype(+ptr)>(__builtin_assume_aligned(ptr, alignmentBytes))
#define INTRA_GNU_EXT_CODE(...) __VA_ARGS__
#define INTRA_NON_GNU_EXT_CODE(...)
#define INTRA_MSVC_EXT_CODE(...)
#define INTRA_GNU_EXTENSION_SUPPORT
#else
#define INTRA_MAY_ALIAS
#define INTRA_ARTIFICIAL
#define INTRA_NON_GNU_EXT_CODE(...) __VA_ARGS__
#define INTRA_GNU_EXT_CODE(...)
#define INTRA_MSVC_EXT_CODE(...) __VA_ARGS__
#endif

#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#define INTRA_MATH_CONSTEXPR constexpr
#define INTRA_MATH_CONSTEXPR_SUPPORT
#define INTRA_NO_VECTORIZE_FUNC __attribute__((optimize("no-tree-vectorize")))
#else
#define INTRA_MATH_CONSTEXPR
#define INTRA_NO_VECTORIZE_FUNC
#endif

#if !defined(__INTEL_COMPILER) && (defined(__clang__) || defined(__GNUC__) && __GNUC__ >= 11 || defined(_MSC_VER))
#define INTRA_CONSTEXPR_BITCAST_SUPPORT
#endif

#ifdef __GNUC__
#define INTRA_ENABLE_COMPILER_OPTIMIZATIONS
#define INTRA_DISABLE_COMPILER_OPTIMIZATIONS
#define INTRA_DEFAULT_COMPILER_OPTIMIZATIONS
#endif

#ifdef __clang__
#define INTRA_OPTIMIZE_FUNCTION(...) __VA_ARGS__
#define INTRA_OPT_UNROLL_LOOP _Pragma(clang loop unroll(enable))
#elif defined(__GNUC__)
#define INTRA_OPTIMIZE_FUNCTION(...) __VA_ARGS__ __attribute__((flatten, optimize("O3")))
#define INTRA_OPT_UNROLL_LOOP
#elif defined(_MSC_VER)
#ifdef _M_AMD64
#define INTRA_ENABLE_COMPILER_OPTIMIZATIONS __pragma(optimize("gt", on))
#else
#define INTRA_ENABLE_COMPILER_OPTIMIZATIONS __pragma(optimize("gty", on)) 
#endif
#define INTRA_DISABLE_COMPILER_OPTIMIZATIONS __pragma(optimize("", off))
#define INTRA_DEFAULT_COMPILER_OPTIMIZATIONS __pragma(optimize("", on))
#define INTRA_OPTIMIZE_FUNCTION(...) INTRA_ENABLE_COMPILER_OPTIMIZATIONS __VA_ARGS__
#define INTRA_OPTIMIZE_FUNCTION_END INTRA_DEFAULT_COMPILER_OPTIMIZATIONS
#define INTRA_OPT_UNROLL_LOOP
#define INTRA_LIKELY(expr) !!(expr)
#define INTRA_ASSUME_ALIGNED(ptr, alignmentBytes) ptr
#define INTRA_FORCEINLINE __forceinline
#endif

#ifdef __cpp_modules
#define INTRA_EXPORT export
#define INTRA_EXPORT_MODULE(name) export module name
#else
#define INTRA_EXPORT
#define INTRA_EXPORT_MODULE(name)
#endif

#define INTRA_UTIL_INLINE INTRA_ARTIFICIAL INTRA_FORCEINLINE

#ifdef _MSC_VER
#define __builtin_trap() __debugbreak()
#define INTRA_COMPILER_ASSUME(hint) __assume(hint)
#define INTRA_CURRENT_FUNCTION __FUNCSIG__
#define INTRA_EMPTY_BASES __declspec(empty_bases)
#define INTRA_NOVTABLE  __declspec(novtable)
#define INTRA_CRTDECL __cdecl //Used to import functions from CRT without including its headers
#define INTRA_CRTRESTRICT __declspec(restrict)
#define INTRA_NO_VECTORIZE_LOOP __pragma(loop(no_vector))

// Pass "/Ob1 -DINTRA_CONFIG_MSVC_INLINE_ONLY_FORCEINLINE" to the compiler
// in order to get better performance wuthout losing code debugability.
#ifdef INTRA_CONFIG_MSVC_INLINE_ONLY_FORCEINLINE
#pragma inline_depth(0)
#endif

#else
#define INTRA_COMPILER_ASSUME(hint) INTRA_LIKELY(hint)
#define INTRA_CURRENT_FUNCTION __PRETTY_FUNCTION__
#define INTRA_EMPTY_BASES
#define INTRA_NOVTABLE
#define INTRA_CRTDECL
#define INTRA_CRTRESTRICT
#define INTRA_NO_VECTORIZE_LOOP
#endif

#if defined(_MSC_VER) && defined(_DLL)
#define INTRA_CRTIMP __declspec(dllimport)
#else
#define INTRA_CRTIMP
#endif

#if defined(_MSC_VER) && defined(__SSE2__)
#define INTRA_VECTORCALL __vectorcall
#else
#define INTRA_VECTORCALL
#endif

#ifndef INTRA_CONSTEXPR_TEST // To disable constexpr tests, compile with -D INTRA_CONSTEXPR_TEST=0
#define INTRA_CONSTEXPR_TEST __cpp_constexpr
#endif

#ifdef __cpp_constexpr_dynamic_alloc
#define INTRA_CONSTEXPR_DESTRUCTOR constexpr
#else
#define INTRA_CONSTEXPR_DESTRUCTOR
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#define INTRA_DLL_IMPORT __declspec(dllimport)
#define INTRA_DLL_EXPORT __declspec(dllexport)
#define INTRA_DLL_LOCAL
#elif defined(__GNUC__)
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

// Implement GCC CPU architecture defines for MSVC. We implement only __<arch>__ form
#if !defined(__i386__) && defined(_M_IX86)
#define __i386__ 1
#elif !defined(__amd64__) && defined(_M_AMD64)
#define __amd64__ 1
#elif !defined(__arm__) && defined(_M_ARM)
#define __arm__ 1
#elif !defined(__aarch64__) && defined(_M_ARM64)
#define __aarch64__ 1
#endif

#ifdef __BYTE_ORDER__
static_assert(__BYTE_ORDER__ != __ORDER_PDP_ENDIAN__);
#endif

#define INTRA_PREPROCESSOR_QUOTE(x) #x
#define INTRAZ_D_CONCATENATE_TOKENS(x, y) x ## y
#define INTRA_CONCATENATE_TOKENS(x, y) INTRAZ_D_CONCATENATE_TOKENS(x, y)

#if defined(__GNUC__) || defined(__clang__)
#define INTRA_WARNING_PUSH _Pragma("GCC diagnostic push")
#define INTRA_WARNING_POP _Pragma("GCC diagnostic pop")
#define INTRAZ_D_WARNING_HELPER(mode, x) INTRA_PREPROCESSOR_QUOTE(GCC diagnostic mode x)
#define INTRAZ_D_IGNORE_WARNING_HELPER(x) INTRAZ_D_WARNING_HELPER(ignored, x)
#define INTRA_IGNORE_WARN(x) _Pragma(INTRAZ_D_IGNORE_WARNING_HELPER("-W" x))
#define INTRAZ_D_ENABLE_WARNING_HELPER(mode, x) INTRAZ_D_WARNING_HELPER(mode, x)
#define INTRA_ENABLE_ERROR(x) _Pragma(INTRAZ_D_ENABLE_WARNING_HELPER(error, "-W" x))
#define INTRA_ENABLE_WARNING(x) _Pragma(INTRAZ_D_ENABLE_WARNING_HELPER(warning, "-W" x))

#define INTRAZ_D_USEFUL_WARNINGS_GCC_CLANG(W) \
	W("all") \
	W("extra") \
	W("old-style-cast") \
	W("conversion") \
	W("sign-conversion") \
	W("init-self") \
	W("unreachable-code") \
	W("pointer-arith") \
	W("pedantic") \
	W("non-virtual-dtor") \
	W("effc++") \
	W("shadow") \
	W("unused-variable") \
	W("unused-parameter")
//-Wshadow=local?
#else
#define INTRA_IGNORE_WARN(x)
#define INTRA_ENABLE_WARNING(x)
#define INTRAZ_D_DISABLE_REDUNDANT_WARNINGS_GCC_CLANG
#define INTRAZ_D_USEFUL_WARNINGS_GCC_CLANG(W)
#ifdef _MSC_VER
#define INTRA_WARNING_PUSH __pragma(warning(push))
#define INTRA_WARNING_POP __pragma(warning(pop))
#else
#define INTRA_WARNING_PUSH
#define INTRA_WARNING_POP
#endif
#endif

#ifdef INTRA_WARNINGS_AS_ERRORS
#define INTRA_ENABLE_WARNING_OR_ERROR INTRA_ENABLE_ERROR
#else
#define INTRA_ENABLE_WARNING_OR_ERROR INTRA_ENABLE_WARNING
#endif

#ifdef __clang__
#define INTRA_IGNORE_WARN_CLANG(x) INTRA_IGNORE_WARN(x)
#else
#define INTRA_IGNORE_WARN_CLANG(x)
#ifdef __GNUC__
#define INTRA_IGNORE_WARN_GCC(name) INTRA_IGNORE_WARN(name)
#define INTRAZ_D_USEFUL_WARNINGS_GCC(W) \
	W("duplicated-cond") \
	W("null-dereference") \
	W("duplicated-branches") \
	W("restrict") \
	W("logical-op") \
	W("useless-cast")
#else
#define INTRA_IGNORE_WARN_GCC(name)
#define INTRAZ_D_USEFUL_WARNINGS_GCC(W)
#endif
#endif

#ifdef _MSC_VER
#define INTRA_IGNORE_WARNS_MSVC(w) __pragma(warning(disable: w))
#define INTRA_PUSH_DISABLE_ALL_WARNINGS __pragma(warning(push, 0)) INTRAZ_D_IGNORE_MOST_WARNINGS
#define INTRAZ_D_USEFUL_WARNINGS_MSVC(mode) \
	__pragma(warning(mode: 4365 4244 4702 4265 4061 4625 4512 5026 5027 4510 4610 \
		4623 5027 4127 4571 4626 4548 4987 4774 4702 4355 4738))
#else
#define INTRA_IGNORE_WARNS_MSVC(w)
#define INTRAZ_D_USEFUL_WARNINGS_MSVC(mode)
#define INTRA_PUSH_DISABLE_ALL_WARNINGS INTRA_WARNING_PUSH INTRAZ_D_IGNORE_MOST_WARNINGS
#endif

#define INTRA_ENABLE_USEFUL_WARNINGS \
	INTRAZ_D_USEFUL_WARNINGS_GCC_CLANG(INTRA_ENABLE_WARNING_OR_ERROR) \
	INTRAZ_D_USEFUL_WARNINGS_GCC(INTRA_ENABLE_WARNING_OR_ERROR) \
	INTRAZ_D_USEFUL_WARNINGS_MSVC(default)

#if defined(_MSC_VER) && !defined(__clang__)
#define INTRA_IGNORE_WARN_LNK4221 namespace {char INTRA_CONCATENATE_TOKENS($DisableLNK4221_, __COUNTER__);}
#define INTRA_PUSH_ENABLE_USEFUL_WARNINGS __pragma(warning(push, 4)) INTRA_ENABLE_USEFUL_WARNINGS
#else
#define INTRA_IGNORE_WARN_LNK4221
#define INTRA_PUSH_ENABLE_USEFUL_WARNINGS INTRA_WARNING_PUSH INTRA_ENABLE_USEFUL_WARNINGS
#endif

#define INTRA_IGNORE_WARN_UNUSED_FUNCTION INTRA_IGNORE_WARN("unused-function")
#define INTRA_IGNORE_WARN_UNHANDLED_ENUM_CASES INTRA_IGNORE_WARNS_MSVC(4061)
#define INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED INTRA_IGNORE_WARNS_MSVC(4625 4626 4512)
#define INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED INTRA_IGNORE_WARNS_MSVC(4510 4610 4623)
#define INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED INTRA_IGNORE_WARNS_MSVC(4626 5027)
#define INTRA_IGNORE_WARN_CONSTANT_CONDITION INTRA_IGNORE_WARNS_MSVC(4127)
#define INTRA_IGNORE_WARN_SIGN_CONVERSION \
	INTRA_IGNORE_WARN("sign-conversion") INTRA_IGNORE_WARNS_MSVC(4365)
#define INTRA_IGNORE_WARN_SIGN_COMPARE \
	INTRA_IGNORE_WARN("sign-compare") INTRA_IGNORE_WARNS_MSVC(4018)
#define INTRA_IGNORE_WARN_LOSING_CONVERSION \
	INTRA_IGNORE_WARN("conversion") INTRA_IGNORE_WARNS_MSVC(4244)
#define INTRA_IGNORE_WARN_NO_VIRTUAL_DESTRUCTOR \
	INTRA_IGNORE_WARN("non-virtual-dtor") INTRA_IGNORE_WARNS_MSVC(4265)
#define INTRA_IGNORE_WARN_LANGUAGE_EXTENSION \
	INTRA_IGNORE_WARN_CLANG("language-extension-token") INTRA_IGNORE_WARNS_MSVC(4201)
#define INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION \
	INTRA_IGNORE_WARN_CLANG("exit-time-destructors") \
	INTRA_IGNORE_WARN_CLANG("global-constructors")
#define INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED \
	INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED \
	INTRA_IGNORE_WARNS_MSVC(5026 5027)

#define INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED \
	INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED \
	INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED

#define INTRA_DISABLE_REDUNDANT_WARNINGS \
	INTRA_IGNORE_WARN("ctor-dtor-privacy") \
	INTRA_IGNORE_WARN_CLANG("logical-op-parentheses") \
	INTRA_IGNORE_WARN_CLANG("c++98-compat") \
	INTRA_IGNORE_WARN_CLANG("c++98-compat-pedantic") \
	INTRA_IGNORE_WARN("c++11-compat") \
	INTRA_IGNORE_WARN_CLANG("documentation-unknown-command") \
	INTRA_IGNORE_WARN_CLANG("comma") \
	INTRA_IGNORE_WARN_CLANG("duplicate-enum") \
	INTRA_IGNORE_WARN_CLANG("implicit-fallthrough") \
	INTRA_IGNORE_WARN_CLANG("float-equal") \
	INTRA_IGNORE_WARN_CLANG("reserved-id-macro") \
	INTRA_IGNORE_WARNS_MSVC(4514 4710 4714 4820 4711 4577 4868 5045)
//4514 - unused inline function
//4710 - function not inlined
//4714 - cannot inline __forceinline function
//4820 - bytes padding added after construct
//4711 - function selected for inline expansion
//4577 - noexcept when compiling with exceptions disabled
//4868 - compiler may not enforce left-to-right evaluation order in braced initializer list
//5045 - compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

#define INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS INTRA_WARNING_PUSH INTRA_DISABLE_REDUNDANT_WARNINGS

#define INTRAZ_D_IGNORE_MOST_WARNINGS \
	INTRA_IGNORE_WARNS_MSVC(4548 4987 4774 4702 4355 4738 4571) \
	INTRAZ_D_USEFUL_WARNINGS_GCC_CLANG(INTRA_IGNORE_WARN) \
	INTRAZ_D_USEFUL_WARNINGS_GCC(INTRA_IGNORE_WARN) \
	INTRA_IGNORE_WARN_CLANG("undefined-reinterpret-cast") \
	INTRA_DISABLE_REDUNDANT_WARNINGS \
	INTRA_IGNORE_WARN_LOSING_CONVERSION \
	INTRA_IGNORE_WARN_SIGN_CONVERSION \
	INTRA_IGNORE_WARN_SIGN_COMPARE \
	INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED \
	INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION \
	INTRA_IGNORE_WARN_LANGUAGE_EXTENSION

#ifdef _DEBUG
#ifndef INTRA_DEBUG
#define INTRA_DEBUG 1
#endif
#ifndef INTRA_DEBUG_ABI
#define INTRA_DEBUG_ABI
#endif
/** Store some additional debug info in allocated blocks.
	Warning: breaks ABI.
*/
#define INTRA_DEBUG_ALLOCATORS
#else
#ifndef INTRA_DEBUG
#define INTRA_DEBUG 0
#endif
#endif

using size_t = decltype(sizeof(0));

#define INTRA_BEGIN namespace Intra { INTRA_PUSH_ENABLE_USEFUL_WARNINGS INTRA_DISABLE_REDUNDANT_WARNINGS
#define INTRA_END INTRA_WARNING_POP }

INTRA_BEGIN
using byte = unsigned char;
using int8 = signed char;
using uint8 = byte;
using int16 = short;
using uint16 = unsigned short;
using int64 = long long;
using uint64 = unsigned long long;
static_assert(sizeof(int8) == 1 && sizeof(int16) == 2 && sizeof(int64) == 8 && sizeof(float) == 4);

#if !defined(__SIZEOF_DOUBLE__) || __SIZEOF_DOUBLE__ == 8
using float64 = double; //not available on some platforms (Arduino Uno)
#endif

#if defined(__SIZEOF_INT__) && __SIZEOF_INT__ == 2
#if defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 4
using int32 = long;
using uint32 = unsigned long;
#endif
#else
using int32 = int;
using uint32 = unsigned;
#endif

/// Signed integral type with size dependent on pointer size. Used for index and size arithmetics.
using index_t = decltype(reinterpret_cast<char*>(1)-reinterpret_cast<char*>(0));

constexpr auto null = nullptr; /// Used with null pointers and empty objects
constexpr struct TUnsafe {} Unsafe;
constexpr struct TUndefined {} Undefined;

#ifdef __SIZEOF_INT128__
using int128 = __int128;
using uint128 = unsigned __int128;
#endif

#define INTRA_MOVE(...) static_cast<::Intra::TRemoveReference<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define INTRA_FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

constexpr bool IsConstantEvaluated() noexcept {return __builtin_is_constant_evaluated();}

constexpr struct {} Construct;

INTRA_IGNORE_WARN_GCC("effc++")
constexpr struct {template<typename T> auto& operator=(T&) const {return *this;}} _;

namespace Config {
constexpr bool TargetIsBigEndian =
#ifdef __BYTE_ORDER__
	__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__;
#else
	false;
#endif

constexpr bool TargetIsFloatBigEndian =
#ifdef __FLOAT_WORD_ORDER__
	__FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__;
#else
	TargetIsBigEndian;
#endif

/** Defines which debug checks should be included into the program.
	0 - no checks
	1 - only checks with time and memory complexity O(1)
	2 - checks with time and memory complexity up to O(N)
*/
constexpr int DebugCheckLevel = INTRA_DEBUG;

#ifndef INTRA_MINIMIZE_CODE_SIZE
/// Define INTRA_MINIMIZE_CODE_SIZE to:
/// - 0: default value - all optimizations and error checks are enabled
/// - 1: disable large code size footprint optimizations (fast integer parsing, manual loop unrolling, complex SIMD)
/// - 2: disable all optimizations even low code size footprint (all SIMD, early exit branches)
/// - 3: disable troubleshooting (logging, error messages)
/// - 4: reduce code size by all means - not for production. May be useful for microcontrollers or demoscene.
///   This can even increase algorithmic complexity (e.g. using arrays everywhere) and
///   sacrifice correctness (low precision or reduced range math, omitting most error checks)
#define INTRA_MINIMIZE_CODE_SIZE 0
#endif
constexpr bool DisableLargeCodeSizeFootprintOptimizations = INTRA_MINIMIZE_CODE_SIZE >= 1;
constexpr bool DisableAllOptimizations = INTRA_MINIMIZE_CODE_SIZE >= 2;
constexpr bool DisableTroubleshooting = INTRA_MINIMIZE_CODE_SIZE >= 3;
constexpr bool ReduceCodeSizeByAllMeans = INTRA_MINIMIZE_CODE_SIZE >= 4;
constexpr bool ReplaceCodeWithOsFunctions = ReduceCodeSizeByAllMeans; //Make sure to use dynamic linking with OS built-in CRT
}

INTRA_END

inline void* operator new(size_t, decltype(Intra::Construct), void* dst) {return dst;}
inline void operator delete(void*, decltype(Intra::Construct), void*) noexcept {}
