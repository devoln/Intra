#pragma once

#include "IntraX/Container/Sequential/Array.h"
#include "IntraX/Unstable/Audio/Resample.h"
#include "Intra/Math/Math.h"
#include "Intra/Range/Mutation/Transform.h"

INTRA_BEGIN
namespace Audio { namespace Synth {

/// �����������, ��� src �������� ������������� ������,
/// ���������� ��� � ������ ������ ���� � ��������� �������� � ���������� multiplier.
/// ��������� ������������ � dst.
void SelfOctaveMix(CSpan<float> src, Span<float> dst, float multiplier);

/// �����������, ��� srcResult �������� ������������� ������,
/// ���������� ��� � octavesCount-1 ������� ������ ���� � ��������� x2, x4, ... � ����������� x0.5, x0.25, ....
/// ��������� ������������ � ���� �� ���� ��������������� �������, srcResult ������ ��������� �� ����� � �����������.
void GenOctaves(Span<float>& srcResult, Span<float> buffer, unsigned octavesCount, unsigned maxSampleDelay);

}}}
