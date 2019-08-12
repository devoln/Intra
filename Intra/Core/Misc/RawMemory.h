#pragma once

#include "Core/Type.h"
#include "Core/Functional.h"
#include "Core/Assert.h"

/** This header imports necessary mem* and strlen C Runtime functions, and tries to implement their constexpr alternatives.
  By default these alternatives are constexpr only if it does not hurt runtime performance.
  #define INTRA_AGRESSIVE_CONSTEXPR to override this default.

  Prefer using safe Core/Range package which uses these functions under the hood for optimization.
*/

INTRA_CORE_BEGIN
INTRA_WARNING_DISABLE_CONSTANT_CONDITION

#ifdef _MSC_VER
//Used to import functions from CRT without including its headers
#define INTRA_CRTDECL __cdecl
#else
#define INTRA_CRTDECL
#endif

#ifdef INTRA_CONSTEXPR_BUILTIN_MEM_SUPPORT
#define INTRA_MEM_CONSTEXPR constexpr
#else
#define INTRA_MEM_CONSTEXPR INTRA_CONSTEXPR2
#endif

namespace Misc {

namespace C {
#ifdef _MSC_VER
extern "C" {
int INTRA_CRTDECL memcmp(const void* buf1, const void* buf2, size_t size) noexcept;
size_t INTRA_CRTDECL strlen(const char* str) noexcept;
size_t INTRA_CRTDECL wcslen(const wchar_t* str) noexcept;
void* INTRA_CRTDECL memcpy(void* dst, const void* src, size_t size) noexcept;
void* INTRA_CRTDECL memmove(void* dst, const void* src, size_t size) noexcept;
void* INTRA_CRTDECL memset(void* dst, int val, size_t size) noexcept;
#elif defined(__GNUC__) || defined(__clang__)
#ifdef INTRA_CONSTEXPR_BUILTIN_MEM_SUPPORT
#define INTRA_MEM_CONSTEXPR1 constexpr
#else
#define INTRA_MEM_CONSTEXPR1
#endif
INTRA_MEM_CONSTEXPR1 forceinline int memcmp(const void* buf1, const void* buf2, size_t size) noexcept {return __builtin_memcmp(buf1, buf2, size);}
INTRA_MEM_CONSTEXPR1 forceinline size_t strlen(const char* str) noexcept {return __builtin_strlen(str);}
INTRA_MEM_CONSTEXPR1 forceinline size_t wcslen(const wchar_t* str) noexcept {return __builtin_wcslen(str);}
forceinline void* memcpy(void* dst, const void* src, size_t size) noexcept {return __builtin_memcpy(dst, src, size);}
forceinline void* memmove(void* dst, const void* src, size_t size) noexcept {return __builtin_memmove(dst, src, size);}
forceinline void* memset(void* dst, int val, size_t size) noexcept {return __builtin_memset(dst, val, size);}
#undef INTRA_MEM_CONSTEXPR1
#endif
//These functions are not used by Core itself but may be used by other parts of Intra.
//TODO: move them where they are used.
void* INTRA_CRTDECL malloc(size_t bytes) noexcept;
void* INTRA_CRTDECL realloc(void* oldPtr, size_t bytes) noexcept;
void INTRA_CRTDECL free(void* ptr) noexcept;
}
}



INTRA_OPTIMIZE_FUNCTION(template<typename T>) INTRA_MEM_CONSTEXPR forceinline bool BitsEqual(const T* a, const T* b, size_t count) noexcept
{
#if !defined(INTRA_CONSTEXPR_BUILTIN_MEM_SUPPORT) && defined(__cpp_constexpr) && __cpp_constexpr >= 201304 && (!defined(_MSC_VER) || defined(INTRA_AGRESSIVE_CONSTEXPR))
	if(IsConstantEvaluated(a, b, count, *a, *b))
	{
		//Clang < 4 may get here i runtime if __builtin_constant_p method gives a false positive
		//2017 <= MSVC < <C++20 version> always get here in runtime
		//MSVC vectorizes these loops for types with sizeof(T) <= 2 regardless of optimization flags because INTRA_OPTIMIZE_FUNCTION overrides them even in debug mode
		enum {unrollFactor = 32 / sizeof(T)};
		if(sizeof(T) <= sizeof(char16_t))
		{
			const size_t k = count / unrollFactor;
			for(size_t i = 0; i < k; i++)
			{
				T c = 0;
				for(size_t j = 0; j < unrollFactor; j++)
					c |= a[j] ^ b[j];
				if(c != 0) return false;
				a += unrollFactor;
				b += unrollFactor;
			}
			count %= unrollFactor;
		}
		if(sizeof(T) == sizeof(char) && unrollFactor >= 32 && count >= unrollFactor / 2)
		{
			T c = 0;
			for(size_t j = 0; j < unrollFactor / 2; j++)
				c |= a[j] ^ b[j];
			if(c != 0) return false;
			a += unrollFactor / 2;
			b += unrollFactor / 2;
			count %= unrollFactor / 2;
		}
		for(size_t i = 0; i < count; i++)
			if(a[i] != b[i]) return false;
		return true;
	}
#endif
	//constexpr since GCC 4.6 and Clang 4.0 (in this case INTRA_CONSTEXPR_BUILTIN_MEM_SUPPORT is defined)
	return C::memcmp(a, b, count*sizeof(T)) == 0;
}
INTRA_OPTIMIZE_FUNCTION_END

/** Bitwise copying.

  [\p dst; \p dst + \p count) and [\p src; \p src + \p count) must not overlap.
*/
INTRA_OPTIMIZE_FUNCTION(template<typename T>) INTRA_CONSTEXPR2 forceinline void CopyBits(T* __restrict dst, const T* __restrict src, size_t count) noexcept
{
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304 && (defined(INTRA_AGRESSIVE_CONSTEXPR) || !defined(_MSC_VER) || INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86_64)
	if(IsConstantEvaluated(dst, src, count, *dst, *src) && CTriviallyCopyAssignable<T>)
	{
		//Due to using keyword __restrict compiler will replace this to call with memcpy or memmove:
		//GCC x86/x64/ARM and MSVC x64 do this regardless of optimization flags because INTRA_OPTIMIZE_FUNCTION overrides them to -O3 even in debug mode
		//Clang does this with -O1 and higher.
		//For x86 (32-bit) MSVC does this only for single byte types (char, byte): shorts and ints are slow on 32-bit.
		for(size_t i = 0; i < count; i++) dst[i] = src[i];
		return;
	}
#endif
	INTRA_DEBUG_ASSERT(src + count <= dst || src >= dst + count);
	// TODO: need more perf tests: memmove was found to be faster on some compiler+OS combinations, and there was no visible difference on others
	//C::memmove(dst, src, count*sizeof(T));
	C::memcpy(dst, src, count*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

INTRA_OPTIMIZE_FUNCTION(template<typename T>) INTRA_CONSTEXPR2 forceinline void CopyBitsBackwards(T* dst, const T* src, size_t count) noexcept
{
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304 && (!defined(_MSC_VER) || defined(INTRA_AGRESSIVE_CONSTEXPR))
	if(IsConstantEvaluated(dst, src, count, *dst, *src) && CTriviallyCopyAssignable<T>)
	{
		//Only MSVC 2017+ and maybe Clang < 9 get here in runtime. This implementation is slow
		for(intptr i = count-1; i >= 0; i--) dst[i] = src[i];
		return;
	}
#endif
	C::memmove(dst, src, count*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

/** Bitwise zero.
  [\p dst; \p dst + \p count) and [\p src; \p src + \p count) must not overlap.
*/
INTRA_OPTIMIZE_FUNCTION(template<typename T>) INTRA_CONSTEXPR2 forceinline void BitwiseZero(T* dst, size_t count) noexcept
{
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304 && (!defined(_MSC_VER) || defined(INTRA_AGRESSIVE_CONSTEXPR))
	if(IsConstantEvaluated(dst, count, *dst) && CTriviallyCopyAssignable<T>)
	{
		//Compiler will replace this to call with memset:
		//GCC x86/x64/ARM do this regardless of optimization flags because INTRA_OPTIMIZE_FUNCTION overrides them to -O3 even in debug mode
		//Clang does this with -O1 and higher.
		//MSVC does not do this, it uses rep stos*, it may be fast too, needs testing.
		//MSVC 2015 doesn't get here in runtime and future C++20 conforming MSVC won't either
		for(size_t i = 0; i < count; i++) dst[i] = 0;
		return;
	}
#endif
	INTRA_DEBUG_ASSERT(src + count <= dst || src >= dst + count);
	C::memset(dst, 0, count*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

#if !defined(_MSC_VER) || defined(INTRA_AGRESSIVE_CONSTEXPR)
INTRA_MEM_CONSTEXPR
#endif
forceinline size_t CStringLength(const char* str) noexcept
{
#if !defined(INTRA_CONSTEXPR_BUILTIN_MEM_SUPPORT) && defined(__cpp_constexpr) && __cpp_constexpr >= 201304 && (!defined(_MSC_VER) || defined(INTRA_AGRESSIVE_CONSTEXPR))
	if(IsConstantEvaluated(str, *str))
	{
		//This implementation is slow but only Clang 3.x, MSVC 2017 and 2019 get here in runtime and only when INTRA_AGRESSIVE_CONSTEXPR is activated
		size_t i = 0;
		for(; str[i]; i++);
		return i;
	}
#endif
	return C::strlen(str);
}

#if !defined(_MSC_VER) || defined(INTRA_AGRESSIVE_CONSTEXPR)
INTRA_MEM_CONSTEXPR
#endif
forceinline size_t CStringLength(const wchar_t* str) noexcept
{
#if !defined(INTRA_CONSTEXPR_BUILTIN_MEM_SUPPORT) && defined(__cpp_constexpr) && __cpp_constexpr >= 201304 && (!defined(_MSC_VER) || defined(INTRA_AGRESSIVE_CONSTEXPR))
	if(IsConstantEvaluated(str, *str)) //TODO: this check may not work correctly on Clang < 9, so it may be necessary to modify #if
	{
		//This implementation is slow but only Clang 3.x, MSVC 2017 and 2019 get here in runtime if INTRA_AGRESSIVE_CONSTEXPR is defined
		size_t i = 0;
		for(; str[i]; i++);
		return i;
	}
#endif
	return C::wcslen(str);
}

#if !defined(_MSC_VER) || defined(INTRA_AGRESSIVE_CONSTEXPR)
INTRA_MEM_CONSTEXPR
#endif
forceinline size_t CStringLength(const char16_t* str) noexcept
{
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304 && (!defined(_MSC_VER) || defined(INTRA_AGRESSIVE_CONSTEXPR))
	if(IsConstantEvaluated(str, *str) || INTRA_PLATFORM_OS != INTRA_PLATFORM_OS_Windows)
#endif
	{
		size_t i = 0;
		for(; str[i]; i++);
		return i;
	}
	return C::wcslen(reinterpret_cast<const wchar_t*>(str));
}

#if !defined(_MSC_VER) || defined(INTRA_AGRESSIVE_CONSTEXPR)
INTRA_MEM_CONSTEXPR
#endif
forceinline size_t CStringLength(const char32_t* str) noexcept
{
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304 && (!defined(_MSC_VER) || defined(INTRA_AGRESSIVE_CONSTEXPR))
	if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows || IsConstantEvaluated(str, *str))
#endif
	{
		size_t i = 0;
		for(; str[i]; i++);
		return i;
	}
	return C::wcslen(reinterpret_cast<const wchar_t*>(str));
}

template<typename Int, typename Byte> INTRA_CONSTEXPR2 inline Requires<
	CIntegral<Int> &&
	sizeof(Byte) == 1
> BinarySerializeLE(Int x, Byte* dst) noexcept
{
	INTRA_DEBUG_ASSERT(dst.Length() >= sizeof(x));
#if defined(INTRA_IS_CONSTANT_EVALUATED_SUPPORT) || defined(__GNUC__) && !defined(__clang__) || defined(INTRA_AGRESSIVE_CONSTEXPR) || INTRA_PLATFORM_ENDIANESS != INTRA_PLATFORM_ENDIANESS_LittleEndian
	if(INTRA_PLATFORM_ENDIANESS != INTRA_PLATFORM_ENDIANESS_LittleEndian || IsConstantEvaluated(x))
	{
		for(int i = 0; i < sizeof(x); i++)
			dst[i] = char((x >> (i*8)) & 0xFF);
	}
	else
#endif
	{
		C::memcpy(dst, &x, sizeof(x));
	}
}

#ifdef INTRA_CONSTEXPR_BITCAST_SUPPORT
//TODO: constexpr version of BitCast in C++20
#else
template<class To, class From> To BitCast(const From& from) noexcept
{
	static_assert(sizeof(From) == sizeof(To), "Invalid bit cast!");
	To to;
	C::memcpy(&to, &from, sizeof(to));
	return to;
}
#endif

}
INTRA_CORE_END
