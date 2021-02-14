#pragma once

#include "Intra/Numeric.h"
#include "Intra/Range/Span.h"
#include "IntraX/Container/ForwardDecls.h"

namespace Intra { INTRA_BEGIN
template<typename T> constexpr Requires<
	CBasicSignedIntegral<T>,
TToUnsigned<T>> ExtractKey(T t)
{
	const auto signBit = 1ull << (sizeof(T)*8-1);
	return TToUnsigned<T>(t ^ T(signBit));
}

template<typename T> constexpr Requires<
	CBasicUnsignedIntegral<T>,
T> ExtractKey(T t) {return t;}

template<typename T> INTRA_FORCEINLINE size_t ExtractKey(T* t) {return reinterpret_cast<size_t>(t);}

template<typename T, typename ExtractKeyFunc = TToUnsigned<T>(*)(T), size_t RadixLog=8>
void RadixSort(Span<T> arr, ExtractKeyFunc extractKey = &ExtractKey<T>);

template<typename T, typename ExtractKeyFunc = size_t(*)(T*), size_t RadixLog=8>
constexpr void RadixSort(Span<T*> arr, ExtractKeyFunc extractKey = &ExtractKey<T>)
{RadixSort(arr.template ReinterpretUnsafe<size_t>(), extractKey);}

namespace z_D
{
	template<typename T, typename ExtractKeyFunc, size_t RadixLog> constexpr void radixSortPass(
		Span<T> arr, Span<T> temp, size_t radixOffset, ExtractKeyFunc extractKey)
	{
		enum: size_t {Radix = 1 << RadixLog, RadixMask = Radix-1};
		const size_t count = arr.Length();

		unsigned c[Radix] = {0};
		for(size_t j = 0; j < count; j++)
		{
			const auto key = extractKey(arr[j]);
			const size_t keyRadix = size_t(key >> radixOffset) & RadixMask;
			c[keyRadix]++;
		}
		for(size_t j = 1; j < Radix; j++) c[j] += c[j-1];
		for(size_t j = count-1; j != MaxValueOf<size_t>; j--)
		{
			const auto key = extractKey(arr[j]);
			const size_t keyRadix = size_t(key >> radixOffset) & RadixMask;
			const size_t tempIndex = --c[keyRadix];
			temp[tempIndex] = arr[j];
		}
	}
}

template<typename T, typename ExtractKeyFunc, size_t RadixLog>
void RadixSort(Span<T> arr, ExtractKeyFunc extractKey)
{
	enum: size_t {KeyBits = sizeof(extractKey(Val<T>()))*8};

	Array<T> tempBuffer;
	tempBuffer.SetCountUninitialized(arr.Length());
	Span<T> dst = arr, temp = tempBuffer;
	size_t shift = 0;
	for(size_t i = 0; i < KeyBits / RadixLog; i++)
	{
		z_D::radixSortPass<T, ExtractKeyFunc, RadixLog>(dst, temp, shift, extractKey);
		shift += RadixLog;
		Swap(dst, temp);
	}
	if(shift < KeyBits)
	{
		z_D::radixSortPass<T, ExtractKeyFunc, KeyBits % RadixLog>(dst, temp, shift, extractKey);
		Swap(dst, temp);
	}
	if(arr==dst) return;
	CopyTo(temp, arr);
}
} INTRA_END
