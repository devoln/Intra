#pragma once

#include <Intra/Functional.h>
#include <Intra/Concepts.h>

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_CONSTANT_CONDITION

namespace z_D {
#if defined(__GNUC__) || defined(__clang__)
INTRA_FORCEINLINE void* memcpy(void* dst, const void* src, size_t size) noexcept {return __builtin_memcpy(dst, src, size);}
INTRA_FORCEINLINE void* memmove(void* dst, const void* src, size_t size) noexcept {return __builtin_memmove(dst, src, size);}
INTRA_FORCEINLINE void* memset(void* dst, int val, size_t size) noexcept {return __builtin_memset(dst, val, size);}
#elif defined(_MSC_VER)
extern "C" {
	void* __cdecl memcpy(void* dst, const void* src, size_t size);
	void* __cdecl memmove(void* dst, const void* src, size_t size);
	void* __cdecl memset(void* dst, int val, size_t size);
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
constexpr void BitwiseCopy(TUnsafe, T* dst, const T* src, Size count) noexcept
{
	INTRA_PRECONDITION(src + size_t(count) <= dst || src >= dst + size_t(count));
	if(IsConstantEvaluated())
	{
		for(size_t i = 0; i < size_t(count); i++) dst[i] = src[i];
		return;
	}
	z_D::memmove(dst, src, size_t(count)*sizeof(T));
}
INTRA_OPTIMIZE_FUNCTION_END

INTRA_OPTIMIZE_FUNCTION(template<CTriviallyCopyAssignable T>)
constexpr void BitwiseCopyBackwards(TUnsafe, T* dst, const T* src, Size count) noexcept
{
	if(IsConstantEvaluated())
	{
		for(index_t i = index_t(count)-1; i >= 0; i--) dst[i] = src[i];
		return;
	}
	z_D::memmove(dst, src, size_t(count)*sizeof(T));
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

template<CBasicArithmetic T, bool AsBigEndian = false, size_t NumOutputBytes = sizeof(T), CSameSize<byte> Byte = byte>
constexpr void BinarySerialize(TUnsafe, TExplicitType<T> x, Byte* dst) noexcept requires (!CConst<Byte>)
{
	static_assert(!CBasicFloatingPoint<T> || NumOutputBytes == sizeof(T));
	constexpr bool sameEndianess = AsBigEndian == (CBasicFloatingPoint<T>? Config::TargetIsFloatBigEndian: Config::TargetIsBigEndian);
	if constexpr(NumOutputBytes == sizeof(T) && sameEndianess && CTriviallySerializable<T>)
	{
		if(!IsConstantEvaluated())
		{
		#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
			__builtin_memcpy(dst, &x, NumOutputBytes);
		#else
			// MSVC doesn't optimize memcpy call in low optimization level builds (debug and min size without -Oi).
			// Fortunately, MSVC doesn't implement any strict aliasing optimizations we have to worry about.
			x = *reinterpret_cast<T*>(dst);
		#endif
			return;
		}
	}

	if constexpr(CBasicArithmetic<T>)
	{
		auto v = BitCastTo<TToIntegral<T>>(x);
		if constexpr((NumOutputBytes <= sizeof(T))) INTRA_PRECONDITION(BitWidth(v) <= (NumOutputBytes*8));
		for(int i = 0; i < NumOutputBytes; i++)
		{
			const auto byteIndex = AsBigEndian ? sizeof(x) - 1 - i : i;
			dst[i] = Byte((x >> (byteIndex*8)) & 0xFF);
		}
	}
}

template<CBasicArithmetic T, bool AsBigEndian = false, size_t NumInputBytes = sizeof(T), CSameSize<byte> Byte = byte>
[[nodiscard]] constexpr T BinaryDeserialize(TUnsafe, const Byte* src) noexcept
{
	static_assert(!CBasicFloatingPoint<T> || NumInputBytes == sizeof(T));
	constexpr bool sameEndianess = AsBigEndian == (CBasicFloatingPoint<T>? Config::TargetIsFloatBigEndian: Config::TargetIsBigEndian);
	if constexpr(NumInputBytes == sizeof(T) && sameEndianess) if(!IsConstantEvaluated())
	{
		T x{};
	#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
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
			const auto byteIndex = AsBigEndian? NumInputBytes - 1 - i : i;
			v |= T(T(TToUnsigned<Byte>(src[i])) << T(byteIndex*8));
		}
	}
	return BitCastTo<T>(v);
}

template<CBasicIntegral T> constexpr T SwapByteOrder(TExplicitType<T> x)
{
	if constexpr(CSameSize<T, int8>) return x;
	else if(IsConstantEvaluated())
	{
		if constexpr(CSameSize<T, int16>)
			return T(((uint16(x) & 0xFF) << 8)|((uint16(x) & 0xFF00) >> 8));
		else if constexpr(CSameSize<T, int32>)
			return T(SwapByteOrder<uint16>(uint16(x)) | SwapByteOrder<uint16>(uint16(uint32(x) >> 16)));
		else if constexpr(CSameSize<T, int64>)
			return T(SwapByteOrder<uint32>(uint32(x)) | SwapByteOrder<uint32>(uint32(uint64(x) >> 32)));
	}
	else
	{
	#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
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
#if INTRA_CONSTEXPR_TEST
static_assert(SwapByteOrder<uint8>(0x47) == 0x47);
static_assert(SwapByteOrder<uint16>(0x1234) == 0x3412);
static_assert(SwapByteOrder<uint32>(0x12345678) == 0x78563412);
static_assert(SwapByteOrder<uint64>(0x0123456789ABCDEF) == 0xEFCDAB8967452301);
#endif

template<CArrayList L, CArrayList L2> [[nodiscard]] constexpr bool ContainsSubrange(L&& list, L2&& sublist) noexcept
{
	return DataOf(list) <= DataOf(sublist) &&
		   DataOf(list) + LengthOf(list) >= DataOf(sublist) + LengthOf(sublist);
}

template<CArrayList R> [[nodiscard]] constexpr bool ContainsAddress(R&& arr, TArrayElementPtr<R> address) noexcept
{
	return size_t(address - DataOf(arr)) < size_t(LengthOf(arr));
}

template<typename R1, CSameArrays<R1> R2> [[nodiscard]] constexpr bool Overlaps(R1&& r1, R2&& r2) noexcept
{
	return DataOf(r1) < DataOf(r2) + LengthOf(r2) &&
		   DataOf(r2) < DataOf(r1) + LengthOf(r1) &&
		   LengthOf(r1) != 0 && LengthOf(r2) != 0;
}

template<CRange R>
constexpr index_t RawReadTo(R& src, void* dst, Size n)
{
	return ReadTo(src, SpanOfPtr(static_cast<TRangeValue<R>*>(dst), n));
}

template<CList R> constexpr index_t RawCopyTo(R&& src, void* dst, Size n)
{
	auto srcCopy = ForwardAsRange<R>(src);
	return RawReadTo(srcCopy, dst, n);
}

template<CRange R, typename T> requires(!CConst<T>)
constexpr index_t RawReadTo(R& src, Span<T> dst)
{
	const auto dstLen = size_t(dst.Length())*sizeof(T)/sizeof(src.First());
	return RawReadTo(src, dst.Data(), dstLen);
}

template<CRange R, typename T> requires(!CConst<T>)
constexpr index_t RawReadTo(R& src, Span<T> dst, Size n)
{
	const auto dstLen = size_t(dst.Length())*(sizeof(T)/sizeof(src.First()));
	return RawReadTo(src, dst.Data(), Min(size_t(n), dstLen));
}


template<CRange R, CTriviallyCopyable U>
constexpr index_t RawReadWrite(R& src, Span<U>& dst, Size maxElementsToRead)
{
	typedef TRangeValue<R> T;
	auto dst1 = dst.Take(maxElementsToRead).template ReinterpretUnsafe<T>();
	const auto elementsRead = size_t(ReadWrite(src, dst1))*(sizeof(T)/sizeof(U));
	dst.Begin += elementsRead;
	return index_t(elementsRead);
}

template<CRange R, COutput R1> requires CArrayList<R1> && CTriviallyCopyable<TRangeValue<R1>>
constexpr index_t RawReadWrite(R& src, R1& dst, Size maxElementsToRead)
{
	Span<U> dst1 = dst;
	const auto result = RawReadWrite(src, dst1, maxElementsToRead);
	PopFirstExactly(dst, result);
	return result;
}

template<CRange R, COutput R1> requires CTriviallyCopyable<TArrayListValue<R1>>
constexpr index_t RawReadWrite(R& src, R1& dst) {return RawReadWrite(src, dst, dst.Length());}


template<typename T, CRange R> requires(!CConst<R> && !CConst<T>)
constexpr RawRead(R& srcRange, T& dstElement)
{
	static_assert(sizeof(T) % sizeof(srcRange.First()) == 0);
	RawReadTo(srcRange, &dstElement, sizeof(T)/sizeof(srcRange.First()));
}


template<typename T, CRange R> constexpr T RawRead(R& src)
{
	T result{};
	RawRead(src, result);
	return result;
}

template<CRange R> constexpr byte RawReadByte(R& src)
{
	static_assert(sizeof(src.First()) == 1);
	const auto result = src.First();
	src.PopFirst();
	return reinterpret_cast<const byte&>(result);
}

template<class R> requires(sizeof(TRangeValue<R>) == 1 && !CConst<R>)
constexpr uint64 ParseVarUInt(R& src)
{
	uint64 result = 0;
	byte l = 0;
	do
	{
		if(src.Empty()) return result;
		l = byte(src.First());
		src.PopFirst();
		result = (result << 7) | (l & 0x7F);
	} while(l & 0x80);
	return result;
}

template<class R> requires(sizeof(TRangeValue<R>) == 1 && !CConst<R>)
constexpr uint64 ParseVarUInt(R& src, size_t& ioBytesRead)
{
	uint64 result = 0;
	byte l = 0;
	do
	{
		if(src.Empty()) return result;
		l = byte(src.First());
		src.PopFirst();
		ioBytesRead++;
		result = (result << 7) | (l & 0x7F);
	} while(l & 0x80);
	return result;
}


template<typename OR> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, const char* src, Size n)
{
	return WriteTo(SpanOfPtr(src, n), dst);
}

template<typename OR> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, const byte* src, Size n)
{
	return WriteTo(SpanOfPtr(src, n), dst);
}

template<typename OR> Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, const void* src, Size n)
{
	return WriteTo(CSpanOfRaw<char>(src, size_t(n)), dst);
}

template<typename OR, typename T> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, Span<const T> src)
{
	return RawWriteFrom(dst, src.Data(), size_t(src.Length())*sizeof(T));
}

template<typename OR, typename T> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, Span<T> src, Size n)
{
	const auto srcLen = src.Length()*sizeof(T);
	if(n > srcLen) n = srcLen;
	return RawWriteFrom(dst, src.Data(), n);
}

template<typename OR, typename T> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, const T& src)
{
	const auto srcLen = src.Length()*sizeof(T);
	return RawWriteFrom(dst, &src, srcLen);
}


template<typename T, typename OR> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>
> RawWrite(OR& dst, const T& value)
{
	RawWriteFrom(dst, &value, sizeof(T));
}

} INTRA_END
