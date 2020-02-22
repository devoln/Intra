#pragma once

#include "Core/Type.h"
#include "Core/Functional.h"
#include "Core/Assert.h"

/** This header imports necessary mem* and strlen C Runtime functions, and tries to implement their constexpr alternatives.
  By default these alternatives are constexpr only if it does not hurt runtime performance.
  #define INTRA_AGRESSIVE_CONSTEXPR to override this default.

  Prefer using safe Core/Range package which uses these functions under the hood for optimization.
*/

INTRA_BEGIN
INTRA_WARNING_DISABLE_CONSTANT_CONDITION

namespace Misc {

namespace C {
#ifdef INTRA_CONSTEXPR_BUILTIN_STRLEN_SUPPORT
constexpr forceinline size_t strlen(const char* str) noexcept {return __builtin_strlen(str);}
constexpr forceinline size_t wcslen(const wchar_t* str) noexcept {return __builtin_wcslen(str);}
#else
extern "C" {
	size_t INTRA_CRTDECL strlen(const char* str) noexcept;
	size_t INTRA_CRTDECL wcslen(const wchar_t* str) noexcept;
}
#endif
#ifdef INTRA_CONSTEXPR_BUILTIN_MEMCMP_SUPPORT
constexpr forceinline int memcmp(const void* buf1, const void* buf2, size_t size) noexcept {return __builtin_memcmp(buf1, buf2, size);}
#else
extern "C" int INTRA_CRTDECL memcmp(const void* buf1, const void* buf2, size_t size) noexcept;
#endif

#if defined(__GNUC__) || defined(__clang__)
forceinline void* memcpy(void* dst, const void* src, size_t size) noexcept {return __builtin_memcpy(dst, src, size);}
forceinline void* memmove(void* dst, const void* src, size_t size) noexcept {return __builtin_memmove(dst, src, size);}
forceinline void* memset(void* dst, int val, size_t size) noexcept {return __builtin_memset(dst, val, size);}
#elif defined(_MSC_VER)
extern "C" {
	void* INTRA_CRTDECL memcpy(void* dst, const void* src, size_t size) noexcept;
	void* INTRA_CRTDECL memmove(void* dst, const void* src, size_t size) noexcept;
	void* INTRA_CRTDECL memset(void* dst, int val, size_t size) noexcept;
}
#endif
}



INTRA_OPTIMIZE_FUNCTION(template<typename T>) constexpr forceinline bool BitsEqual(const T* a, const T* b, size_t count) noexcept
{
#ifdef INTRA_CONSTEXPR_BUILTIN_MEMCMP_SUPPORT
	//constexpr in: MSVC with -std:c++17 or higher, GCC and Clang 4.0+
	return C::memcmp(a, b, count*sizeof(T)) == 0;
#else
	//Implementing it manually to make it constexpr for clang < 4.0 and MSVC without -std:c++17 or higher
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
#endif
}
INTRA_OPTIMIZE_FUNCTION_END

/** Bitwise copying.

  [\p dst; \p dst + \p count) and [\p src; \p src + \p count) must not overlap.
*/
INTRA_OPTIMIZE_FUNCTION(template<typename T>) constexpr forceinline void CopyBits(T* __restrict dst, const T* __restrict src, index_t count) noexcept
{
	INTRA_PRECONDITION(count >= 0);
	INTRA_PRECONDITION(src + count <= dst || src >= dst + count);
#if defined(INTRA_AGRESSIVE_CONSTEXPR) || !(defined(_MSC_VER) && !defined(__clang__) && INTRA_PLATFORM_ARCH != INTRA_PLATFORM_X86_64)
	if(CTriviallyCopyAssignable<T> && IsConstantEvaluated(dst, src, count, *dst, *src))
	{
		//Due to using keyword __restrict compiler will replace this to call with memcpy or memmove:
		//GCC x86/x64/ARM and MSVC x64 do this regardless of optimization flags because INTRA_OPTIMIZE_FUNCTION overrides them to -O3 even in debug mode
		//Clang does this with -O1 and higher.
		//For x86 (32-bit) MSVC does this only for single byte types (char, byte): shorts and ints are slow on 32-bit.
		for(index_t i = 0; i < count; i++) dst[i] = src[i];
		return;
	}
#endif
	// TODO: need more perf tests: memmove was found to be faster on some compiler+OS combinations, and there was no visible difference on others
	//C::memmove(dst, src, size_t(count)*sizeof(T));
	C::memcpy(dst, src, size_t(count)*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

INTRA_OPTIMIZE_FUNCTION(template<typename T>) constexpr forceinline void CopyBitsBackwards(T* dst, const T* src, index_t count) noexcept
{
	if(CTriviallyCopyAssignable<T> && IsConstantEvaluated(dst, src, count, *dst, *src))
	{
		//Only MSVC and maybe Clang < 9 get here in runtime. This implementation is slow
		for(intptr i = index_t(count)-1; i >= 0; i--) dst[i] = src[i];
		return;
	}
	C::memmove(dst, src, count*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

/** Bitwise zero.
  [\p dst; \p dst + \p count) and [\p src; \p src + \p count) must not overlap.
*/
INTRA_OPTIMIZE_FUNCTION(template<typename T>) constexpr forceinline void BitwiseZero(T* dst, index_t count) noexcept
{
	if(CTriviallyCopyAssignable<T> && IsConstantEvaluated(dst, count, *dst))
	{
		//Compiler will probably replace this to call with memset:
		//GCC x86/x64/ARM does this regardless of optimization flags because INTRA_OPTIMIZE_FUNCTION overrides them to -O3 even in debug mode
		//Clang does this with -O1 and higher.
		//MSVC does not do this, it uses rep stos*, it may be fast too, needs testing.
		//Anyway, future C++20 conforming MSVC won't get here in runtime for any trivial type T.
		for(index_t i = 0; i < count; i++) dst[i] = 0;
		return;
	}
	C::memset(dst, 0, count*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

constexpr forceinline index_t CStringLength(const char* str) noexcept
{
#ifdef INTRA_CONSTEXPR_BUILTIN_STRLEN_SUPPORT
	return index_t(C::strlen(str));
#else
	index_t i = 0;
	for(; str[i]; i++);
	return i;
#endif
}

constexpr forceinline index_t CStringLength(const wchar_t* str) noexcept
{
#ifdef INTRA_CONSTEXPR_BUILTIN_STRLEN_SUPPORT
	return index_t(C::wcslen(str));
#else
	index_t i = 0;
	for(; str[i]; i++);
	return i;
#endif
}

constexpr forceinline index_t CStringLength(const char16_t* str) noexcept
{
	if(sizeof(char16_t) != sizeof(wchar_t) || IsConstantEvaluated(str, *str))
	{
		index_t i = 0;
		for(; str[i]; i++);
		return i;
	}
	return index_t(C::wcslen(reinterpret_cast<const wchar_t*>(str)));
}

constexpr forceinline index_t CStringLength(const char32_t* str) noexcept
{
	if(sizeof(char32_t) != sizeof(wchar_t) || IsConstantEvaluated(str, *str))
	{
		index_t i = 0;
		for(; str[i]; i++);
		return i;
	}
	return index_t(C::wcslen(reinterpret_cast<const wchar_t*>(str)));
}

template<typename Int, typename Byte> constexpr inline Requires<
	CIntegral<Int> &&
	sizeof(Byte) == 1
> BinarySerializeLE(Int x, Byte* dst) noexcept
{
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
INTRA_END
