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

//! �������� ���������� � ���������� 1/2 ��� ������������� �������������� ������.
//! @return �����������, � ������� ���������� �������������� ������.
Span<float> DecimateX2LinearInPlace(Span<float> inOutSamples);

//! �������� ���������� � ���������� 1/2 ������ src � ����� dst.
//! @return ����������� dst, � ������� ���������� �������������� ������.
Span<float> DecimateX2Linear(Span<float> dst, CSpan<float> src);

//! �������� ���������� � ���������� 2 ������ src � ����� dst.
//! @return ����������� dst, � ������� ���������� �������������� ������.
Span<float> UpsampleX2Linear(Span<float> dst, CSpan<float> src);
INTRA_END
