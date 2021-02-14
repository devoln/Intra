#pragma once

#if (__cplusplus < 201703 || !defined(__cpp_concepts)) && (!defined(_MSVC_LANG) || _MSVC_LANG < 201704 || _MSC_VER < 1928)
static_assert(false, "Unsupported compiler configuration. Supported configurations: "
	"GCC 10 (-std=c++20), "
	"Clang 10 (-std=c++20), "
	"MSVC 2019.8 (/std:c++latest), "
	"and above");
#endif

#ifdef __BYTE_ORDER__
static_assert(__BYTE_ORDER__ != __ORDER_PDP_ENDIAN__, "Intra doesn't support this architecture");
#endif

/// Useful to organize code in foldable sections.
/// It is a warning-free alternative to MSVC's #pragma region for all compilers
#define INTRA_CODE_SECTION 1

//// <Compiler specific features>
#if INTRA_CODE_SECTION

#if !defined(__GNUC__) && !defined(_MSC_VER)
static_assert(false, "Unrecognized compiler!");
#endif

#if defined(__GNUC__) || defined(__clang__)
#define INTRA_MAY_ALIAS __attribute__((__may_alias__))
#define INTRA_FORCEINLINE inline __attribute__((always_inline))
#define INTRA_FORCEINLINE_LAMBDA __attribute__((always_inline))
#define INTRA_ARTIFICIAL [[gnu::artificial]]
#define INTRA_OPTIMIZE_FUNCTION_END
#define INTRA_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define INTRA_ASSUME_ALIGNED(ptr, alignmentBytes) static_cast<decltype(+ptr)>(__builtin_assume_aligned(ptr, alignmentBytes))
#define INTRA_GNU_EXT_CODE(...) __VA_ARGS__
#define INTRA_NON_GNU_EXT_CODE(...)
#define INTRA_MSVC_EXT_CODE(...)
#define INTRA_GNU_EXTENSION_SUPPORT
#define INTRA_CRTDECL
#else
#define INTRA_MAY_ALIAS
#define INTRA_ARTIFICIAL
#define INTRA_NON_GNU_EXT_CODE(...) __VA_ARGS__
#define INTRA_GNU_EXT_CODE(...)
#define INTRA_MSVC_EXT_CODE(...) __VA_ARGS__

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

#ifndef __SSE__
#if defined(_M_IX86_FP) && _M_IX86_FP >= 2 || defined(__amd64__)
#define __SSE2__ 1
#define __SSE__ 1
#elif defined(_M_IX86_FP) && _M_IX86_FP == 1
#define __SSE__ 1
#endif
#ifdef __AVX__
#define __SSE3__ 1
#define __SSSE3__ 1
#define __SSE4_1__ 1
#define __SSE4_2__ 1
#endif
#endif

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
#define INTRA_FORCEINLINE_LAMBDA
#define __builtin_trap() __debugbreak()
#endif

#define INTRA_UTIL_INLINE INTRA_ARTIFICIAL INTRA_FORCEINLINE

#ifdef _MSC_VER
#define INTRA_CURRENT_FUNCTION __FUNCSIG__
#define INTRA_EMPTY_BASES __declspec(empty_bases)
#define INTRA_NOVTABLE  __declspec(novtable)
#define INTRA_CRTRESTRICT __declspec(restrict)
#define INTRA_NO_VECTORIZE_LOOP __pragma(loop(no_vector))
#define INTRA_INLINE_REQUIRES_EXPR(type, ...) CombineOverloads{[]<typename T>(T*) -> decltype((__VA_ARGS__), true) {return true;}, retFalse}(static_cast<type*>(nullptr))

// Pass "/Ob1 -DINTRA_CONFIG_MSVC_INLINE_ONLY_FORCEINLINE" to the compiler
// in order to get better performance wuthout losing code debugability.
#ifdef INTRA_CONFIG_MSVC_INLINE_ONLY_FORCEINLINE
#pragma inline_depth(0)
#endif

#else
#define INTRA_CURRENT_FUNCTION __PRETTY_FUNCTION__
#define INTRA_EMPTY_BASES
#define INTRA_NOVTABLE
#define INTRA_CRTRESTRICT
#define INTRA_NO_VECTORIZE_LOOP
#define INTRA_INLINE_REQUIRES_EXPR(type, ...) requires {__VA_ARGS__;}
#endif

#if defined(_WIN32) && defined(__i386__)
#define INTRA_CRTDECL __cdecl //Used to import functions from CRT without including its headers
#define INTRA_WINAPI __stdcall
#else
#define INTRA_CRTDECL
#define INTRA_WINAPI
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

#if defined(_MSC_VER) && defined(_DLL)
#define INTRA_CRTIMP INTRA_DLL_IMPORT
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

#if !defined(_MSC_VER) || _MSC_VER >= 1929
#define INTRA_CONSTEXPR_VIRTUAL constexpr
#else
#define INTRA_CONSTEXPR_VIRTUAL
#endif

#ifdef __cpp_modules
#define INTRA_EXPORT export
#define INTRA_EXPORT_MODULE(name) export module name
#else
#define INTRA_EXPORT
#define INTRA_EXPORT_MODULE(name)
#endif

#endif
//// </Compiler specific features>

#define INTRA_PREPROCESSOR_QUOTE(x) #x
#define INTRAZ_D_CONCATENATE_TOKENS(x, y) x ## y
#define INTRA_CONCATENATE_TOKENS(x, y) INTRAZ_D_CONCATENATE_TOKENS(x, y)

//// <Warning management>
#if INTRA_CODE_SECTION

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
	W("nullptr-dereference") \
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
	INTRA_IGNORE_WARNS_MSVC(4514 4710 4714 4820 4711 4577 4868 5045   4648 /*TODO: remove this when [[no_unique_address]] is supported*/)
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

#endif
//// </Warning management>

using size_t = decltype(sizeof(0));

#define INTRA_BEGIN INTRA_PUSH_ENABLE_USEFUL_WARNINGS INTRA_DISABLE_REDUNDANT_WARNINGS
#define INTRA_END INTRA_WARNING_POP

/// All Intra libraries define everything in this namespace
namespace Intra { INTRA_BEGIN
//// <Basic type definitions and build config variables>
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

#ifdef __wasm__ // wasm has 64-bit instructions and is likely to run on 64-bit systems
using WidestFastInt = uint64;
#else
using WidestFastInt = size_t;
#endif

/// Signed integral type with size dependent on pointer size. Used for index and size arithmetics.
using index_t = decltype(static_cast<char*>(nullptr) - static_cast<char*>(nullptr));

#ifndef __cpp_char8_t
using char8_t = char;
#endif

constexpr struct TUnsafe {} Unsafe;
constexpr struct TUndefined {} Undefined;

#ifdef __SIZEOF_INT128__
using int128 = __int128;
using uint128 = unsigned __int128;
#endif

#define INTRA_MOVE(...) static_cast<::Intra::TRemoveReference<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define INTRA_FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

[[nodiscard]] INTRA_UTIL_INLINE constexpr bool IsConstantEvaluated() noexcept {return __builtin_is_constant_evaluated();}
constexpr auto AddressOf = [](auto&& arg) noexcept {return __builtin_addressof(arg);};

[[noreturn]] INTRA_UTIL_INLINE void UnreachableCode(TUnsafe)
{
#ifdef __GNUC__
	__builtin_unreachable();
#else
	__assume(false);
#endif
}

namespace z_D {
// May be extended later to support other types by adding function overloads.
template<typename T1, typename T2> requires(requires(T1 a, T2 b) {a < b? a: b;})
constexpr auto Min_(T1 a, T2 b) {return a < b? a: b;}

template<typename T1, typename T2> requires(requires(T1 a, T2 b) {a > b? a: b;})
constexpr auto Max_(T1 a, T2 b) {return a > b? a: b;}
}

constexpr auto Min = [](auto&& a, auto&&... args)
{
	if constexpr(sizeof...(args) == 0) return INTRA_FWD(a);
	else if constexpr(sizeof...(args) == 1) return z_D::Min_(INTRA_FWD(a), INTRA_FWD(args)...);
	else return operator()(INTRA_FWD(a), operator()(INTRA_FWD(args)...));
};
constexpr auto Max = [](auto&& a, auto&&... args)
{
	if constexpr(sizeof...(args) == 0) return a;
	else if constexpr(sizeof...(args) == 1) return z_D::Max_(a, args...);
	else return operator()(a, operator()(args...));
};

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

enum class OperatingSystem {Windows, Linux, Android, FreeBSD, Emscripten, IOS, MacOS, Unknown};

constexpr OperatingSystem TargetOS = OperatingSystem::
#ifdef _WIN32
	Windows;
#elif defined(__linux__)
	Linux;
#elif defined(__ANDROID__)
	Android;
#elif defined(__FreeBSD__)
	FreeBSD;
#elif defined(__EMSCRIPTEN__)
	Emscripten;
#elif defined(__APPLE__)
#ifdef __MACH__
    MacOS;
#else
    IOS;
#endif
#else
    Unknown;
#endif

#if defined(_MSC_VER) && !defined(__clang__)
//Visual Studio 2019 has no Windows XP toolkit version.
//Earlier versions are not supported by Intra.
#define INTRA_DROP_XP_SUPPORT
#endif

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

} INTRA_END
//// </Basic type definitions and build config variables>

inline void* operator new(size_t, decltype(Intra::Construct), void* dst) {return dst;}
inline void operator delete(void*, decltype(Intra::Construct), void*) noexcept {}

//// <Fundamental C++ type metafunctions and concepts>
namespace Intra { INTRA_BEGIN

template<typename T> T&& Val();

template<typename T> struct TConstructT {};
template<typename T> constexpr TConstructT<T> ConstructT;
template<size_t I> struct TConstructAt {};
template<size_t I> constexpr TConstructAt<I> ConstructAt;

template<typename> constexpr bool CConstructAt = false;
template<size_t I> constexpr bool CConstructAt<TConstructAt<I>> = true;

struct EmptyType
{
	EmptyType() = default;
	template<typename... Args> constexpr EmptyType(Args&&...) {}
};

template<typename T> struct TType {using _ = T;};
template<typename T> constexpr TType<T> Type;
template<auto Value> struct TValue {static constexpr auto _ = Value;};
template<size_t Value> using TIndex = TValue<Value>;

/// TTag is used by types to declare that they have some special trait:
/// ```using TagName = TTag<>;```
/// or with condition:
/// ```using TagName = TTag<constantBooleanExpression()>;```
/// that is easy to check with concepts: requires {T::TagName::True;}
template<bool V = true> struct TTag: TValue<false> {};
template<> struct TTag<true>: TValue<true> {enum {True};};

struct NonCopyableType
{
	NonCopyableType() = default;
	NonCopyableType(NonCopyableType&&) = default;
	NonCopyableType& operator=(NonCopyableType&&) = default;
	NonCopyableType(const NonCopyableType&) = delete;
	NonCopyableType& operator=(const NonCopyableType&) = delete;
};

namespace z_D {
template<typename T, typename... Ts> struct TPackFirst_ {using _ = T;};
}
template<typename... Ts> using TPackFirst = typename z_D::TPackFirst_<Ts...>::_;
#ifdef __clang__
template<size_t Index, typename... Ts>
using TPackAt = __type_pack_element<Index, Ts...>;
#else
namespace z_D {
template<size_t Index, typename T0, typename... Ts>
struct TPackAt_: TPackAt_<Index-1, Ts...> {static_assert(Index < 1 + sizeof...(Ts));};
template<typename T0, typename... Ts>
struct TPackAt_<0, T0, Ts...> {using _ = T0;};
}
template<size_t Index, typename... Ts>
using TPackAt = typename z_D::TPackAt_<Index, Ts...>::_;
#endif

template<size_t Index, auto... Values> constexpr auto VPackAt = TPackAt<Index, TValue<Values>...>::_;

template<typename T> constexpr bool CUnknownBoundArrayType = false;
template<typename T> constexpr bool CUnknownBoundArrayType<T[]> = true;
template<typename T> constexpr bool CKnownBoundArrayType = false;
template<typename T, size_t N> constexpr bool CKnownBoundArrayType<T[N]> = true;
template<typename T> concept CArrayType = CUnknownBoundArrayType<T> || CKnownBoundArrayType<T>;

namespace z_D {
template<typename T> struct TRemoveReference_ {using _ = T;};
template<typename T> struct TRemoveReference_<T&> {using _ = T;};
template<typename T> struct TRemoveReference_<T&&> {using _ = T;};
template<typename T> constexpr bool CLValueReference_ = false;
template<typename T> constexpr bool CLValueReference_<T&> = true;
template<typename T> constexpr bool CRValueReference_ = false;
template<typename T> constexpr bool CRValueReference_<T&&> = true;

template<typename T> constexpr bool CConst_ = false;
template<typename T> constexpr bool CConst_<const T> = true;
template<typename T> constexpr bool CVolatile_ = false;
template<typename T> constexpr bool CVolatile_<volatile T> = true;
}
template<typename T> using TRemoveReference = typename z_D::TRemoveReference_<T>::_;

template<typename T> concept CLValueReference = z_D::CLValueReference_<T>;
template<typename T> concept CRValueReference = z_D::CRValueReference_<T>;
template<typename T> concept CReference = CLValueReference<T> || CRValueReference<T>;

template<typename T> concept CConst = z_D::CConst_<T>;
template<typename T> concept CVolatile = z_D::CVolatile_<T>;
template<typename T> concept CNonConstLValueReference = CLValueReference<T> && !CConst<TRemoveReference<T>>;
template<typename T> concept CNonConstRValueReference = CRValueReference<T> && !CConst<TRemoveReference<T>>;

template<typename IfTrue, typename IfFalse, bool Condition> using TSelect = TPackAt<Condition, IfFalse, IfTrue>;
template<typename FieldType, bool Condition> using TConditionalField = TSelect<FieldType, EmptyType, Condition>;


template<bool Condition> using CopyableIf = TSelect<EmptyType, NonCopyableType, Condition>;

namespace z_D {
template<typename T> struct TRemovePointer_ {using _ = T;};
template<typename T> struct TRemovePointer_<T*> {using _ = T;};
template<typename T> struct TRemovePointer_<T* const> {using _ = T;};
template<typename T> struct TRemovePointer_<T* volatile> {using _ = T;};
template<typename T> struct TRemovePointer_<T* const volatile> {using _ = T;};

template<typename T> struct TRemoveConst_ {using _ = T;};
template<typename T> struct TRemoveConst_<const T> {using _ = T;};

template<typename T> struct TRemoveVolatile_ {using _ = T;};
template<typename T> struct TRemoveVolatile_<volatile T> {using _ = T;};
template<typename T> struct TRemoveVolatile_<volatile T[]> {using _ = T[];};
template<typename T, size_t N> struct TRemoveVolatile_<volatile T[N]> {using _ = T[N];};

template<typename T> struct TRemoveExtent_ {using _ = T;};
template<typename T, size_t N> struct TRemoveExtent_<T[N]> {using _ = T;};
template<typename T> struct TRemoveExtent_<T[]> {using _ = T;};

template<typename T> struct TExplicitType_ {using _ = T;};
}

template<typename T> using TRemovePointer = typename z_D::TRemovePointer_<T>::_;
template<typename T> using TRemoveConst = typename z_D::TRemoveConst_<T>::_;
template<typename T> using TRemoveVolatile = typename z_D::TRemoveVolatile_<T>::_;
template<typename T> using TRemoveConstRef = TRemoveConst<TRemoveReference<T>>;
template<typename T> using TUnqual = TRemoveConst<TRemoveVolatile<T>>;
template<typename T> using TUnqualRef = TUnqual<TRemoveReference<T>>;
template<typename T> using TRemoveExtent = typename z_D::TRemoveExtent_<T>::_;

/// Use this as function argument type to avoid automatic template type inference
template<typename T> using TExplicitType = typename z_D::TExplicitType_<T>::_;

//TODO: doesn't work if T is a cv- or ref-qualified function type
template<typename T> using TAddPointer = TRemoveReference<T>*;


namespace z_D {
template<typename... Ts> constexpr bool CSame_ = false;
template<typename T> constexpr bool CSame_<T> = true;
template<typename T> constexpr bool CSame_<T, T> = true;
template<typename T1, typename T2, typename T3, typename... Ts>
constexpr bool CSame_<T1, T2, T3, Ts...> = CSame_<T1, T2> && CSame_<T1, T3> && (CSame_<T1, Ts> && ...);
}
template<typename... Ts> concept CSame = z_D::CSame_<Ts...>;
template<typename... Ts> concept CSameUnqual = CSame<TUnqual<Ts>...>;
template<typename... Ts> concept CSameIgnoreRef = CSame<TRemoveReference<Ts>...>;
template<typename... Ts> concept CSameUnqualRef = CSameUnqual<TRemoveReference<Ts>...>;
template<typename... Ts> concept CSameNotVoid = CSame<Ts...> && !CSame<void, Ts...>;
template<typename T, typename... Ts> concept CAnyOf = (CSame<T, Ts> || ...);

template<typename T1, typename... Ts> concept CSameSize = ((sizeof(T1) == sizeof(Ts)) && ...);

namespace z_D {
template<typename T, bool = CSameUnqual<T, void>> struct TAddReference
{
	using LValue = T&;
	using RValue = T&&;
};
template<typename T> struct TAddReference<T, true>
{
	using LValue = void;
	using RValue = void;
};
}
template<typename T> using TAddLValueReference = typename z_D::TAddReference<T>::LValue;
template<typename T> using TAddRValueReference = typename z_D::TAddReference<T>::RValue;

namespace z_D {
template<typename T> constexpr bool CFunctionPointer_ = false;
#ifdef _MSC_VER
#ifdef __i386__
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(__cdecl*)(Args...)> = true;
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(__stdcall*)(Args...)> = true;
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(__fastcall*)(Args...)> = true;
#else
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(*)(Args...)> = true;
#endif
#if defined(__SSE2__)
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(__vectorcall*)(Args...)> = true;
#endif
#else
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(*)(Args...)> = true;
#endif
}
template<typename T> concept CFunctionPointer = z_D::CFunctionPointer_<T>;
template<typename T> concept CAbstractClass = __is_abstract(T);
template<typename T> concept CUnion = __is_union(T);
template<typename T> concept CClass = __is_class(T);
template<typename T> concept CEnum = __is_enum(T);
template<typename T> concept CEmpty = __is_empty(T);
template<typename T> concept CFinalClass = __is_final(T);
template<typename T> concept CTriviallyDestructible = __has_trivial_destructor(T);
template<class T, class From> concept CDerived = __is_base_of(From, T);
template<typename T> concept CHasVirtualDestructor = __has_virtual_destructor(T);
template<typename T> concept CPolymorphic = __is_polymorphic(T);
template<typename T> concept CFunction = !CReference<T> && !CConst<const T>;
template<typename T> concept CObject = CConst<const T> && !CVoid<T>; //== CScalar<T> || CArray<T> || CUnion<T> || CClass<T> == !CFunction<T> && !CReference<T> && !CVoid<T>
template<typename T> concept CAggregate = __is_aggregate(T);
template<typename T> concept CStandardLayout = __is_standard_layout(T);
template<typename T> concept CTriviallyCopyable = __is_trivially_copyable(T);
template<typename T, typename... Args> concept CTriviallyConstructible = __is_trivially_constructible(T, Args...);
template<typename T, typename Arg> concept CTriviallyAssignable = __is_trivially_assignable(T, Arg);
template<typename T> concept CTriviallyCopyAssignable = CTriviallyAssignable<TAddLValueReference<T>, TAddLValueReference<const T>>;
template<typename T> concept CTriviallyMoveAssignable = CTriviallyAssignable<TAddLValueReference<T>, TAddRValueReference<T>>;
template<typename T> concept CHasUniqueObjectRepresentations = __has_unique_object_representations(T);

template<typename T> using TUnderlyingType = __underlying_type(T);

namespace z_D {
template<typename T, template<typename...> class> constexpr bool CInstanceOfTemplate_ = false;
template<typename... Ts, template<typename...> class Template> constexpr bool CInstanceOfTemplate_<Template<Ts...>, Template> = true;
}
template<typename T, template<typename...> class Template> concept CInstanceOfTemplate = z_D::CInstanceOfTemplate_<T, Template>;


template<typename T> concept CUnqualedVoid = CSame<T, void>;
template<typename T> concept CUnqualedBasicUnsignedIntegral = CAnyOf<T,
	char16_t, char32_t, bool, uint8, uint16, unsigned, unsigned long, uint64,
	TSelect<char, unsigned, (char(~0) > 0)>,
	TSelect<wchar_t, unsigned, (wchar_t(~0) > 0)>
>;

template<typename T> concept CUnqualedBasicSignedIntegral = CAnyOf<T,
	int8, short, int, long, int64,
	TSelect<int, char, (char(~0) > 0)>,
	TSelect<int, wchar_t, (wchar_t(~0) > 0)>
>;

template<typename T> concept CUnqualedBasicFloatingPoint = CAnyOf<T, float, double, long double>;

/** note: signed char and unsigned char are not treated as character types.
	They are separate types from char because Intra doesn't use them to store characters.
*/
template<typename T> concept CUnqualedChar = CAnyOf<T, char, char16_t, char32_t, wchar_t
#ifdef __cpp_char8_t
	, char8_t
#endif
>;

template<typename T> concept CUnqualedBasicIntegral = CUnqualedBasicUnsignedIntegral<T> || CUnqualedBasicSignedIntegral<T>;
template<typename T> concept CUnqualedBasicSigned = CUnqualedBasicSignedIntegral<T> || CUnqualedBasicFloatingPoint<T>;
template<typename T> concept CUnqualedBasicArithmetic = CUnqualedBasicIntegral<T> || CUnqualedBasicFloatingPoint<T>;

template<typename T> constexpr bool CUnqualedBasicPointer = false;
template<typename T> constexpr bool CUnqualedBasicPointer<T*> = true;

template<typename T> constexpr bool CUnqualedMemberPointer = false;
template<typename T, class U> constexpr bool CUnqualedMemberPointer<T U::*> = true;

template<typename T> constexpr bool CUnqualedMethodPointer = false;
template<typename T, typename U> constexpr bool CUnqualedMethodPointer<T U::*> = CFunction<T>;

template<typename T> concept CUnqualedFieldPointer = CUnqualedMemberPointer<T> && !CUnqualedMethodPointer<T>;

template<typename T> concept CVoid = CUnqualedVoid<TUnqual<T>>;
template<typename T> concept CChar = CUnqualedChar<TUnqual<T>>;
template<typename T> concept CBasicSignedIntegral = CUnqualedBasicSignedIntegral<TUnqual<T>>;
template<typename T> concept CBasicUnsignedIntegral = CUnqualedBasicUnsignedIntegral<TUnqual<T>>;
template<typename T> concept CBasicIntegral = CUnqualedBasicIntegral<TUnqual<T>>;
template<typename T> concept CBasicFloatingPoint = CUnqualedBasicFloatingPoint<TUnqual<T>>;
template<typename T> concept CBasicSigned = CUnqualedBasicSigned<TUnqual<T>>;
template<typename T> concept CBasicArithmetic = CUnqualedBasicArithmetic<TUnqual<T>>;
template<typename T> concept CBasicPointer = CUnqualedBasicPointer<TUnqual<T>>;
template<typename T> concept CMemberPointer = CUnqualedMemberPointer<TUnqual<T>>;
template<typename T> concept CMethodPointer = CUnqualedMethodPointer<TUnqual<T>>;
template<typename T> concept CFieldPointer = CUnqualedFieldPointer<TUnqual<T>>;


template<typename From, typename To> using TPropagateConst = TSelect<const To, To, CConst<From>>;
template<typename From, typename To> using TPropagateVolatile = TSelect<volatile To, To, CVolatile<From>>;
template<typename From, typename To> using TPropagateQualifiers = TPropagateConst<From, TPropagateVolatile<From, To>>;
template<typename From, typename To> using TPropagateLVRef = TSelect<To&, To, CLValueReference<From>>;
template<typename From, typename To> using TPropagateRVRef = TSelect<To&&, To, CRValueReference<From>>;
template<typename From, typename To> using TPropagateRef = TPropagateLVRef<From, TPropagateRVRef<From, To>>;
template<typename From, typename To> using TPropagateQualLVRef = TPropagateLVRef<From, TPropagateQualifiers<TRemoveReference<From>, To>>;
template<typename From, typename To> using TPropagateQualRef = TPropagateRef<From, TPropagateQualifiers<TRemoveReference<From>, To>>;

#if INTRA_CONSTEXPR_TEST
static_assert(CSame<TPropagateConst<const int, float>, const float>);
static_assert(CSame<TPropagateRef<int&, float>, float&>);
static_assert(CSame<TPropagateRef<int&&, float>, float&&>);
static_assert(CSame<TPropagateQualLVRef<const int&, float>, const float&>);
#endif

template<class T, size_t N = 0> constexpr size_t ArrayExtent = 0;
template<class T> constexpr size_t ArrayExtent<T[], 0> = 0;
template<class T, size_t N> constexpr size_t ArrayExtent<T[], N> = ArrayExtent<T, N-1>;
template<class T, size_t N> constexpr size_t ArrayExtent<T[N], 0> = N;
template<class T, size_t I, size_t N> constexpr size_t ArrayExtent<T[I], N> = ArrayExtent<T, N-1>;

template<typename T> using TDecay = TSelect<
	TRemoveExtent<TRemoveReference<T>>*,
	TSelect<
		TAddPointer<TRemoveReference<T>>,
		TUnqualRef<T>,
		CFunction<TRemoveReference<T>>>,
	CArrayType<TRemoveReference<T>>>;



namespace z_D {
template<typename T> struct TRemoveAllExtents_ {using _ = T;};
template<typename T> struct TRemoveAllExtents_<T[]>: TRemoveAllExtents_<T> {};
template<typename T, size_t N> struct TRemoveAllExtents_<T[N]>: TRemoveAllExtents_<T> {};
}
template<typename T> using TRemoveAllExtents = typename z_D::TRemoveAllExtents_<T>::_;

#define INTRA_DEFINE_SAFE_DECLTYPE(checker_name, expr) \
	struct z_D_ ## checker_name {\
		template<typename T> static auto test(int) -> decltype(expr); \
		template<typename T> static void test(...); \
	};\
	template<typename U> using checker_name = decltype(z_D_ ## checker_name::test<U>(0))

#define INTRA_DEFINE_SAFE_DECLTYPE_T_ARGS(checker_name, expr) \
	struct z_D_ ## checker_name { \
		template<typename T, typename... Args> static auto test(int) -> decltype(expr); \
		static void test(...); \
	};\
	template<typename U, typename... UArgs> using checker_name = decltype(z_D_ ## checker_name::test<U, UArgs...>(0))

#define INTRA_DEFINE_CONCEPT_EXPR(checker_name, expr, resultOnError) \
	struct z_D_ ## checker_name {\
		template<typename T> static constexpr auto test(int) -> decltype((expr), bool()) {return expr;} \
		static constexpr bool test(...) {return resultOnError;} \
	}; \
	template<typename U> concept checker_name = z_D_ ## checker_name::test<U>(0)

template<typename T> concept CScalar =
	CBasicArithmetic<T> ||
	CBasicPointer<T> ||
	CEnum<T> ||
	CMemberPointer<T> ||
	CSame<TUnqual<T>, decltype(nullptr)>;

template<typename T> concept CByteByByteLexicographicallyComparable =
	CHasUniqueObjectRepresentations<T> && requires{T::TagByteByByteLexicographicallyComparable::True;} ||
		(CBasicIntegral<T> || CBasicPointer<T> || CEnum<T>) && (sizeof(T) == 1 || Config::TargetIsBigEndian);

template<typename T> concept CTriviallyEqualComparable = CScalar<T> ||
	CByteByByteLexicographicallyComparable<T> ||
	requires{T::TagBitwiseEqualityComparable::True;};



template<typename T> concept CDestructible =
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	__is_destructible(T);
#else
	CReference<T> || !CUnknownBoundArrayType<T> && requires(TRemoveAllExtents<T> x) {x.~T();};
#endif

template<typename T, typename To> concept CStaticCastable = requires(T x, To) {static_cast<To>(x);};

template<typename T, typename... Args> concept CConstructible = __is_constructible(T, Args...);

#if defined(_MSC_VER) || defined(__clang__) || defined(__INTEL_COMPILER) || defined(__GNUC__) && __GNUC__ >= 11
template<typename T, typename... Args> concept CNothrowConstructible = __is_nothrow_constructible(T, Args...);
template<typename To, typename From> concept CNothrowAssignable = __is_nothrow_assignable(To, From);
#else
template<typename T, typename... Args> concept CNothrowConstructible = []{
	if constexpr(!CConstructible<T, Args...>) return false;
	else if constexpr(sizeof...(Args) == 1) return noexcept(static_cast<T>(Val<Args...>()));
	else if constexpr(sizeof...(Args) == 0 && CKnownBoundArrayType<T>) return noexcept(TRemoveExtent<T>());
	else return noexcept(T(Val<Args>()...));
}();
template<typename To, typename From> concept CNothrowAssignable = []{
	if constexpr(!__is_assignable(To, From)) return false;
	else return noexcept(Val<To>() = Val<From>());
}();
#endif

template<typename T> concept CCopyConstructible = CConstructible<T, TAddLValueReference<const T>>;
template<typename T> concept CMoveConstructible = CConstructible<T, T>;
template<typename T> concept CTriviallyCopyConstructible = CTriviallyConstructible<T, TAddLValueReference<const T>>;
template<typename T> concept CTriviallyDefaultConstructible = CTriviallyConstructible<T>;
template<typename T> concept CTriviallyMoveConstructible = CTriviallyConstructible<T, T>;
template<typename T> concept CNothrowCopyConstructible = CNothrowConstructible<T, TAddLValueReference<const T>>;
template<typename T> concept CNothrowMoveConstructible = CNothrowConstructible<T, T>;

template<typename F, typename... Args> concept CCallable = requires(F&& f, Args&&... args) {INTRA_FWD(f)(INTRA_FWD(args)...);};

template<typename T, typename... Args> using TResultOf = decltype(Val<T>()(Val<Args>()...));
INTRA_DEFINE_SAFE_DECLTYPE_T_ARGS(TResultOfOrVoid, Val<T>()(Val<Args>()...));


template<typename To, typename From> concept CAssignable = __is_assignable(To, From);
template<typename T> concept CCopyAssignable = CAssignable<TAddLValueReference<T>, TAddLValueReference<const T>>;
template<typename T> concept CMoveAssignable = CAssignable<TAddLValueReference<T>, T>;
template<typename T> concept CNothrowCopyAssignable = CNothrowAssignable<TAddLValueReference<T>, TAddLValueReference<const T>>;
template<typename T> concept CNothrowMoveAssignable = CNothrowAssignable<TAddLValueReference<T>, T>;

/*!
  Trivially relocatable is a less constrained concept than trivially copyable.
  All trivially copyable types are also trivially relocatable.
  However there may be types having move constructor and destructor that are not trivial separately but combination of them may be trivial.
  It is true for most containers. You can make a bitwise copy of a container object without calling the move constructor and the destructor of source.
  Specialize IsTriviallyRelocatable for such types after their definition.
*/
template<typename T> constexpr bool IsTriviallyRelocatable = CTriviallyCopyable<T>;
template<typename T> concept CTriviallyRelocatable = IsTriviallyRelocatable<T>;

namespace z_D {
template<bool Cond, class T=void> struct TRequires {};
template<class T> struct TRequires<true, T> {using _ = T;};
template<bool Cond, class T> struct TRequiresAssert {static_assert(Cond); using _ = T;};
}
template<bool Cond, typename T = void> using Requires = typename z_D::TRequires<Cond, T>::_;
template<typename TypeToCheckValidity, typename T = void> using RequiresT = T;
template<bool Cond, typename T = void> using RequiresAssert = typename z_D::TRequiresAssert<Cond, T>::_;

template<typename From, typename To> concept CConvertibleTo =
#if defined(_MSC_VER) || defined(__clang__) || defined(__INTEL_COMPILER)
	__is_convertible_to(From, To);
#else
	CVoid<From> && CVoid<To> || requires(void(*f)(To), From from) {f(from);};
#endif

template<typename T> concept CScopedEnum = CEnum<T> && !CConvertibleTo<T, int>;

namespace z_D {
template<typename... Ts> struct TCommon_ {};
template<typename T> struct TCommon_<T> {using _ = TDecay<T>;};
template<typename T> struct TCommon_<T, void> {using _ = void;};
template<typename T> struct TCommon_<void, T> {using _ = void;};
template<> struct TCommon_<void, void> {using _ = void;};
template<typename T, typename U> struct TCommon_<T, U> {using _ = TDecay<decltype(bool()? Val<T>(): Val<U>())>;};
template<typename T, typename U, typename... V> struct TCommon_<T, U, V...> {
	using _ = typename TCommon_<typename TCommon_<T, U>::_, V...>::_;
};
template<class, typename... Ts> struct TCommonRef_ {using _ = void;};
template<typename... Ts> struct TCommonRef_<TValue<false>, Ts...> {
	using Common = typename TCommon_<Ts...>::_;
	using _ = TSelect<
		TSelect<const TAddLValueReference<Common>,
			TAddLValueReference<Common>,
			(CConst<TDecay<Ts>> || ...)>,
		Common,
		CSame<TDecay<Ts>...> && (CReference<Ts> && ...)>;
};
}
/// Common type without const\volatile and references.
template<typename... Ts> using TCommon = typename z_D::TCommon_<Ts...>::_;

/// Common type preserving references.
template<typename... Ts> using TCommonRef = typename z_D::TCommonRef_<TValue<(CVoid<Ts> || ...)>, Ts...>::_;

namespace z_D {
template<typename T> struct TMemberFieldType_;
template<typename T, typename F> struct TMemberFieldType_<F T::*> {using _ = F;};
}
template<typename T> using TMemberFieldType = typename z_D::TMemberFieldType_<T>::_;


#if INTRA_CONSTEXPR_TEST
static_assert(CArrayType<TRemoveReference<const char(&)[9]>>);
// TODO: add more tests
#endif

namespace z_D {
template<typename F> struct TFinally
{
	INTRA_CONSTEXPR_DESTRUCTOR ~TFinally() {OnDestruct();}
	F OnDestruct;
};
struct TFinallyMaker
{
	template<CCallable F> constexpr	TFinally<F> operator=(F&& f) const noexcept {return {INTRA_FWD(f)};}
};
}
/** Execute a statement block after leaving the current scope for any reason: normal or exception.
  usage: INTRA_FINALLY{<code block>};
*/
#define INTRA_FINALLY auto INTRA_CONCATENATE_TOKENS(z_D_finally_, __COUNTER__) = ::Intra::z_D::TFinallyMaker() = [&]()->void

/// Combine multiple functors into a single overloaded functor. Useful for creating visitors.
template<class... Callbacks> struct CombineOverloads: Callbacks... {using Callbacks::operator()...;};
template<class... Callbacks> CombineOverloads(Callbacks...) -> CombineOverloads<Callbacks...>;

#if INTRA_CONSTEXPR_TEST
static_assert(CombineOverloads{
	[](int x) {return x+1;},
	[](float x) {return x*2;}
}(3.25f) == 6.5f);
#endif

} INTRA_END
//// </Fundamental C++ type metafunctions and concepts>


//// <Parameter pack and type list manipulation>
namespace Intra { INTRA_BEGIN
template<typename T, T... Ints> struct TIntSeq {using _ = TIntSeq;};
template<size_t... Ints> using TIndexSeq = TIntSeq<size_t, Ints...>;

#if defined(_MSC_VER) || defined(__clang__)
template<typename T, T Size> using TMakeIntSeq = __make_integer_seq<TIntSeq, T, Size>;
#elif defined(__GNUC__)
template<typename T, T Size> using TMakeIntSeq = TIntSeq<T, __integer_pack(Size)...>;
#endif
template<size_t Size> using TMakeIndexSeq = TMakeIntSeq<size_t, Size>;
template<typename... Ts> using TSeqFor = TMakeIndexSeq<sizeof...(Ts)>;

template<typename T, T Add, class Seq> struct TIntSeqAdd;
template<typename T, T Add, T... Ints>
struct TIntSeqAdd<T, Add, TIntSeq<T, Ints...>>: TIntSeq<T, (Add+Ints)...> {};
template<size_t End, size_t Start = 0>
using TIndexRange = TIntSeqAdd<size_t, Start, TMakeIntSeq<size_t, End-Start>>;


template<typename... Ts> struct TList {};
namespace z_D {
template<class TL> struct TListTail_ {};
template<template<typename...> class TL, typename TFirst, typename... TRest>
struct TListTail_<TL<TFirst, TRest...>> {using _ = TL<TRest...>;};
}
template<class TL> using TListTail = typename z_D::TListTail_<TL>::_;

template<class TL> constexpr size_t TListLength = 0;
template<template<typename...> class TL, typename... Ts> constexpr size_t TListLength<TL<Ts...>> = sizeof...(Ts);

namespace z_D {
template<size_t N, class TL> struct TListAt_;
template<size_t N, template<typename...> class TL, typename... Ts>
struct TListAt_<N, TL<Ts...>> {using _ = TPackAt<N, Ts...>;};
}
template<unsigned N, typename TL> using TListAt = typename z_D::TListAt_<N, TL>::_;

namespace z_D {
template<class TL, template<typename...> class DstTL> struct TListConvert;
template<template<typename...> class TL, template<typename...> class DstTL, typename... Ts>
struct TListConvert<TL<Ts...>, DstTL> {using _ = DstTL<Ts...>;};
}
template<class TL, template<typename...> class DstTL = TList>
using TListConvert = typename z_D::TListConvert<TL, DstTL>::_;

namespace z_D {
template<class TL1, typename T2> struct TListConcat_;
template<template<typename...> class TL, typename... Ts1, typename... Ts2>
struct TListConcat_<TL<Ts1...>, TL<Ts2...>> {using _ = TL<Ts1..., Ts2...>;};
template<template<typename...> class TL, typename T, typename... Ts>
struct TListConcat_<T, TL<Ts...>> {using _ = TL<Ts..., T>;};
}
template<class TL, typename... Ts>
using TListConcat = typename z_D::TListConcat_<TL, Ts...>::_;


namespace z_D {
template<class TL, typename T> struct TListRemove_;
template<template<typename...> class TL, typename T, typename... Args> struct TListRemove_<TL<Args...>, T>
{
	using Removed = typename TListRemove_<TListTail<TL<Args...>>, T>::_;
	using First = TPackFirst<Args...>;
	using _ = TSelect<Removed, TListConcat<TL<First>, Removed>, CSame<First, T>>;
};

template<template<typename...> class TL, typename T, typename Head>
struct TListRemove_<TL<Head>, T> {using _ = TL<Head>;};
template<template<typename...> class TL, typename T>
struct TListRemove_<TL<T>, T> {using _ = TL<>;};
template<template<typename...> class TL, typename T>
struct TListRemove_<TL<>, T> {using _ = TL<>;};
}
template<class TL, typename T> using TListRemove = typename z_D::TListRemove_<TL, T>::_;


namespace z_D {
template<class TL> struct TListRemoveDuplicates_ {};
template<template<typename...> class TL> struct TListRemoveDuplicates_<TL<>> {using _ = TL<>;};
template<template<typename...> class TL, typename... Ts> struct TListRemoveDuplicates_<TL<Ts...>>
{
	using HeadRemovedFromTail = TListRemove<TPackFirst<Ts...>, TListTail<TL<Ts...>>>; //TODO: is the order correct?
	using TailWithoutDuplicates = typename TListRemoveDuplicates_<HeadRemovedFromTail>::_;
	using _ = TListConcat<TailWithoutDuplicates, TL<TPackFirst<Ts...>>>;
};
}
template<class TL> using TListRemoveDuplicates_ = typename z_D::TListRemoveDuplicates_<TL>::_;

template<size_t IndexFrom = 0> constexpr auto IndexOfFirstTrueArg = [](auto... args)
{
	if constexpr(sizeof...(args) != 0)
	{
		const bool isTrue[] = {!!args...};
		for(size_t i = 0; i < sizeof...(args); i++)
			if(isTrue[i]) return i;
	}
	return sizeof...(args);
};

template<class TL, typename T, size_t IndexFrom = 0> constexpr size_t TListFind = 0;
template<template<typename...> class TL, typename T, size_t IndexFrom, typename... Ts>
constexpr size_t TListFind<TL<Ts...>, T, IndexFrom> = IndexOfFirstTrueArg<IndexFrom>(CSame<T, Ts>...);

template<class TL, typename T, size_t IndexFrom = 0>
constexpr size_t TListContains = TListFind<TL, T, IndexFrom> != TListLength<TL>;

template<class TL, typename T, size_t IndexFrom = 0>
constexpr size_t TListFindUnique = [] {
	constexpr auto firstEntryIndex = TListFind<TL, T, IndexFrom>;
	if constexpr(firstEntryIndex == TListLength<TL>) return firstEntryIndex;
	else if constexpr(TListFind<TL, T, firstEntryIndex + 1> != TListLength<TL>) return TListLength<TL>;
	else return firstEntryIndex;
}();

#if INTRA_CONSTEXPR_TEST
static_assert(TListFind<TList<int, float, float>, float> == 1);
static_assert(TListFind<TList<int, float, float>, double> == 3);
static_assert(TListFindUnique<TList<int, float, float>, float> == 3);
#endif

namespace z_D {
template<class TL, size_t Begin, size_t Last>
struct TListSlice_ {using _ = void;};

template<template<typename...> class TL, size_t BeginAndEnd, typename... Ts>
struct TListSlice_<TL<Ts...>, BeginAndEnd, BeginAndEnd> {using _ = TL<>;};

template<template<typename...> class TL, size_t Begin, size_t End, typename... Ts>
struct TListSlice_<TL<Ts...>, Begin, End>
{
	static_assert(End >= Begin, "Invalid range!");
	using _ = TListConcat<
		typename TListSlice_<TL<Ts...>, Begin, End - 1>::_,
		TL<TPackAt<End-1, Ts...>>
	>;
};
}
template<class TL, size_t Begin, size_t End = TListLength<TL>>
using TListSlice = typename z_D::TListSlice_<TL,
	Begin <= TListLength<TL>? Begin: TListLength<TL>,
	End <= TListLength<TL>? End: TListLength<TL>>::_;

template<template<typename> class Predicate, class TL>
constexpr bool CListAll = false;
template<template<typename> class Predicate, template<typename...> class TL, typename... Ts>
constexpr bool CListAll<Predicate, TL<Ts...>> = (Predicate<Ts>::_ && ...);

template<template<typename> class Predicate, class TL> constexpr bool CListAny = false;
template<template<typename> class Predicate, template<typename...> class TL, typename... Ts>
constexpr bool CListAny<Predicate, TL<Ts...>> = (Predicate<Ts>::_ || ...);


template<template<typename, typename> class Predicate, typename Arg0, class TL>
constexpr bool CListAll1 = false;
template<template<typename, typename> class Predicate, template<typename...> class TL, typename Arg0, typename... Ts>
constexpr bool CListAll1<Predicate, Arg0, TL<Ts...>> = (Predicate<Arg0, Ts>::_ && ...);

template<template<typename, typename> class Predicate, typename Arg0, class TL>
constexpr bool CListAny1 = false;
template<template<typename, typename> class Predicate, template<typename...> class TL, typename Arg0, typename... Ts>
constexpr bool CListAny1<Predicate, Arg0, TL<Ts...>> = (Predicate<Arg0, Ts>::_ || ...);


template<template<typename, typename> class Predicate, class TL1, class TL2>
constexpr bool CListAllPairs = false;
template<template<typename, typename> class Predicate, template<typename...> class TL, typename... Ts1, typename... Ts2>
constexpr bool CListAllPairs<Predicate, TL<Ts1...>, TL<Ts2...>> = (Predicate<Ts1, Ts2>::_ && ...);

template<template<typename, typename> class Predicate, class TL1, class TL2>
constexpr bool CListAnyPair = false;
template<template<typename, typename> class Predicate, template<typename...> class TL, typename... Ts1, typename... Ts2>
constexpr bool CListAnyPair<Predicate, TL<Ts1...>, TL<Ts2...>> = (Predicate<Ts1, Ts2>::_ || ...);

namespace z_D {
template<class TL, template<typename...> class Transform, typename... BindArgs>
struct TListTransform_ {using _ = void;};
template<template<typename...> class Transform, template<typename...> class TL, typename... BindArgs, typename... Ts>
struct TListTransform_<TL<Ts...>, Transform, BindArgs...> {using _ = TL<Transform<BindArgs..., Ts>...>;};

template<class TL, template<typename...> class Transform>
struct TListUnpackTo_ {using _ = void;};
template<template<typename...> class C, template<typename...> class TL, typename... Ts>
struct TListUnpackTo_<TL<Ts...>, C> {using _ = C<Ts...>;};
}
template<class TL, template<typename...> class Transform, typename... BindArgs>
using TListTransform = typename z_D::TListTransform_<TL, Transform, BindArgs...>::_;
template<class TL, template<typename...> class C>
using TListUnpackTo = typename z_D::TListUnpackTo_<TL, C>::_;

template<class TL> using TListCommon = TListUnpackTo<TL, TCommon>;
template<class TL> using TListCommonRef = TListUnpackTo<TL, TCommonRef>;
namespace z_D {
template<typename To, typename From> using CAssignableT = TValue<CAssignable<To, From>>;
template<typename From, typename To> using CConvertibleToT = TValue<CConvertibleTo<From, To>>;
}
template<class From, class To> concept CTListConvertible = CListAllPairs<z_D::CConvertibleToT, From, To>;
template<class To, class From> concept CTListAssignable = CListAllPairs<z_D::CAssignableT, To, From>;

template<typename T, auto... Values> constexpr auto VMapByType = TListAt<TListFind<TList<decltype(Values)...>, T>, TList<TValue<Values>...>>::_;

#if INTRA_CONSTEXPR_TEST
static_assert(TListFind<TList<int, float, double>, int> == 0);
static_assert(TListFind<TList<int, float, double>, float> == 1);
static_assert(TListFind<TList<int, float, double>, void> == 3);
static_assert(CSame<TPackFirst<int, float, double>, int>);
static_assert(CSame<TListTail<TList<int, float, double>>, TList<float, double>>);
static_assert(CSame<TListRemove<TList<int, float, double>, float>, TList<int, double>>);
static_assert(CSame<TList<int>, TListSlice<TList<int, float>, 0, 1>>);
static_assert(CSame<TList<>, TListSlice<TList<int, float, double, void>, 4, 4>>);
static_assert(CSame<TList<float, double>, TListSlice<TList<int, float, double, void>, 1, 3>>);
static_assert(CSame<TList<float, double, void>, TListTransform<TList<float, const double, const void>, TRemoveConst>>);

static_assert(VMapByType<int32,
	uint32(4294967295),
	int16(32767),
	int32(2147483647),
	float(3.14f)
> == 2147483647);
#endif

template<template<typename... Ts> class TList> constexpr auto ForEachType = []<typename F>(F&& f) requires (CCallable<F, TType<Ts>> && ...)
{
	(f(Type<Ts>), ...);
	return TList<typename decltype(f(Type<Ts>))::_...>();
};
template<template<typename... Ts> class TList, class F> using TForEachType = decltype(ForEachType<TList>(Val<F>()));

template<template<typename... Ts> class TList> constexpr auto TListMapReduce =
	[]<typename M, typename R>(M&& map, R&& reduce) requires (CCallable<M, TType<Ts>> && ...)
{
	return reduce(map(Type<Ts>)...);
};

} INTRA_END
//// </Parameter pack and type list manipulation>

//// <Functors>
namespace Intra { INTRA_BEGIN
constexpr auto FNot = []<typename P>(P&& f) {
	return [f = INTRA_FWD(f)]<typename... Args>(Args&&... args) -> decltype(!f(INTRA_FWD(args)...)) {
		return !f(INTRA_FWD(args)...);
	};
};

constexpr auto FRepeat = [](auto&& value) {
	return [value = INTRA_FWD(value)](auto&&...) -> const auto& noexcept {return value;};
};

template<typename T> struct FRef
{
	T& FunctorReference;
	constexpr FRef(T& functorReference) noexcept: FunctorReference(functorReference) {}

	template<typename... Args> constexpr auto operator()(Args&&... args) const
		-> decltype(FunctorReference(INTRA_FWD(args)...)) {return FunctorReference(INTRA_FWD(args)...);}
};

template<auto Value> constexpr auto StaticConst = [](auto&&...) {return Value;};
constexpr auto Always = StaticConst<true>;
constexpr auto Never = StaticConst<false>;

#if INTRA_CONSTEXPR_TEST
static_assert(FRepeat(5)() == 5);
static_assert(Always(7, 5, "qwerty", 1));
static_assert(!Never(43, nullptr, 65));
#endif

/// Comparison operations
constexpr auto Less = [](auto&& a, auto&& b) -> decltype(INTRA_FWD(a) < INTRA_FWD(b)) {return INTRA_FWD(a) < INTRA_FWD(b);};
constexpr auto LEqual = [](auto&& a, auto&& b) -> decltype(INTRA_FWD(a) <= INTRA_FWD(b)) {return INTRA_FWD(a) <= INTRA_FWD(b);};
constexpr auto Greater = [](auto&& a, auto&& b) -> decltype(INTRA_FWD(a) > INTRA_FWD(b)) {return INTRA_FWD(a) > INTRA_FWD(b);};
constexpr auto GEqual = [](auto&& a, auto&& b) -> decltype(INTRA_FWD(a) >= INTRA_FWD(b)) {return INTRA_FWD(a) >= INTRA_FWD(b);};
constexpr auto Equal = [](auto&& a, auto&&... args) -> decltype((INTRA_FWD(a) == INTRA_FWD(args) && ...)) {return (INTRA_FWD(a) == INTRA_FWD(args) && ...);};
constexpr auto NotEqual = [](auto&& a, auto&&... args) -> decltype((INTRA_FWD(a) != INTRA_FWD(args) || ...)) {return (INTRA_FWD(a) != INTRA_FWD(args) || ...);};

constexpr auto EqualsTo = [](auto&& x) {return Bind(Equal, INTRA_FWD(x));};

constexpr auto Add = [](auto&&... args) -> decltype((INTRA_FWD(args) + ...)) {return (INTRA_FWD(args) + ...);};
constexpr auto Mul = [](auto&&... args) -> decltype((INTRA_FWD(args) * ...)) {return (INTRA_FWD(args) * ...);};

constexpr auto Sub = [](auto&& a, auto&& b) -> decltype(INTRA_FWD(a) - INTRA_FWD(b)) {return INTRA_FWD(a) - INTRA_FWD(b);};
constexpr auto RSub = [](auto&& a, auto&& b) -> decltype(INTRA_FWD(b) - INTRA_FWD(a)) {return INTRA_FWD(b) - INTRA_FWD(a);};
constexpr auto Div = [](auto&& a, auto&& b) -> decltype(INTRA_FWD(a) / INTRA_FWD(b)) {return INTRA_FWD(a) / INTRA_FWD(b);};
constexpr auto RDiv = [](auto&& a, auto&& b) -> decltype(INTRA_FWD(b) / INTRA_FWD(a)) {return INTRA_FWD(b) / INTRA_FWD(a);};
constexpr auto RMod = [](auto&& a, auto&& b) -> decltype(INTRA_FWD(b) % INTRA_FWD(a)) {return INTRA_FWD(b) % INTRA_FWD(a);};

constexpr auto Cmp = [](const auto& a, const auto& b) {return (a > b) - (a < b);};
constexpr auto ISign = [](const auto& a) {return (a > 0) - (a < 0);};

constexpr auto And = [](auto&&... args) -> decltype((args && ...)) {return (args && ...);};
constexpr auto Or = [](auto&&... args) -> decltype((args || ...)) {return (args || ...);};

constexpr auto BitAnd = [](auto&&... args) -> decltype((INTRA_FWD(args) & ...)) {return (INTRA_FWD(args) & ...);};
constexpr auto BitOr = [](auto&&... args) -> decltype((INTRA_FWD(args) | ...)) {return (INTRA_FWD(args) | ...);};
constexpr auto BitXor = [](auto&&... args) -> decltype((INTRA_FWD(args) ^ ...)) {return (INTRA_FWD(args) ^ ...);};

constexpr auto LShift = [](auto&& a, auto&& b) -> decltype(INTRA_FWD(a) << b) {return INTRA_FWD(a) << b;};
constexpr auto RShift = [](auto&& a, auto&& b) -> decltype(INTRA_FWD(a) >> b) {return INTRA_FWD(a) >> b;};

constexpr auto Deref = [](auto&& x) noexcept -> decltype(*x) {return *x;};
constexpr auto Negate = [](auto&& x) noexcept -> decltype(-x) {return -x;};
constexpr auto BitNot = [](auto&& x) noexcept -> decltype(~x) {return ~x;};

constexpr auto Swap = []<typename T>(T&& a, T&& b) {
	if(AddressOf(a) == AddressOf(b)) return;
	auto temp = INTRA_MOVE(a);
	a = INTRA_MOVE(b);
	b = INTRA_MOVE(temp);
};

constexpr auto Exchange = [](auto& dst, auto&& newValue) {
	auto oldValue = INTRA_MOVE(dst);
	dst = INTRA_FWD(newValue);
	return oldValue;
};

constexpr auto Move = [](auto&& x) noexcept -> decltype(auto) {return INTRA_MOVE(x);};

constexpr auto MoveNoexcept = []<typename T>(T&& t) noexcept -> decltype(auto) {
	if constexpr(CNothrowMoveConstructible<T> || !CCopyConstructible<T>)
		return static_cast<TRemoveReference<T>&&>(t);
	else return static_cast<TRemoveReference<T>&>(t);
};

constexpr auto Dup = []<typename T>(T&& x) noexcept {
	if constexpr(CLValueReference<T>) return x;
	else return INTRA_MOVE(x);
};

/// Useful with ranges, for example:
/// indexRange|Map(Bind(IndexOp, valueRange))
constexpr auto IndexOp = [](auto&& from, auto&& index) -> decltype(from[index]) {return from[index];};

template<typename T> constexpr auto ImplicitCastTo = [](auto&& value) -> T {return INTRA_FWD(value);};
template<typename T> constexpr auto CastTo = [](auto&& value) -> decltype(T(INTRA_FWD(value))) {return T(INTRA_FWD(value));};
constexpr auto Identity = [](auto&& x) -> decltype(auto) {return INTRA_FWD(x);};

template<auto Op, typename... Ts> concept CHasOp = CCallable<decltype(Op), Ts...>;
template<typename T1, typename T2 = T1> concept CHasOpAdd = CCallable<decltype(Add), T1, T2>;
template<typename T1, typename T2 = T1> concept CHasOpSub = CCallable<decltype(Sub), T1, T2>;
template<typename T1, typename T2 = T1> concept CHasOpMul = CCallable<decltype(Mul), T1, T2>;
template<typename T1, typename T2 = T1> concept CHasOpDiv = CCallable<decltype(Div), T1, T2>;
template<typename T1, typename T2 = T1> concept CHasOpMod = requires(T1 x, T2 y) {x % y;}; //TODO: CCallable<decltype(Mod), T1, T2>
template<typename T1, typename T2 = int> concept CHasOpLShift = CCallable<decltype(LShift), T1, T2>;
template<typename T1, typename T2 = int> concept CHasOpRShift = CCallable<decltype(RShift), T1, T2>;
template<typename T1, typename T2 = T1> concept CHasOpBitAnd = CCallable<decltype(BitAnd), T1, T2>;
template<typename T1, typename T2 = T1> concept CHasOpBitOr = CCallable<decltype(BitOr), T1, T2>;
template<typename T1, typename T2 = T1> concept CHasOpBitXor = CCallable<decltype(BitXor), T1, T2>;
template<typename T> concept CHasOpBitNot = CCallable<decltype(BitNot), T>;
template<class L, typename I = index_t> concept CHasIndex = requires(L&& list, I index) {list[index];};

template<typename T> concept CAdditive = CHasOpAdd<T> && CHasOpSubtract<T>;
template<typename T> concept CMultiplicative = CHasOpMul<T> && CHasOpDiv<T>;
template<typename T> concept CArithmetic = CAdditive<T> && CMultiplicative<T>;
template<typename T> concept CBitset = CHasOpBitAnd<T> && CHasOpBitOr<T> && CHasOpBitXor<T> && CHasOpBitNot<T>;
template<typename T> concept CShiftable = CHasOpLShift<T> && CHasOpRShift<T>;
template<typename T> concept CNumber = CArithmetic<T> && CConstructible<double, T> && CConstructible<T, double>;

} INTRA_END
//// </Functors>

//// <BitCastTo and numeric traits>
namespace Intra { INTRA_BEGIN
namespace z_D {
template<class To, class From> constexpr auto ConstexprBitCastBetweenFloatAndInt(const From& from) noexcept;
}
template<class To, class From> constexpr auto BitCastTo = [](const From& from) noexcept
{
	static_assert(sizeof(From) == sizeof(To));
	static_assert(CTriviallyCopyable<To> && CTriviallyCopyable<From>);
#ifdef INTRA_CONSTEXPR_BITCAST_SUPPORT
	return __builtin_bit_cast(To, from);
#else
	if constexpr(CBasicIntegral<To> && CBasicIntegral<From> || CBasicFloatingPoint<To> && CBasicFloatingPoint<From>) return To(from);
	else
	{
		if constexpr(CBasicIntegral<From> && CBasicFloatingPoint<To> || CBasicFloatingPoint<From> && CBasicIntegral<To>)
			if(IsConstantEvaluated()) return z_D::ConstexprBitCastBetweenFloatAndInt(from);
		To to;
		__builtin_memcpy(&to, &from, sizeof(to));
		return to;
	}
#endif
};

template<size_t MinSize> using TUnsignedIntOfSizeAtLeast = TPackAt<MinSize-1,
	uint8, uint16, uint32, uint32, uint64, uint64, uint64, uint64>;
template<size_t MinSize> using TSignedIntOfSizeAtLeast = TPackAt<MinSize-1,
	int8, int16, int32, int32, int64, int64, int64, int64>;

template<size_t MinSize, bool Signed> using TIntOfSizeAtLeast = TSelect<
	TSignedIntOfSizeAtLeast<MinSize>,
	TUnsignedIntOfSizeAtLeast<MinSize>,
	Signed>;

template<size_t MaxSize> using TUnsignedIntOfSizeAtMost = TPackAt<Min(8, MaxSize) - 1,
	uint8, uint16, uint16, uint32, uint32, uint32, uint32, uint64>;
template<size_t MaxSize> using TSignedIntOfSizeAtMost = TPackAt<Min(8, MaxSize) - 1,
	int8, int16, int16, int32, int32, int32, int32, int64>;

template<size_t MinSize, bool Signed> using TIntOfSizeAtMost = TSelect<
	TSignedIntOfSizeAtMost<MaxSize>,
	TUnsignedIntOfSizeAtMost<MaxSize>,
	Signed>;

template<size_t MinSize> using TFloatOfSizeAtLeast = TPackAt<(MinSize - 1) / 4, float, double>;

template<typename T> using TToUnsigned = TUnsignedIntOfSizeAtLeast<(CBasicIntegral<T> || CChar<T>)? sizeof(T): 0xBad>;
template<typename T> using TToSigned = TSelect<TSignedIntOfSizeAtLeast<(CBasicIntegral<T> || CChar<T>)? sizeof(T): 0>, T, CBasicIntegral<T>>;

namespace z_D {
template<typename T, bool = CBasicArithmetic<T>> struct TToIntegral_ {};
template<typename T> struct TToIntegral_<T, true> {using _ = TIntOfSizeAtLeast<sizeof(T), CBasicSigned<T>>;};
// Will be specialized later for other types
}
template<CBasicArithmetic T> using TToIntegral = typename z_D::TToIntegral_<TRemoveConstRef<T>>::_;

template<typename T> constexpr int SizeofInBits = int(sizeof(T)*8);

} INTRA_END
//// </BitCastTo and numeric traits>

//// <assert>
#if INTRA_CODE_SECTION
namespace Intra { INTRA_BEGIN
struct SourceInfo
{
	const char* Function = nullptr;
	const char* File = nullptr;
	unsigned Line = 0;

	constexpr explicit operator bool() const
	{
		return Function != nullptr || File != nullptr || Line != 0;
	}

	constexpr SourceInfo(
#ifndef __INTEL_COMPILER
		const char* function = __builtin_FUNCTION(),
		const char* file = __builtin_FILE(),
		unsigned line = __builtin_LINE()):
#else
		const char* function = nullptr,
		const char* file = nullptr,
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

/// @returns a reference to a function pointer to be called on any fatal error including assertion failures.
/// It becomes initialized automatically if System module is used.
/// Otherwise it should be assigned manually unless the access violations on errors are acceptable.
inline FatalErrorCallbackType& FatalErrorCallback()
{
	static FatalErrorCallbackType callback = nullptr;
	return callback;
}

namespace z_D {
#ifdef _WIN32
extern "C" INTRA_DLL_IMPORT int INTRA_WINAPI IsDebuggerPresent();
#endif
}

constexpr bool IsDebuggerAttached()
{
	if(IsConstantEvaluated()) return false;
#ifdef _WIN32
	return z_D::IsDebuggerPresent() != 0;
#else
	return false;
#endif
}
} INTRA_END

#define INTRA_SOURCE_INFO ::Intra::SourceInfo(INTRA_CURRENT_FUNCTION, __FILE__, unsigned(__LINE__))

#define INTRA_DEBUGGER_BREAKPOINT (( \
		!::Intra::Config::DisableTroubleshooting && \
		!::Intra::IsConstantEvaluated() && \
		::Intra::IsDebuggerAttached() \
	)? (__builtin_trap(), (void)0): (void)0)


#define INTRA_FATAL_ERROR(msg) (( \
		!::Intra::Config::DisableTroubleshooting || \
		::Intra::IsConstantEvaluated())? (INTRA_DEBUGGER_BREAKPOINT, \
	::Intra::FatalErrorCallback()(msg, INTRA_SOURCE_INFO)): ::Intra::UnreachableCode(::Intra::Unsafe))

#define INTRA_ASSERT(expr) INTRA_LIKELY(expr)? (void)0: (void)(\
    (INTRA_FATAL_ERROR("Assertion " # expr " failed!"), true))

#define INTRA_DEBUG_FATAL_ERROR(msg) (( \
	::Intra::Config::DebugCheckLevel > 0 || \
	::Intra::IsConstantEvaluated())? \
		INTRA_FATAL_ERROR(msg): ::Intra::UnreachableCode(::Intra::Unsafe))

#define INTRA_DEBUG_ASSERT(expr) (( \
	::Intra::Config::DebugCheckLevel > 0 || \
	::Intra::IsConstantEvaluated())? \
		INTRA_ASSERT(expr): ::Intra::UnreachableCode(::Intra::Unsafe))

#define INTRA_PRECONDITION INTRA_DEBUG_ASSERT
#define INTRA_POSTCONDITION INTRA_DEBUG_ASSERT
#endif
//// </assert>
