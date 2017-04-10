#pragma once

#include "Range/ForwardDecls.h"

namespace Intra { namespace Audio { namespace Synth {

//! Подобрать целое количество периодов размером samplesPerPeriod так,
//! чтобы их было не очень много, но конец переходил в начало с минимальным швом.
//! \return Количество повторений периода.
uint GetGoodSignalPeriod(double samplesPerPeriod, uint maxPeriods);

//! Повторить фрагмент fragmentSamples в буфере inOutSamples.
//! \param add Сложение (true) или присваивание (false).
void RepeatFragmentInBuffer(CSpan<float> fragmentSamples,
	Span<float> inOutSamples, bool add);

}}}
