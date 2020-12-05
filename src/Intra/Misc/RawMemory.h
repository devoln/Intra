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
INTRA_IGNORE_WARN_CONSTANT_CONDITION

namespace z_D {
#if defined(__GNUC__) || defined(__clang__)
INTRA_FORCEINLINE void* memcpy(void* dst, const void* src, size_t size) noexcept {return __builtin_memcpy(dst, src, size);}
INTRA_FORCEINLINE void* memmove(void* dst, const void* src, size_t size) noexcept {return __builtin_memmove(dst, src, size);}
INTRA_FORCEINLINE void* memset(void* dst, int val, size_t size) noexcept {return __builtin_memset(dst, val, size);}
#elif defined(_MSC_VER)
extern "C" {
	INTRA_CRTIMP void* __cdecl memcpy(void* dst, const void* src, size_t size);
	INTRA_CRTIMP void* __cdecl memmove(void* dst, const void* src, size_t size);
	INTRA_CRTIMP void* __cdecl memset(void* dst, int val, size_t size);
	uint16 __cdecl _byteswap_ushort(uint16);
	unsigned long __cdecl _byteswap_ulong(unsigned long);
	uint64 __cdecl _byteswap_uint64(uint64);
#pragma intrinsic(memcpy, memmove, memset, _byteswap_ushort, _byteswap_ulong, _byteswap_uint64)
}
#endif
}

/** Bitwise copying.
  [dst; dst + count) and [src; src + count) must not overlap.
*/
INTRA_OPTIMIZE_FUNCTION(template<CTriviallyCopyAssignable T>)
constexpr void BitwiseCopy(TUnsafe, T* __restrict dst, const T* __restrict src, Size count) noexcept
{
	INTRA_PRECONDITION(src + size_t(count) <= dst || src >= dst + size_t(count));
	if(IsConstantEvaluated(dst, src, count, *dst, *src))
	{
		for(size_t i = 0; i < size_t(count); i++) dst[i] = src[i];
		return;
	}
	z_D::memcpy(dst, src, size_t(count)*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

INTRA_OPTIMIZE_FUNCTION(template<CTriviallyCopyAssignable T>)
constexpr void BitwiseCopyBackwards(TUnsafe, T* dst, const T* src, Size count) noexcept
{
	if(IsConstantEvaluated(dst, src, count, *dst, *src))
	{
		for(index_t i = index_t(count)-1; i >= 0; i--) dst[i] = src[i];
		return;
	}
	z_D::memmove(dst, src, size_t(count)*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

/** Bitwise zero.
  [`dst`; `dst` + `count`) and [`src`; `src` + `count`) must not overlap.
*/
INTRA_OPTIMIZE_FUNCTION(template<CTriviallyCopyAssignable T>)
constexpr void BitwiseZero(T* dst, Size count) noexcept
{
	if(IsConstantEvaluated(dst, count, *dst))
	{
		for(size_t i = 0; i < size_t(count); i++) dst[i] = 0;
		return;
	}
	z_D::memset(dst, 0, size_t(count)*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

template<CChar T> [[nodiscard]] constexpr index_t CStringLength(const T* str) noexcept
{
    if constexpr(CSame<T, char>) return index_t(__builtin_strlen(str));
	else
	{
		index_t i = 0;
		for(; str[i]; i++);
		return i;
	}
}

template<CIntegral Int, typename Byte> requires (!CConst<Byte>) && (sizeof(Byte) == 1)
constexpr void BinarySerializePlatformSpecific(Int x, Byte* dst) noexcept
{
	if(!IsConstantEvaluated(x, dst))
	{
	#if defined(__GNUC__) || defined(__clang__)
		__builtin_memcpy(dst, &x, sizeof(x));
	#else
		// MSVC doesn't optimize memcpy call in low optimization level builds (debug and min size without -Oi).
		// Fortunately, MSVC doesn't implement any strict aliasing optimizations we have to worry about.
		x = *reinterpret_cast<Int*>(dst);
	#endif
		return;
	}
	for(int i = 0; i < sizeof(x); i++)
	{
		const auto byteIndex = Config::TargetIsBigEndian? sizeof(x) - 1 - i: i;
		dst[i] = Byte((x >> (byteIndex*8)) & 0xFF);
	}
}

template<CIntegral Int, typename Byte> requires (!CConst<Byte>) && (sizeof(Byte) == 1)
constexpr void BinarySerializeLE(Int x, Byte* dst) noexcept
{
	if constexpr(!Config::TargetIsBigEndian) BinarySerializePlatformSpecific(x, dst);
	else
	{
		for(int i = 0; i < sizeof(x); i++)
			dst[i] = Byte((x >> (i*8)) & 0xFF);
	}
}

template<CIntegral Int, typename Byte> requires (!CConst<Byte>) && (sizeof(Byte) == 1)
constexpr void BinarySerializeBE(Int x, Byte* dst) noexcept
{
	if constexpr(Config::TargetIsBigEndian) BinarySerializePlatformSpecific(x, dst);
	else
	{
		for(int i = 0; i < sizeof(x); i++)
			dst[i] = Byte((x >> ((sizeof(x) - 1 - i)*8)) & 0xFF);
	}
}

template<CIntegral Int, typename Byte> requires (sizeof(Byte) == 1)
[[nodiscard]] constexpr Int BinaryDeserializePlatformSpecific(const Byte* src) noexcept
{
	Int x{};
	if(!IsConstantEvaluated(src, *src))
	{
	#if defined(__GNUC__) || defined(__clang__)
		__builtin_memcpy(&x, src, sizeof(x));
	#else
		// MSVC doesn't optimize memcpy call in low optimization level builds (debug and min size without -Oi).
		// Fortunately, MSVC doesn't implement any strict aliasing optimizations we have to worry about.
		x = *reinterpret_cast<const Int*>(src);
	#endif
	}
	else for(int i = 0; i < sizeof(x); i++)
	{
		const auto byteIndex = Config::TargetIsBigEndian? sizeof(x) - 1 - i: i;
		x |= Int(Int(TToUnsigned<Byte>(src[i])) << Int(byteIndex*8));
	}
	return x;
}

template<CIntegral Int, typename Byte> requires (sizeof(Byte) == 1)
[[nodiscard]] constexpr Int BinaryDeserializeLE(const Byte* src) noexcept
{
	if constexpr(Config::TargetIsBigEndian)
	{
		Int x{};
		for(int i = 0; i < sizeof(x); i++)
			x |= Int(Int(TToUnsigned<Byte>(src[i])) << Int(i*8));
		return x;
	}
	else return BinaryDeserializePlatformSpecific(src);
}

template<CIntegral Int, typename Byte> requires (sizeof(Byte) == 1)
[[nodiscard]] constexpr Int BinaryDeserializeBE(const Byte* src) noexcept
{
	if constexpr(!Config::TargetIsBigEndian)
	{
		Int x{};
		for(int i = 0; i < sizeof(x); i++)
		{
			x |= Int(TToUnsigned<Byte>(src[i]));
			x <<= 8;
		}
		return x;
	}
	else return BinaryDeserializePlatformSpecific(src);
}

template<class To, class From> constexpr auto BitCastTo = [](const From& from) noexcept
{
	static_assert(sizeof(From) == sizeof(To));
	static_assert(CTriviallyCopyable<To> && CTriviallyCopyable<From>);
	if constexpr(CIntegral<To> && CIntegral<From> || CFloatingPoint<To> && CFloatingPoint<From>) return To(from);
	else
	{
	#ifdef INTRA_CONSTEXPR_BITCAST_SUPPORT
		return __builtin_bit_cast(To, from);
	#else
		if(IsConstantEvaluated(from) && (CIntegral<From> && CFloatingPoint<To> || CFloatingPoint<From> && CIntegral<To>))
		{
			constexpr auto signBitShift = sizeof(From)*8 - 1;
			if constexpr(CIntegral<From> && CFloatingPoint<To>)
			{
				constexpr auto mantissaMask = (From(1) << NumMantissaExplicitBitsOf<To>) - 1;
				constexpr auto exponentMask = (From(1) << ExponentLenOf<To>) - 1;
				return ComposeFloat<To>(
					from & mantissaMask,
					((from >> NumMantissaExplicitBitsOf<To>) & exponentMask) - ExponentBiasOf<To>,
					From >> signBitShift);
			}
			else ExtractMantissa(from) |
				(To(ExtractExponent(from)) << NumMantissaExplicitBitsOf<From>) |
				(To(from < 0) << signBitShift);
		}
		else
		{
			To to;
			__builtin_memcpy(&to, &from, sizeof(to));
			return to;
		}
	#endif
	}
};

template<CIntegral T> constexpr T SwapByteOrder(TExplicitType<T> x)
{
	if constexpr(sizeof(T) == sizeof(int8)) return x;
	else if(IsConstantEvaluated())
	{
		if constexpr(sizeof(T) == sizeof(int16))
			return T(((uint16(x) & 0xFF) << 8)|((uint16(x) & 0xFF00) >> 8));
		else if constexpr(sizeof(T) == sizeof(int32))
			return T(SwapByteOrder<uint16>(uint16(x)) | SwapByteOrder<uint16>(uint16(uint32(x) >> 16)));
		else if constexpr(sizeof(T) == sizeof(int64))
			return T(SwapByteOrder<uint32>(uint32(x)) | SwapByteOrder<uint32>(uint32(uint64(x) >> 32)));
	}
	else
	{
	#if defined(__GNUC__) || defined(__clang__)
		if constexpr(sizeof(T) == sizeof(int16)) return T(__builtin_bswap16(uint16(x)));
		else if constexpr(sizeof(T) == sizeof(int32)) return T(__builtin_bswap32(uint32(x)));
		else if constexpr(sizeof(T) == sizeof(int64)) return T(__builtin_bswap64(uint64(x)));
	#else
		if constexpr(sizeof(T) == sizeof(int16)) return T(z_D::_byteswap_ushort(uint16(x)));
		else if constexpr(sizeof(T) == sizeof(int32)) return T(z_D::_byteswap_ulong(uint32(x)));
		else if constexpr(sizeof(T) == sizeof(int64)) return T(z_D::_byteswap_uint64(uint64(x)));
	#endif
	}
}

template<CArrayList Range, CArrayList Subrange> [[nodiscard]] constexpr bool ContainsSubrange(Range&& range, Subrange&& subrange) noexcept
{
    return DataOf(range) <= DataOf(subrange) &&
           DataOf(range) + LengthOf(range) >= DataOf(subrange) + LengthOf(subrange);
}

template<CArrayList R> [[nodiscard]] constexpr bool ContainsAddress(R&& arr, TArrayElementPtr<R> address) noexcept
{
    return size_t(address - DataOf(arr)) < size_t(LengthOf(arr));
}

template<typename R1, typename R2> requires CSameArrays<R1, R2>
[[nodiscard]] constexpr bool Overlaps(R1&& r1, R2&& r2) noexcept
{
    return DataOf(r1) < DataOf(r2) + LengthOf(r2) &&
           DataOf(r2) < DataOf(r1) + LengthOf(r1) &&
           LengthOf(r1) != 0 && LengthOf(r2) != 0;
}

template<typename T = byte> [[nodiscard]] inline auto SpanOfRaw(TUnsafe, void* data, Size bytes) noexcept
{
    return Span<T>(Unsafe, static_cast<T*>(data), size_t(bytes) / sizeof(T));
}

template<typename T = byte> [[nodiscard]] inline auto SpanOfRaw(TUnsafe, const void* data, Size bytes) noexcept
{
    return CSpan<T>(Unsafe, static_cast<const T*>(data), size_t(bytes) / sizeof(T));
}

template<typename T> [[nodiscard]] inline Span<T> SpanOfRawElements(TUnsafe, void* data, Size elements) noexcept
{
    return SpanOfPtr(static_cast<T*>(data), elements);
}
template<typename T> [[nodiscard]] inline CSpan<T> SpanOfRawElements(TUnsafe, const void* data, Size elements) noexcept
{
    return SpanOfPtr(static_cast<const T*>(data), elements);
}

template<typename U, CArrayList R>
[[nodiscard]] inline Span<U> ReinterpretSpan(TUnsafe, R&& arr) noexcept
{
    return Span<U>(Unsafe, reinterpret_cast<U*>(DataOf(arr)),
        reinterpret_cast<U*>(DataOf(arr)+LengthOf(arr)));
}
INTRA_END
