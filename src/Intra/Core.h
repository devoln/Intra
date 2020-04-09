#pragma once

/**
  This header contains:
  1. compiler independent wrappers around compiler extensions and intrinsics;
  2. architecture detection;
  3. compiler independent warning management macros;
  4. implementation of a placement new (with slightly different syntax) without including any standard headers.
*/

#if __cplusplus < 201703 && (!defined(_MSVC_LANG) || _MSVC_LANG < 201703 || _MSC_VER < 1916)
static_assert(false, "Intra library requires C++17 or above: GCC 7+/Clang 5+/ICC 19+/MSVC 2017.6+");
#endif

#if defined(__GNUC__) || defined(__clang__)
#define INTRA_FORCEINLINE inline __attribute__((always_inline))
#define INTRA_OPTIMIZE_FUNCTION_END
#define INTRA_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define INTRA_ASSUME_ALIGNED(ptr, alignmentBytes) static_cast<decltype(+ptr)>(__builtin_assume_aligned(ptr, alignmentBytes))
#endif

#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#define INTRA_MATH_CONSTEXPR constexpr
#define INTRA_MATH_CONSTEXPR_SUPPORT
#define INTRA_SOURCE_LOCATION_SUPPORT
#if __GNUC__ >= 9
#define INTRA_IS_CONSTANT_EVALUATED_SUPPORT
#define INTRA_CONSTEXPR_VIRT constexpr
#endif
#define INTRA_NO_VECTORIZE_FUNC __attribute__((optimize("no-tree-vectorize")))
#else
#define INTRA_MATH_CONSTEXPR
#ifdef __has_builtin
#if __has_builtin(__builtin_FUNCTION) && __has_builtin(__builtin_FILE) && __has_builtin(__builtin_LINE) //Clang 9+
#define INTRA_SOURCE_LOCATION_SUPPORT
#endif
#if __has_builtin(__builtin_is_constant_evaluated) //Clang 9+, MSVC 2019.5
#define INTRA_IS_CONSTANT_EVALUATED_SUPPORT
#endif
#endif
#define INTRA_NO_VECTORIZE_FUNC
#endif

#ifdef __clang__
#define INTRA_OPTIMIZE_FUNCTION(...) __VA_ARGS__
#define INTRA_OPT_UNROLL_LOOP _Pragma(clang loop unroll(enable))
#elif defined(__GNUC__)
#define INTRA_OPTIMIZE_FUNCTION(...) __VA_ARGS__ __attribute__((flatten, optimize("O3")))
#define INTRA_OPT_UNROLL_LOOP
#elif defined(_MSC_VER)
#define INTRA_OPTIMIZE_FUNCTION(...) __pragma(optimize("gt", on)) __VA_ARGS__ //gty
#define INTRA_OPTIMIZE_FUNCTION_END __pragma(optimize("", on))
#define INTRA_OPT_UNROLL_LOOP
#define INTRA_LIKELY(expr) !!(expr)
#define INTRA_ASSUME_ALIGNED(ptr, alignmentBytes) ptr
#define INTRA_FORCEINLINE __forceinline
#else
static_assert(false, "Unrecognized compiler!");
#endif

#ifdef __cpp_modules
#define INTRA_EXPORT export
#define INTRA_EXPORT_MODULE(name) export module name
#else
#define INTRA_EXPORT
#define INTRA_EXPORT_MODULE(name)
#endif

#ifndef __cpp_concepts //partially emulate concepts
#define concept constexpr bool
#ifdef __clang__
#define requires(cond) __attribute__((enable_if((cond), #cond)))
#endif
#elif defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 9
#define concept concept bool //emulate modern concept syntax with older GCC concepts 
#endif

#ifdef _MSC_VER
#define __builtin_trap() __debugbreak()
#define INTRA_COMPILER_ASSUME(hint) __assume(hint)
#define INTRA_CURRENT_FUNCTION __FUNCSIG__
#define INTRA_EMPTY_BASES __declspec(empty_bases)
#define INTRA_NOVTABLE  __declspec(novtable)
#define INTRA_CRTDECL __cdecl //Used to import functions from CRT without including its headers
#define INTRA_CRTRESTRICT __declspec(restrict)
#define INTRA_NO_VECTORIZE_LOOP __pragma(loop(no_vector))
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

#if defined(_MSC_VER) && defined(INTRA_SIMD_LEVEL) && INTRA_SIMD_LEVEL != 0
#define INTRA_VECTORCALL __vectorcall
#else
#define INTRA_VECTORCALL
#endif

#ifndef INTRA_CONSTEXPR_TEST
#define INTRA_CONSTEXPR_TEST __cpp_constexpr //To disable constexpr tests, compile with -D INTRA_CONSTEXPR_TEST=0
#endif

#if __cpp_constexpr >= 201907
#define INTRA_CONSTEXPR_CPP20 constexpr
#define INTRA_CONSTEXPR_VIRT constexpr
#else
#define INTRA_CONSTEXPR_CPP20
#define INTRA_CONSTEXPR_VIRT
#endif

#if INTRA_CONSTEXPR_TEST >= 201907
#define INTRA_CONSTEXPR_CPP20_TEST __cpp_constexpr
#else
#define INTRA_CONSTEXPR_CPP20_TEST 0
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
#elif !defined(__ia64__) && defined(_M_IA64)
#define __ia64__ 1
#elif !defined(__powerpc__) && defined(_M_PPC)
#define __powerpc__ 1
#define __BIG_ENDIAN__ 1
#endif

#ifdef __BYTE_ORDER__
static_assert(__BYTE_ORDER__ != __ORDER_PDP_ENDIAN__);
#endif

#define INTRA_PREPROCESSOR_QUOTE(x) #x
#define INTRAZ_D_CONCATENATE_TOKENS(x, y) x ## y
#define INTRA_CONCATENATE_TOKENS(x, y) INTRAZ_D_CONCATENATE_TOKENS(x, y)

#if defined(__clang__) || defined(__GNUC__)
#define INTRA_WARNING_PUSH _Pragma("GCC diagnostic push")
#define INTRA_WARNING_POP _Pragma("GCC diagnostic pop")
#define INTRAZ_D_WARNING_HELPER(mode, x) INTRA_PREPROCESSOR_QUOTE(GCC diagnostic mode x)
#define INTRAZ_D_IGNORE_WARNING_HELPER(x) INTRAZ_D_WARNING_HELPER(ignored, x)
#define INTRA_IGNORE_WARNING(x) _Pragma(INTRAZ_D_IGNORE_WARNING_HELPER("-W" x))
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
#define INTRA_IGNORE_WARNING(x)
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
#define INTRA_IGNORE_WARNING_CLANG(x) INTRA_IGNORE_WARNING(x)
#else
#define INTRA_IGNORE_WARNING_CLANG(x)
#ifdef __GNUC__
#define INTRA_IGNORE_WARNING_GCC(name) INTRA_IGNORE_WARNING(name)
#define INTRAZ_D_USEFUL_WARNINGS_GCC(W) \
	W("duplicated-cond") \
	W("null-dereference") \
	W("duplicated-branches") \
	W("restrict") \
	W("logical-op") \
	W("useless-cast")
#else
#define INTRA_IGNORE_WARNING_GCC(name)
#define INTRAZ_D_USEFUL_WARNINGS_GCC(W)
#endif
#endif

#ifdef _MSC_VER
#define INTRA_IGNORE_WARNINGS_MSVC(w) __pragma(warning(disable: w))
#define INTRA_PUSH_DISABLE_ALL_WARNINGS __pragma(warning(push, 0)) INTRAZ_D_IGNORE_MOST_WARNINGS
#if _MSC_VER < 1922 || _MSC_VER >= 1925
#define INTRAZ_D_USEFUL_WARNINGS_MSVC_4626 4626
#else //This warning is buggy in MSVC 2019 Update 2-4 when /std:c++latest is enabled with many false positives for lambdas
#define INTRAZ_D_USEFUL_WARNINGS_MSVC_4626
#endif
#define INTRAZ_D_USEFUL_WARNINGS_MSVC(mode) \
	__pragma(warning(mode: 4365 4244 4702 4265 4061 4625 4512 5026 5027 4510 4610 \
		4623 5027 4127 4571 INTRAZ_D_USEFUL_WARNINGS_MSVC_4626 4548 4987 4774 4702 4355 4738))
#else
#define INTRA_IGNORE_WARNINGS_MSVC(w)
#define INTRAZ_D_USEFUL_WARNINGS_MSVC(mode)
#define INTRA_PUSH_DISABLE_ALL_WARNINGS INTRA_WARNING_PUSH INTRAZ_D_IGNORE_MOST_WARNINGS
#endif

#define INTRA_ENABLE_USEFUL_WARNINGS \
	INTRAZ_D_USEFUL_WARNINGS_GCC_CLANG(INTRA_ENABLE_WARNING_OR_ERROR) \
	INTRAZ_D_USEFUL_WARNINGS_GCC(INTRA_ENABLE_WARNING_OR_ERROR) \
	INTRAZ_D_USEFUL_WARNINGS_MSVC(default)

#if defined(_MSC_VER) && !defined(__clang__)
#define INTRA_IGNORE_WARNING_LNK4221 namespace {char INTRA_CONCATENATE_TOKENS($DisableLNK4221_, __COUNTER__);}
#define INTRA_PUSH_ENABLE_USEFUL_WARNINGS __pragma(warning(push, 4)) INTRA_ENABLE_USEFUL_WARNINGS
#else
#define INTRA_IGNORE_WARNING_LNK4221
#define INTRA_PUSH_ENABLE_USEFUL_WARNINGS INTRA_WARNING_PUSH INTRA_ENABLE_USEFUL_WARNINGS
#endif

#define INTRA_IGNORE_WARNING_UNUSED_FUNCTION INTRA_IGNORE_WARNING("unused-function")
#define INTRA_IGNORE_WARNING_UNHANDLED_ENUM_CASES INTRA_IGNORE_WARNINGS_MSVC(4061)
#define INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED INTRA_IGNORE_WARNINGS_MSVC(4625 4626 4512)
#define INTRA_IGNORE_WARNING_MOVE_IMPLICITLY_DELETED INTRA_IGNORE_WARNINGS_MSVC(5026 5027)
#define INTRA_IGNORE_WARNING_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED INTRA_IGNORE_WARNINGS_MSVC(4510 4610 4623)
#define INTRA_IGNORE_WARNING_ASSIGN_IMPLICITLY_DELETED INTRA_IGNORE_WARNINGS_MSVC(4626 5027)
#define INTRA_IGNORE_WARNING_CONSTANT_CONDITION INTRA_IGNORE_WARNINGS_MSVC(4127)
#define INTRA_IGNORE_WARNING_SIGN_CONVERSION INTRA_IGNORE_WARNING("sign-conversion") INTRA_IGNORE_WARNINGS_MSVC(4365)
#define INTRA_IGNORE_WARNING_SIGN_COMPARE INTRA_IGNORE_WARNING("sign-compare") INTRA_IGNORE_WARNINGS_MSVC(4018)
#define INTRA_IGNORE_WARNING_LOSING_CONVERSION INTRA_IGNORE_WARNING("conversion") INTRA_IGNORE_WARNINGS_MSVC(4244)
#define INTRA_IGNORE_WARNING_NO_VIRTUAL_DESTRUCTOR INTRA_IGNORE_WARNING("non-virtual-dtor") INTRA_IGNORE_WARNINGS_MSVC(4265)
#define INTRA_IGNORE_WARNING_UNDEFINED_REINTERPRET_CAST INTRA_IGNORE_WARNING_CLANG("undefined-reinterpret-cast")
#define INTRA_IGNORE_WARNING_LANGUAGE_EXTENSION INTRA_IGNORE_WARNING_CLANG("language-extension-token") INTRA_IGNORE_WARNINGS_MSVC(4201)
#define INTRA_IGNORE_WARNING_GLOBAL_CONSTRUCTION \
	INTRA_IGNORE_WARNING_CLANG("exit-time-destructors") \
	INTRA_IGNORE_WARNING_CLANG("global-constructors")
#define INTRA_IGNORE_WARNING_COPY_MOVE_IMPLICITLY_DELETED \
	INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED \
	INTRA_IGNORE_WARNING_MOVE_IMPLICITLY_DELETED

#define INTRA_IGNORE_WARNING_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED \
	INTRA_IGNORE_WARNING_COPY_MOVE_IMPLICITLY_DELETED \
	INTRA_IGNORE_WARNING_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED

#define INTRA_DISABLE_REDUNDANT_WARNINGS \
	INTRA_IGNORE_WARNING("ctor-dtor-privacy") \
	INTRA_IGNORE_WARNING_CLANG("logical-op-parentheses") \
	INTRA_IGNORE_WARNING_CLANG("c++98-compat") \
	INTRA_IGNORE_WARNING_CLANG("c++98-compat-pedantic") \
	INTRA_IGNORE_WARNING("c++11-compat") \
	INTRA_IGNORE_WARNING_CLANG("documentation-unknown-command") \
	INTRA_IGNORE_WARNING_CLANG("comma") \
	INTRA_IGNORE_WARNING_CLANG("duplicate-enum") \
	INTRA_IGNORE_WARNING_CLANG("implicit-fallthrough") \
	INTRA_IGNORE_WARNING_CLANG("float-equal") \
	INTRA_IGNORE_WARNING_CLANG("reserved-id-macro") \
	INTRA_IGNORE_WARNINGS_MSVC(4514 4710 4714 4820 4711 4577 4868 5045)
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
	INTRA_IGNORE_WARNINGS_MSVC(4548 4987 4774 4702 4355 4738 4571) \
	INTRAZ_D_USEFUL_WARNINGS_GCC_CLANG(INTRA_IGNORE_WARNING) \
	INTRAZ_D_USEFUL_WARNINGS_GCC(INTRA_IGNORE_WARNING) \
	INTRA_DISABLE_REDUNDANT_WARNINGS \
	INTRA_IGNORE_WARNING_LOSING_CONVERSION \
	INTRA_IGNORE_WARNING_SIGN_CONVERSION \
	INTRA_IGNORE_WARNING_SIGN_COMPARE \
	INTRA_IGNORE_WARNING_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED \
	INTRA_IGNORE_WARNING_GLOBAL_CONSTRUCTION \
	INTRA_IGNORE_WARNING_UNDEFINED_REINTERPRET_CAST \
	INTRA_IGNORE_WARNING_LANGUAGE_EXTENSION

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

typedef decltype(sizeof(0)) size_t;

#define INTRA_BEGIN namespace Intra { INTRA_PUSH_ENABLE_USEFUL_WARNINGS INTRA_DISABLE_REDUNDANT_WARNINGS
#define INTRA_END INTRA_WARNING_POP }

INTRA_BEGIN
/** Defines which debug checks should be included into the program.
	0 - no checks
	1 - only checks with time and memory complexity O(1)
	2 - checks with time and memory complexity up to O(N)
*/
constexpr int DebugCheckLevel = INTRA_DEBUG;

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

//! Signed integral type with size dependent on pointer size. Used for index and size arithmetics.
using index_t = decltype(reinterpret_cast<char*>(1)-reinterpret_cast<char*>(0));

constexpr auto null = nullptr; //! Used with null pointers and empty objects

#ifdef __SIZEOF_INT128__
using int128 = __int128;
using uint128 = unsigned __int128;
#endif

template<bool value> struct TBool {static constexpr bool _ = value;};

namespace z_D {
template<typename T> struct TRemoveReference_ {using _ = T;};
template<typename T> struct TRemoveReference_<T&> {using _ = T;};
template<typename T> struct TRemoveReference_<T&&> {using _ = T;};
template<typename T> constexpr bool CLValueReference_ = false;
template<typename T> constexpr bool CLValueReference_<T&> = true;
}
template<typename T> using TRemoveReference = typename z_D::TRemoveReference_<T>::_;

template<typename T>
constexpr INTRA_FORCEINLINE decltype(auto) Move(T&& t) noexcept {return static_cast<TRemoveReference<T>&&>(t);}

template<typename T>
constexpr INTRA_FORCEINLINE T&& Forward(TRemoveReference<T>& t) noexcept {return static_cast<T&&>(t);}
template<typename T>
constexpr INTRA_FORCEINLINE T&& Forward(TRemoveReference<T>&& t) noexcept
{
	static_assert(!z_D::CLValueReference_<T>, "Bad Forward call!");
	return static_cast<T&&>(t);
}

template<typename T> constexpr INTRA_FORCEINLINE auto Dup(T&& t) noexcept
{
	if constexpr(z_D::CLValueReference_<T>) return t;
	else return Move(t);
}

/** Use Owner<T*> to explicitly show that the pointer owns its data.

  Only one pointer can own an object. It can be assigned to other Owner but the previous Owner must be reset.
  @see gsl::owner and its description in C++ Core Guidelines for more information.
*/
template<typename T> using Owner = T;

template<typename... Args> constexpr bool IsConstantEvaluated([[maybe_unused]] Args&&... args) noexcept
{
#ifdef INTRA_IS_CONSTANT_EVALUATED_SUPPORT
	return __builtin_is_constant_evaluated();
#elif defined(__GNUC__) || defined(__clang__)
	return (... && __builtin_constant_p(args));
#elif defined(INTRA_AGRESSIVE_CONSTEXPR)
	return true; //Assume constant evaluation by default. This may decrease runtime performance
#else
	return false; //Assume runtime evaluation by default. Constexpr function call may fail to compile
#endif
}

constexpr bool TargetIsBigEndian =
#ifdef __BIG_ENDIAN__
	true;
#else
	false;
#endif

constexpr bool TargetIsFloatBigEndian =
#ifdef __FLOAT_WORD_ORDER__
	__FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__;
#else
	TargetIsBigEndian;
#endif

constexpr struct {} Construct;
INTRA_IGNORE_WARNING_GCC("effc++")
constexpr struct {template<typename T> auto& operator=(T&) const {return *this;}} _;
INTRA_END

inline void* operator new(size_t, decltype(Intra::Construct), void* dst) {return dst;}
inline void operator delete(void*, decltype(Intra::Construct), void*) noexcept {}
