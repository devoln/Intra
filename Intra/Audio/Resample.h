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

//! �������� ���������� � ���������� 1/2 ��� ������������� �������������� ������.
//! @return �����������, � ������� ���������� �������������� ������.
Span<float> DecimateX2LinearInPlace(Span<float> inOutSamples);

//! �������� ���������� � ���������� 1/2 ������ src � ����� dst.
//! @return ����������� dst, � ������� ���������� �������������� ������.
Span<float> DecimateX2Linear(Span<float> dst, CSpan<float> src);

//! �������� ���������� � ���������� 2 ������ src � ����� dst.
//! @return ����������� dst, � ������� ���������� �������������� ������.
Span<float> UpsampleX2Linear(Span<float> dst, CSpan<float> src);

}}
