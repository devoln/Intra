#pragma once

#include <Intra/Concepts.h>
#include <Intra/Functional.h>
#include <Intra/Platform/Toolchain.h>
#include <Intra/Numeric/Bits.h>

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_CONSTANT_CONDITION

/** Memory copying.
  [dst; dst + count) and [src; src + count) must not overlap.
*/
template<CTriviallyCopyAssignable T> INTRA_FORCEINLINE constexpr void MemoryCopy(TUnsafe, T* dst, const T* src, size_t count) noexcept
{
	INTRA_PRECONDITION(src + count <= dst || src >= dst + count);
	if(IsConstantEvaluated())
	{
		for(size_t i = 0; i < count; i++) dst[i] = src[i];
		return;
	}
	z_D::memcpy(dst, src, count * sizeof(T));
}

/// Same as MemoryCopy but optimized for small copies of <= 8 bytes
INTRA_FORCEINLINE constexpr void MemoryCopySmall8(TUnsafe, char* dst, const char* src, size_t n) noexcept
{
	if(IsConstantEvaluated() || Config::DisableLargeCodeSizeFootprintOptimizations) MemoryCopy(Unsafe, dst, src, n);
	else switch(n)
	{
	default: z_D::memcpy(dst, src, n); return;
	case 8: case 7: case 6: case 5:
		z_D::memcpy(dst + n - 4, src + n - 4, 4);
	case 4:
		z_D::memcpy(dst, src, 4);
		break;
	case 3: dst[2] = src[2];
	case 2:
		z_D::memcpy(dst, src, 2);
		break;
	case 1: *dst = *src;
	case 0:;
	}
}

/// Memory copying that can be used for shifting arrays to the left.
template<CTriviallyCopyAssignable T> INTRA_FORCEINLINE constexpr void MemoryCopyForward(TUnsafe, T* dst, const T* src, size_t count) noexcept
{
	INTRA_PRECONDITION(src + count <= dst || src >= dst);
	if(IsConstantEvaluated())
	{
		for(size_t i = 0; i < count; i++) dst[i] = src[i];
		return;
	}
	z_D::memmove(dst, src, count*sizeof(T));
}

/// Memory copying that can be used for shifting arrays to the right.
template<CTriviallyCopyAssignable T> INTRA_FORCEINLINE constexpr void MemoryCopyBackwards(TUnsafe, T* dst, const T* src, Size count) noexcept
{
	if(IsConstantEvaluated())
	{
		for(index_t i = index_t(count) - 1; i >= 0; i--) dst[i] = src[i];
		return;
	}
	z_D::memmove(dst, src, size_t(count)*sizeof(T));
}

template<CChar T> [[nodiscard]] INTRA_FORCEINLINE constexpr index_t CStringLength(const T* str) noexcept
{
	if constexpr(CSame<T, char>) return index_t(__builtin_strlen(str));
	else if constexpr(CSame<T, wchar_t>) return index_t(__builtin_wcslen(str));
	else
	{
		if constexpr(CSameSize<T, wchar_t>)
			if(!IsConstantEvaluated())
				return index_t(__builtin_wcslen(reinterpret_cast<const wchar_t*>(str)));
		index_t i = 0;
		for(; str[i]; i++);
		return i;
	}
}

template<CTriviallySerializable T, size_t NumOutputBytes = sizeof(T), CSameSize<uint8> Byte = uint8> requires (!CConst<Byte>)
INTRA_FORCEINLINE constexpr void BinarySerialize(TUnsafe, Byte* dst, TExplicitType<T> x) noexcept
{
	static_assert(!CBasicFloatingPoint<T> || NumOutputBytes == sizeof(T));
	if constexpr(NumOutputBytes == sizeof(T))
	{
		if(!IsConstantEvaluated())
		{
		#if defined(__GNUC__) || defined(__clang__)
			__builtin_memcpy(dst, &x, NumOutputBytes);
		#else
			// MSVC doesn't optimize memcpy call in low optimization level builds (debug and min size without -Oi).
			// Fortunately, MSVC doesn't implement any strict aliasing optimizations we have to worry about.
			*reinterpret_cast<T*>(dst) = x;
		#endif
			return;
		}
	}

	if constexpr(CArithmetic<T>)
	{
		constexpr bool AsBigEndian = CBasicFloatingPoint<T>? Config::TargetIsFloatBigEndian: Config::TargetIsBigEndian;
		auto v = BitCastTo<TToIntegral<T>>(x);
		if constexpr((NumOutputBytes <= sizeof(T))) INTRA_PRECONDITION(BitWidth<TToIntegral<T>>(v) <= (NumOutputBytes * 8));
		for(int i = 0; i < NumOutputBytes; i++)
		{
			const auto byteIndex = AsBigEndian? sizeof(v) - 1 - i: i;
			dst[i] = Byte((v >> (byteIndex * 8)) & 0xFF);
		}
	}
}

template<CTriviallySerializable T, size_t NumInputBytes = sizeof(T), CSameSize<uint8> Byte = uint8>
[[nodiscard]] INTRA_FORCEINLINE constexpr T BinaryDeserialize(TUnsafe, const Byte* src) noexcept
{
	static_assert(!CBasicFloatingPoint<T> || NumInputBytes == sizeof(T));
	if constexpr(NumInputBytes == sizeof(T)) if(!IsConstantEvaluated())
	{
		T x{};
	#if defined(__GNUC__) || defined(__clang__)
		__builtin_memcpy(&x, src, NumInputBytes);
	#else
		// MSVC doesn't optimize memcpy call in low optimization level builds (debug and min size without -Oi).
		// Fortunately, MSVC doesn't implement any strict aliasing optimizations we have to worry about.
		x = *reinterpret_cast<const T*>(src);
	#endif
		return x;
	}
	using UInt = TUnsignedIntOfSizeAtLeast<sizeof(T)>;
	UInt v{};
	if constexpr(CBasicIntegral<T>)
	{
		for(int i = 0; i < NumInputBytes; i++)
		{
			const auto byteIndex = Config::TargetIsBigEndian? NumInputBytes - 1 - i: i;
			v |= T(T(TToUnsigned<Byte>(src[i])) << T(byteIndex * 8));
		}
	}
	return BitCastTo<T>(v);
}

template<CBasicIntegral T> INTRA_FORCEINLINE constexpr T SwapByteOrder(TExplicitType<T> x)
{
	if constexpr(CSameSize<T, int8>) return x;
	else if(IsConstantEvaluated())
	{
		if constexpr(CSameSize<T, int16>)
			return T(((uint16(x) & 0xFF) << 8)|((uint16(x) & 0xFF00) >> 8));
		else if constexpr(CSameSize<T, int32>)
			return T((SwapByteOrder<uint16>(uint16(x)) << 16) | SwapByteOrder<uint16>(uint16(uint32(x) >> 16)));
		else if constexpr(CSameSize<T, int64>)
			return T((uint64(SwapByteOrder<uint32>(uint32(x))) << 32) | SwapByteOrder<uint32>(uint32(uint64(x) >> 32)));
	}
	else
	{
	#if defined(_MSC_VER) && !defined(__clang__)
		if constexpr(sizeof(T) == sizeof(int16)) return T(z_D::_byteswap_ushort(uint16(x)));
		else if constexpr(sizeof(T) == sizeof(int32)) return T(z_D::_byteswap_ulong(uint32(x)));
		else if constexpr(sizeof(T) == sizeof(int64)) return T(z_D::_byteswap_uint64(uint64(x)));
	#else
		if constexpr(sizeof(T) == sizeof(int16)) return T(__builtin_bswap16(uint16(x)));
		else if constexpr(sizeof(T) == sizeof(int32)) return T(__builtin_bswap32(uint32(x)));
		else if constexpr(sizeof(T) == sizeof(int64)) return T(__builtin_bswap64(uint64(x)));
	#endif
	}
}
#if INTRA_CONSTEXPR_TEST
static_assert(SwapByteOrder<uint8>(0x47) == 0x47);
static_assert(SwapByteOrder<uint16>(0x1234) == 0x3412);
static_assert(SwapByteOrder<uint32>(0x12345678) == 0x78563412);
static_assert(SwapByteOrder<uint64>(0x0123456789ABCDEF) == 0xEFCDAB8967452301);
#endif

[[nodiscard]] constexpr bool ContainsSubrange(CConvertibleToSpan auto&& list, CConvertibleToSpan auto&& sublist) noexcept
{
	return Data(list) <= Data(sublist) &&
		   Data(list) + Length(list) >= Data(sublist) + Length(sublist);
}

template<CConvertibleToSpan L> [[nodiscard]] INTRA_FORCEINLINE constexpr bool ContainsAddress(L&& arr, TArrayElementPtr<L> address) noexcept
{
	return size_t(address - Data(arr)) < size_t(Length(arr));
}

template<CConvertibleToSpan L1, CSameArrays<L1> L2> [[nodiscard]] constexpr bool Overlaps(L1&& list1, L2&& list2) noexcept
{
	return Data(list1) < Data(list2) + Length(list2) &&
		   Data(list2) < Data(list1) + Length(list1) &&
		   Length(list1) != 0 && Length(list2) != 0;
}

template<CRange R> requires(sizeof(TRangeValue<R>) == 1 && !CConst<R>)
constexpr uint64 ParseVarUInt(R& src)
{
	uint64 result = 0;
	uint8 l = 0;
	do
	{
		if(src.Empty()) return result;
		l = uint8(src.First());
		src.PopFirst();
		result = (result << 7) | (l & 0x7F);
	} while(l & 0x80);
	return result;
}

// Encode an integer as a leading byte containing the length and part of the value followed by more significant bytes that use all their bits to encode an integer.
// This encoding method averages at 7-bits per byte: (value 0-127 is encoded as 1 byte, 128-16383 as 2 bytes, etc.)
// NOTE: dst buffer must always have at least 9 bytes which may mean that caller must provide a padding of at least 8 writable bytes!
//  To avoid branches and loops this implementation writes up to 9 bytes to dst even if it's more than necessary.
//  Redundant bytes are set to zero and can be safely overwritten by the caller later.
// @return The number of bytes encoding the value in dst.
template<CBasicUnsignedIntegral T> INTRA_NOINLINE size_t INTRA_FASTCALL EncodeP8UintLEUnsafe(T value, char* dst)
{
	T varValue = 0;
	uint8 varValueLast = 0;
	size_t numBytes = 1;
	if(value)
	{
		const auto payloadBitsNeeded = sizeof(T) * 8 - CountLeadingZeros<T>(value);
		numBytes = (payloadBitsNeeded + 6) / 7;
		const auto lengthMask = (1U << (numBytes - 1)) - 1;
		varValue = (value << numBytes) | lengthMask;
		if constexpr(sizeof(T) > sizeof(WidestFastInt)) // avoid __aullshr on 32-bit platforms
			varValueLast = uint8(uint32(value >> 32) >> ((sizeof(T) - 4) * 8 - numBytes));
		else varValueLast = uint8(value >> (sizeof(T) * 8 - numBytes));
	}
	if constexpr(Config::TargetIsBigEndian) varValue = SwapByteOrder<T>(varValue);
	BinarySerialize<T>(Unsafe, dst, varValue);
	dst[sizeof(T)] = varValueLast;
	return numBytes;
}

// Decode an integer as a leading byte containing the length and part of the value followed by more significant bytes that use all their bits to encode an integer.
// This encoding method averages at 7-bits per byte: (value 0-127 is encoded as 1 byte, 128-16383 as 2 bytes, etc.)
// NOTE: src buffer must always have at least 9 bytes which may mean that caller must provide a padding of at least 8 readable bytes!
//  To avoid branches and loops this implementation reads up to 9 bytes from src even if it's more than necessary.
//  Redundant bytes are masked off but still can trigger an address sanitizer or access violation error if they are outside of range.
// @return Decoded integer.
template<CBasicUnsignedIntegral T> INTRA_NOINLINE size_t INTRA_FASTCALL DecodeP8UintLEUnsafe(T& value, const char* src)
{
	if constexpr(sizeof(T) < 4)
	{
		uint32 val;
		const auto numBytes = DecodeP8UintLE<uint32>(val, src);
		value = T(val);
		return numBytes;
	}
	T varValue = BinaryDeserialize<T>(Unsafe, src, sizeof(varValue));
	if constexpr(Config::TargetIsBigEndian) varValue = SwapByteOrder<T>(varValue);
	size_t numBytes = 1 + CountTrailingZeros<uint32_t>(uint32_t(~varValue));
	T highBits = 0;
	if(numBytes > sizeof(T))
		highBits = T(src[sizeof(T)]) << (sizeof(T) * 7);
	else varValue &= ~T() >> ((sizeof(T) - numBytes) * 8); // mask off redundant bytes
	value = (varValue >> numBytes) | highBits;
	return numBytes;
}

} INTRA_END
