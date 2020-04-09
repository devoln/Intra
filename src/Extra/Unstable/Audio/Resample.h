#pragma once

#include "Intra/Range/Span.h"
#include "Intra/Math/Math.h"

INTRA_BEGIN
inline float LinearSample(CSpan<float> arr, float index)
{
	const int i = int(index);
	return LinearMix(arr[i], arr[i+1], index - float(i));
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
INTRA_END
