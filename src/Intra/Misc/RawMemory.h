#pragma once

#include "Intra/Assert.h"
#include "Intra/Functional.h"
#include "Intra/Concepts.h"
#include "Intra/TypeSafe.h"
#include "Intra/Range/Span.h"

/** This header implements generic constexpr alternatives to the mem* and strlen C Runtime functions.

  Consider using safe algorithms defined in Range directory.
*/

INTRA_BEGIN
INTRA_IGNORE_WARNING_CONSTANT_CONDITION

namespace Misc {

namespace CImp {
#if defined(__GNUC__) || defined(__clang__)
INTRA_FORCEINLINE void* memcpy(void* dst, const void* src, size_t size) noexcept {return __builtin_memcpy(dst, src, size);}
INTRA_FORCEINLINE void* memmove(void* dst, const void* src, size_t size) noexcept {return __builtin_memmove(dst, src, size);}
INTRA_FORCEINLINE void* memset(void* dst, int val, size_t size) noexcept {return __builtin_memset(dst, val, size);}
#elif defined(_MSC_VER)
extern "C" {
	INTRA_CRTIMP void* INTRA_CRTDECL memcpy(void* dst, const void* src, size_t size);
	INTRA_CRTIMP void* INTRA_CRTDECL memmove(void* dst, const void* src, size_t size);
	INTRA_CRTIMP void* INTRA_CRTDECL memset(void* dst, int val, size_t size);
}
#endif
}

/** Bitwise copying.
  [\p dst; \p dst + \p count) and [\p src; \p src + \p count) must not overlap.
*/
INTRA_OPTIMIZE_FUNCTION(template<typename T, typename = Requires<CTriviallyCopyAssignable<T>>>)
constexpr void BitwiseCopyUnsafe(T* __restrict dst, const T* __restrict src, Size count) noexcept
{
	INTRA_PRECONDITION(src + size_t(count) <= dst || src >= dst + size_t(count));
	if(IsConstantEvaluated(dst, src, count, *dst, *src))
	{
        //Due to using keyword __restrict compiler will replace this to call with memcpy or memmove:
        //GCC x86/x64/ARM and MSVC x64 do this regardless of optimization flags because INTRA_OPTIMIZE_FUNCTION overrides them to -O3 even in debug mode
        //Clang does this with -O1 and higher.
        //For x86 (32-bit) MSVC does this only for single byte types (char, byte): shorts and ints are slow on 32-bit.
		for(size_t i = 0; i < size_t(count); i++) dst[i] = src[i];
		return;
	}
	// TODO: need more perf tests: memmove was found to be faster on some compiler+OS combinations,
	//  and there was no visible difference on others
	//CImp::memmove(dst, src, size_t(count)*sizeof(T));
	CImp::memcpy(dst, src, size_t(count)*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

INTRA_OPTIMIZE_FUNCTION(template<typename T, typename = Requires<CTriviallyCopyAssignable<T>>>)
constexpr void BitwiseCopyBackwardsUnsafe(T* dst, const T* src, Size count) noexcept
{
	if(IsConstantEvaluated(dst, src, count, *dst, *src))
	{
		for(index_t i = index_t(count)-1; i >= 0; i--) dst[i] = src[i];
		return;
	}
	CImp::memmove(dst, src, size_t(count)*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

/** Bitwise zero.
  [\p dst; \p dst + \p count) and [\p src; \p src + \p count) must not overlap.
*/
INTRA_OPTIMIZE_FUNCTION(template<typename T, typename = Requires<CTriviallyCopyAssignable<T>>>)
constexpr void BitwiseZero(T* dst, Size count) noexcept
{
	if(IsConstantEvaluated(dst, count, *dst))
	{
		for(size_t i = 0; i < size_t(count); i++) dst[i] = 0;
		return;
	}
	CImp::memset(dst, 0, size_t(count)*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

template<typename T, typename = Requires<CChar<T>>>
[[nodiscard]] constexpr index_t CStringLength(const T* str) noexcept
{
    if constexpr(CSame<T, char>) return index_t(__builtin_strlen(str));
    index_t i = 0;
    for(; str[i]; i++);
    return i;
}

template<typename Int, typename Byte, typename = Requires<CIntegral<Int> && sizeof(Byte) == 1>>
constexpr void BinarySerializeLE(Int x, Byte* dst) noexcept
{
	if(TargetIsBigEndian || IsConstantEvaluated(x, dst))
	{
		for(int i = 0; i < sizeof(x); i++)
			dst[i] = Byte((x >> (i*8)) & 0xFF);
	}
	else CImp::memcpy(dst, &x, sizeof(x));
}

template<typename Int, typename Byte, typename = Requires<CIntegral<Int> && sizeof(Byte) == 1>>
[[nodiscard]] constexpr Int BinaryDeserializeLE(Byte* src) noexcept
{
	Int x{};
	if(TargetIsBigEndian || IsConstantEvaluated(src, *src))
	{
		for(int i = 0; i < sizeof(x); i++)
			x |= Int(Int(TToUnsigned<Byte>(src[i])) << Int(i*8));
	}
	else CImp::memcpy(&x, src, sizeof(x));
	return x;
}

#ifdef INTRA_CONSTEXPR_BITCAST_SUPPORT
//TODO: constexpr version of BitCast in C++20
#else
template<class To, class From> [[nodiscard]] To BitCast(const From& from) noexcept
{
	static_assert(sizeof(From) == sizeof(To), "Invalid bit cast!");
	To to;
	CImp::memcpy(&to, &from, sizeof(to));
	return to;
}
#endif

template<class Range, class Subrange, class = Requires<CArrayClass<Range> && CArrayClass<Subrange>>>
[[nodiscard]] constexpr bool ContainsSubrange(Range&& range, Subrange&& subrange) noexcept
{
    return DataOf(range) <= DataOf(subrange) &&
           DataOf(range) + LengthOf(range) >= DataOf(subrange) + LengthOf(subrange);
}

template<typename R, typename = Requires<CArrayClass<R>>>
[[nodiscard]] constexpr bool ContainsAddress(R&& arr, TArrayElementPtr<R> address) noexcept
{
    return size_t(address - DataOf(arr)) < size_t(LengthOf(arr));
}

template<typename R1, typename R2, typename = Requires<CArrayClass<R1> && CArrayClass<R2>>>
[[nodiscard]] constexpr bool Overlaps(R1&& r1, R2&& r2) noexcept
{
    return DataOf(r1) < DataOf(r2) + LengthOf(r2) &&
           DataOf(r2) < DataOf(r1) + LengthOf(r1) &&
           LengthOf(r1) != 0 && LengthOf(r2) != 0;
}

template<typename T = byte> [[nodiscard]] auto SpanOfRaw(void* data, Size bytes) noexcept
{
    return Span<T>(ConstructFromPtr, static_cast<T*>(data), size_t(bytes) / sizeof(T));
}

template<typename T = byte> [[nodiscard]] inline auto SpanOfRaw(const void* data, Size bytes) noexcept
{
    return CSpan<T>(ConstructFromPtr, static_cast<const T*>(data), size_t(bytes) / sizeof(T));
}

template<typename T> [[nodiscard]] inline Span<T> SpanOfRawElements(void* data, Size elements) noexcept
{
    return SpanOfPtr(static_cast<T*>(data), elements);
}
template<typename T> [[nodiscard]] inline CSpan<T> SpanOfRawElements(const void* data, Size elements) noexcept
{
    return SpanOfPtr(static_cast<const T*>(data), elements);
}

template<typename U, class R> [[nodiscard]] inline Span<U> Reinterpret(R&& arr) noexcept
{
    return Span<U>(ConstructFromPtr, reinterpret_cast<U*>(DataOf(arr)),
        reinterpret_cast<U*>(DataOf(arr)+LengthOf(arr)));
}

}
INTRA_END
