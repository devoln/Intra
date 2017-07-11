#pragma once

#include "Container/Sequential/Array.h"
#include "Audio/Resample.h"
#include "Math/Math.h"
#include "Range/Mutation/Transform.h"

namespace Intra { namespace Audio { namespace Synth {

//! Предполагая, что src содержит периодический сигнал,
//! складывает его с копией самого себя с удвоенной частотой и множителем multiplier.
//! Результат записывается в dst.
void SelfOctaveMix(CSpan<float> src, Span<float> dst, float multiplier);

//! Предполагая, что srcResult содержит периодический сигнал,
//! складывает его с octavesCount-1 копиями самого себя с частотами x2, x4, ... и амплитудами x0.5, x0.25, ....
//! Результат записывается в один из двух предоставленных буферов, srcResult станет указывать на буфер с результатом.
void GenOctaves(Span<float>& srcResult, Span<float> buffer, uint octavesCount);

}}}
