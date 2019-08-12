#pragma once

#include "Core/Numeric.h"
#include "Core/Range/Span.h"
#include "Container/ForwardDecls.h"

INTRA_BEGIN
inline namespace Range {

template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CSignedIntegral<T>,
TMakeUnsigned<T>> ExtractKey(T t)
{
	const auto signBit = 1ull << (sizeof(T)*8-1);
	return TMakeUnsigned<T>(t ^ T(signBit));
}

template<typename T> constexpr forceinline Requires<
	CUnsignedIntegral<T>,
T> ExtractKey(T t) {return t;}

template<typename T> forceinline size_t ExtractKey(T* t) {return reinterpret_cast<size_t>(t);}

template<typename T, typename ExtractKeyFunc = TMakeUnsigned<T>(*)(T), size_t RadixLog=8>
void RadixSort(Span<T> arr, ExtractKeyFunc extractKey = &ExtractKey<T>);

template<typename T, typename ExtractKeyFunc = size_t(*)(T*), size_t RadixLog=8>
INTRA_CONSTEXPR2 forceinline void RadixSort(Span<T*> arr, ExtractKeyFunc extractKey = &ExtractKey<T>)
{RadixSort(arr.template Reinterpret<size_t>(), extractKey);}

namespace D
{
	template<typename T, typename ExtractKeyFunc, size_t RadixLog> INTRA_CONSTEXPR2 void radixSortPass(
		Span<T> arr, Span<T> temp, size_t radixOffset, ExtractKeyFunc extractKey)
	{
		enum: size_t {Radix = 1 << RadixLog, RadixMask = Radix-1};
		const size_t count = arr.Length();

		uint c[Radix] = {0};
		for(size_t j = 0; j < count; j++)
		{
			const auto key = extractKey(arr[j]);
			const size_t keyRadix = size_t(key >> radixOffset) & RadixMask;
			c[keyRadix]++;
		}
		for(size_t j = 1; j < Radix; j++) c[j] += c[j-1];
		for(size_t j = count-1; j != Core::MaxOf(size_t()); j--)
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
	enum: size_t {KeyBits = sizeof(extractKey(Core::Val<T>()))*8};

	Array<T> tempBuffer;
	tempBuffer.SetCountUninitialized(arr.Length());
	Span<T> dst = arr, temp = tempBuffer;
	size_t shift = 0;
	for(size_t i = 0; i < KeyBits / RadixLog; i++)
	{
		D::radixSortPass<T, ExtractKeyFunc, RadixLog>(dst, temp, shift, extractKey);
		shift += RadixLog;
		Swap(dst, temp);
	}
	if(shift < KeyBits)
	{
		D::radixSortPass<T, ExtractKeyFunc, KeyBits % RadixLog>(dst, temp, shift, extractKey);
		Swap(dst, temp);
	}
	if(arr==dst) return;
	CopyTo(temp, arr);
}

}
INTRA_END
