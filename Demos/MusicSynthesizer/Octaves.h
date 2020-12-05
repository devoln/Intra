#pragma once

#include "IntraX/Container/Sequential/Array.h"
#include "IntraX/Unstable/Audio/Resample.h"
#include "Intra/Math/Math.h"
#include "Intra/Range/Mutation/Transform.h"

INTRA_BEGIN
namespace Audio { namespace Synth {

/// Предполагая, что src содержит периодический сигнал,
/// складывает его с копией самого себя с удвоенной частотой и множителем multiplier.
/// Результат записывается в dst.
void SelfOctaveMix(CSpan<float> src, Span<float> dst, float multiplier);

/// Предполагая, что srcResult содержит периодический сигнал,
/// складывает его с octavesCount-1 копиями самого себя с частотами x2, x4, ... и амплитудами x0.5, x0.25, ....
/// Результат записывается в один из двух предоставленных буферов, srcResult станет указывать на буфер с результатом.
void GenOctaves(Span<float>& srcResult, Span<float> buffer, unsigned octavesCount, unsigned maxSampleDelay);

}}}
