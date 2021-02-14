#pragma once

#include "Intra/Range/Span.h"
#include "Intra/Math/Math.h"

namespace Intra { INTRA_BEGIN
inline float LinearSample(Span<const float> arr, float index)
{
	const int i = int(index);
	return LinearMix(arr[i], arr[i+1], index - float(i));
}

void ResampleLinear(Span<const float> src, Span<float> dst);

/// �������� ���������� � ���������� 1/2 ��� ������������� �������������� ������.
/// @return �����������, � ������� ���������� �������������� ������.
Span<float> DecimateX2LinearInPlace(Span<float> inOutSamples);

/// �������� ���������� � ���������� 1/2 ������ src � ����� dst.
/// @return ����������� dst, � ������� ���������� �������������� ������.
Span<float> DecimateX2Linear(Span<float> dst, Span<const float> src);

/// �������� ���������� � ���������� 2 ������ src � ����� dst.
/// @return ����������� dst, � ������� ���������� �������������� ������.
Span<float> UpsampleX2Linear(Span<float> dst, Span<const float> src);
} INTRA_END
