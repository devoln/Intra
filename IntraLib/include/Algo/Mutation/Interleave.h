#pragma once

#include "Range/Concepts.h"
#include "Range/ArrayRange.h"

namespace Intra { namespace Algo {

//! Скопировать нескольких массивов src в один массив dst с чередующимися элементами:
//! dst = {src[0][0], ..., src.Last()[0], src[0][1], ..., src.Last()[1], ...}
template<typename T> void Interleave(ArrayRange<T> dst, ArrayRange<const ArrayRange<const T>> src)
{
	INTRA_ASSERT(!dst.Empty());
	INTRA_ASSERT(!src.Empty());
	const size_t channelCount = src.Length();
	const size_t valuesPerChannel = src.First().Length();
	INTRA_ASSERT(dst.Length() == valuesPerChannel*channelCount);

	for(size_t i=0; i<valuesPerChannel; i++)
	{
		for(size_t c=0; c<channelCount; c++)
		{
			dst.First() = src[c][i];
			dst.PopFirst();
		}
	}
}

//! Скопировать массив src с чередующимися элементами в несколько отдельных массивов:
//! dst = {src[0][0], ..., src.Last()[0], src[0][1], ..., src.Last()[1], ...}
template<typename T> void Deinterleave(ArrayRange<const ArrayRange<T>> dst, ArrayRange<const T> src)
{
	INTRA_ASSERT(!dst.Empty());
	INTRA_ASSERT(!src.Empty());
	const size_t channelCount = dst.Length();
	const size_t valuesPerChannel = dst.First().Length();
	INTRA_ASSERT(src.Length() == valuesPerChannel*channelCount);

	for(size_t i=0; i<valuesPerChannel; i++)
		for(size_t c=0; c<channelCount; c++)
		{
			dst[c][i] = src.First();
			src.PopFirst();
		}
}

}}
