#pragma once

#include "Container/Sequential/Array.h"
#include "Audio/Resample.h"
#include "Math/Math.h"
#include "Range/Mutation/Transform.h"

namespace Intra { namespace Audio { namespace Synth {

//! �����������, ��� src �������� ������������� ������,
//! ���������� ��� � ������ ������ ���� � ��������� �������� � ���������� multiplier.
//! ��������� ������������ � dst.
void SelfOctaveMix(CSpan<float> src, Span<float> dst, float multiplier);

//! �����������, ��� srcResult �������� ������������� ������,
//! ���������� ��� � octavesCount-1 ������� ������ ���� � ��������� x2, x4, ... � ����������� x0.5, x0.25, ....
//! ��������� ������������ � ���� �� ���� ��������������� �������, srcResult ������ ��������� �� ����� � �����������.
void GenOctaves(Span<float>& srcResult, Span<float> buffer, uint octavesCount);

}}}
