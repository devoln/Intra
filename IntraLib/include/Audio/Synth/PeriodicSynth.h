#pragma once

#include "Range/ForwardDecls.h"

namespace Intra { namespace Audio { namespace Synth {

//! ��������� ����� ���������� �������� �������� samplesPerPeriod ���,
//! ����� �� ���� �� ����� �����, �� ����� ��������� � ������ � ����������� ����.
//! \return ���������� ���������� �������.
uint GetGoodSignalPeriod(double samplesPerPeriod, uint maxPeriods);

//! ��������� �������� fragmentSamples � ������ inOutSamples.
//! \param add �������� (true) ��� ������������ (false).
void RepeatFragmentInBuffer(ArrayRange<const float> fragmentSamples,
	ArrayRange<float> inOutSamples, bool add);

}}}
