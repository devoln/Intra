#pragma once

#include "Utils/Span.h"
#include "Math/Math.h"

namespace Intra { namespace Audio {

inline float LinearSample(CSpan<float> arr, float index)
{
	const uint i = uint(index);
	return Math::LinearMix(arr[i], arr[i+1], index - float(i));
}

void ResampleLinear(CSpan<float> src, Span<float> dst);

//! Линейный ресемплинг с отношением 1/2 без использования дополнительной памяти.
//! @return Поддиапазон, в котором содержится результирующий сигнал.
Span<float> DecimateX2LinearInPlace(Span<float> inOutSamples);

//! Линейный ресемплинг с отношением 1/2 буфера src в буфер dst.
//! @return Поддиапазон dst, в котором содержится результирующий сигнал.
Span<float> DecimateX2Linear(Span<float> dst, CSpan<float> src);

//! Линейный ресемплинг с отношением 2 буфера src в буфер dst.
//! @return Поддиапазон dst, в котором содержится результирующий сигнал.
Span<float> UpsampleX2Linear(Span<float> dst, CSpan<float> src);

}}
